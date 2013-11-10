/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* 
 * Copyright (c) 1992 NeXT, Inc.
 *
 * HISTORY
 * 13 May 1992 ? at NeXT
 *	Created.
 */

#include <mach/mach_types.h>
#include <mach/exception.h>
#include <kern/thread.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <sys/sysproto.h>
#include <sys/sysent.h>
#include <sys/ucontext.h>
#include <sys/wait.h>
#include <mach/thread_act.h>    /* for thread_abort_safely */
#include <mach/thread_status.h>
#include <mach/arm/thread_state.h>
#include <mach/arm/thread_status.h>
#include <arm/machine_routines.h>
#include <machine/pal_routines.h>
#include <sys/kdebug.h>
#include <sys/sdt.h>

#define	C_32_REDZONE_LEN	224
#define	C_32_STK_ALIGN		16
#define C_32_PARAMSAVE_LEN	64
#define	C_32_LINKAGE_LEN	48

#define TRUNC_DOWN32(a,b,c)	((((uint32_t)a)-(b)) & ((uint32_t)(-(c))))
/*
 * The stack layout possibilities (info style); This needs to mach with signal trampoline code
 *
 * Traditional:			1
 * Traditional64:		20
 * Traditional64with vec:	25
 * 32bit context		30
 * 32bit context with vector	35
 * 64bit context		40
 * 64bit context with vector	45
 * Dual context			50
 * Dual context with vector	55
 */
#define UC_TRAD			1
#define UC_TRAD_VEC		6
#define UC_TRAD64		20
#define UC_TRAD64_VEC		25
#define UC_FLAVOR		30
#define UC_FLAVOR_VEC		35
#define UC_FLAVOR64		40
#define UC_FLAVOR64_VEC		45
#define UC_DUAL			50
#define UC_DUAL_VEC		55

void sendsig(struct proc *p, user_addr_t ua_catcher, int sig, int mask, __unused uint32_t code)
{
    struct mcontext mctx;
    thread_t th_act;
    struct uthread *ut;
    void *tstate;
    int flavor;
    user_addr_t p_mctx = USER_ADDR_NULL;    /* mcontext dest. */
    int infostyle = UC_TRAD;
    mach_msg_type_number_t state_count;
    user_addr_t trampact;
    int oonstack;
    struct user_ucontext32 uctx;
    user_addr_t sp;
    user_addr_t p_uctx;         /* user stack addr top copy ucontext */
    user_siginfo_t sinfo;
    user_addr_t p_sinfo;        /* user stack addr top copy siginfo */
    struct sigacts *ps = p->p_sigacts;
    int stack_size = 0;
    kern_return_t kretn;
    
    th_act = current_thread();
    kprintf("sendsig: Sending signal to thread %p, code %d.\n", th_act, sig);
    return;

    ut = get_bsdthread_info(th_act);

    if (p->p_sigacts->ps_siginfo & sigmask(sig)) {
        infostyle = UC_FLAVOR;
    }

    flavor = ARM_THREAD_STATE;
    tstate = (void *) &mctx.ss;
    state_count = ARM_THREAD_STATE_COUNT;
    if (thread_getstatus(th_act, flavor, (thread_state_t) tstate, &state_count) != KERN_SUCCESS)
        goto bad;

    trampact = ps->ps_trampact[sig];
    oonstack = ut->uu_sigstk.ss_flags & SA_ONSTACK;

    /*
     * figure out where our new stack lives 
     */
    if ((ut->uu_flag & UT_ALTSTACK) && !oonstack && (ps->ps_sigonstack & sigmask(sig))) {
        sp = ut->uu_sigstk.ss_sp;
        sp += ut->uu_sigstk.ss_size;
        stack_size = ut->uu_sigstk.ss_size;
        ut->uu_sigstk.ss_flags |= SA_ONSTACK;
    } else {
        sp = CAST_USER_ADDR_T(mctx.ss.sp);
    }

    /*
     * context goes first on stack 
     */
    sp -= sizeof(struct ucontext);
    p_uctx = sp;

    /*
     * this is where siginfo goes on stack 
     */
    sp -= sizeof(user32_siginfo_t);
    p_sinfo = sp;

    /*
     * final stack pointer 
     */
    sp = TRUNC_DOWN32(sp, C_32_PARAMSAVE_LEN + C_32_LINKAGE_LEN, C_32_STK_ALIGN);

    uctx.uc_mcsize = (size_t) ((ARM_THREAD_STATE_COUNT) * sizeof(int));
    uctx.uc_onstack = oonstack;
    uctx.uc_sigmask = mask;
    uctx.uc_stack.ss_sp = sp;
    uctx.uc_stack.ss_size = stack_size;
    if (oonstack)
        uctx.uc_stack.ss_flags |= SS_ONSTACK;
    uctx.uc_link = 0;

    /*
     * setup siginfo 
     */
    bzero((caddr_t) & sinfo, sizeof(sinfo));
    sinfo.si_signo = sig;
    sinfo.si_addr = CAST_USER_ADDR_T(mctx.ss.pc);
    sinfo.pad[0] = CAST_USER_ADDR_T(mctx.ss.sp);

    switch (sig) {
    case SIGILL:
        sinfo.si_code = ILL_NOOP;
        break;
    case SIGFPE:
        sinfo.si_code = FPE_NOOP;
        break;
    case SIGBUS:
        sinfo.si_code = BUS_ADRALN;
        break;
    case SIGSEGV:
        sinfo.si_code = SEGV_ACCERR;
        break;
    default:
        {
            int status_and_exitcode;

            /*
             * All other signals need to fill out a minimum set of
             * information for the siginfo structure passed into
             * the signal handler, if SA_SIGINFO was specified.
             *
             * p->si_status actually contains both the status and
             * the exit code; we save it off in its own variable
             * for later breakdown.
             */
            proc_lock(p);
            sinfo.si_pid = p->si_pid;
            p->si_pid = 0;
            status_and_exitcode = p->si_status;
            p->si_status = 0;
            sinfo.si_uid = p->si_uid;
            p->si_uid = 0;
            sinfo.si_code = p->si_code;
            p->si_code = 0;
            proc_unlock(p);
            if (sinfo.si_code == CLD_EXITED) {
                if (WIFEXITED(status_and_exitcode))
                    sinfo.si_code = CLD_EXITED;
                else if (WIFSIGNALED(status_and_exitcode)) {
                    if (WCOREDUMP(status_and_exitcode)) {
                        sinfo.si_code = CLD_DUMPED;
                        status_and_exitcode = W_EXITCODE(status_and_exitcode, status_and_exitcode);
                    } else {
                        sinfo.si_code = CLD_KILLED;
                        status_and_exitcode = W_EXITCODE(status_and_exitcode, status_and_exitcode);
                    }
                }
            }
            /*
             * The recorded status contains the exit code and the
             * signal information, but the information to be passed
             * in the siginfo to the handler is supposed to only
             * contain the status, so we have to shift it out.
             */
            sinfo.si_status = WEXITSTATUS(status_and_exitcode);
            break;
        }
    }

    if (copyout(&uctx, p_uctx, sizeof(struct user_ucontext32)))
        goto bad;

    if (copyout(&sinfo, p_sinfo, sizeof(sinfo)))
        goto bad;

    /*
     * set signal registers, these are probably wrong.. 
     */
    {
        mctx.ss.r[0] = CAST_DOWN(uint32_t, ua_catcher);
        mctx.ss.r[1] = (uint32_t) infostyle;
        mctx.ss.r[2] = (uint32_t) sig;
        mctx.ss.r[3] = CAST_DOWN(uint32_t, p_sinfo);
        mctx.ss.r[4] = CAST_DOWN(uint32_t, p_uctx);
        mctx.ss.pc = CAST_DOWN(uint32_t, trampact);
        mctx.ss.sp = CAST_DOWN(uint32_t, sp);
        state_count = ARM_THREAD_STATE_COUNT;
        printf("sendsig: Sending signal to thread %p, code %d, new pc 0x%08x\n", th_act, sig, trampact);
        if ((kretn = thread_setstatus(th_act, ARM_THREAD_STATE, (void *) &mctx.ss, state_count)) != KERN_SUCCESS) {
            panic("sendsig: thread_setstatus failed, ret = %08X\n", kretn);
        }
    }

    proc_lock(p);
    return;

 bad:
    proc_lock(p);
    SIGACTION(p, SIGILL) = SIG_DFL;
    sig = sigmask(SIGILL);
    p->p_sigignore &= ~sig;
    p->p_sigcatch &= ~sig;
    ut->uu_sigmask &= ~sig;
    /*
     * sendsig is called with signal lock held 
     */
    proc_unlock(p);
    psignal_locked(p, SIGILL);
    proc_lock(p);
    return;
}

/*
 * machine_exception() performs MD translation
 * of a mach exception to a unix signal and code.
 */

boolean_t machine_exception(int exception, mach_exception_code_t code, __unused mach_exception_subcode_t subcode, int *unix_signal, mach_exception_code_t * unix_code)
{

    switch (exception) {

    case EXC_BAD_ACCESS:
        /*
         * Map GP fault to SIGSEGV, otherwise defer to caller 
         */
        *unix_signal = SIGSEGV;
        *unix_code = code;
        return (FALSE);

    case EXC_BAD_INSTRUCTION:
        *unix_signal = SIGILL;
        *unix_code = code;
        break;

    case EXC_ARITHMETIC:
        *unix_signal = SIGFPE;
        *unix_code = code;
        break;

    case EXC_SOFTWARE:
        *unix_signal = SIGTRAP;
        *unix_code = code;
        break;

    default:
        return (FALSE);
    }

    return (TRUE);
}
