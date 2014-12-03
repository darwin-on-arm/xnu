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

#define C_32_STK_ALIGN          16
#define C_32_PARAMSAVE_LEN      32
#define C_32_LINKAGE_LEN        48

#define TRUNC_DOWN32(a,c)   ((((uint32_t)a)-(c)) & ((uint32_t)(-(c))))

/*
 * The stack layout possibilities (info style); This needs to mach with signal trampoline code
 *
 * Traditional:			1
 * 32bit context		30
 */
#define UC_TRAD			1
#define UC_FLAVOR		30

 /* The following are valid mcontext sizes */
#define UC_FLAVOR_SIZE ((ARM_THREAD_STATE_COUNT + ARM_EXCEPTION_STATE_COUNT + ARM_VFP_STATE_COUNT) * sizeof(int))

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by chmk
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, the frame 
 * pointer, and the argument pointer, it returns
 * to the user specified pc, psl.
 */

void sendsig(struct proc *p, user_addr_t ua_catcher, int sig, int mask, __unused uint32_t code)
{
    user_addr_t ua_sp;
    user_addr_t ua_fp;
    user_addr_t ua_sip;
    user_addr_t trampact;
    user_addr_t ua_uctxp;
    user_addr_t ua_mctxp;
    user_siginfo_t  sinfo32;

    struct uthread *ut;
    struct mcontext mctx32;
    struct user_ucontext32 uctx32;
    struct sigacts *ps = p->p_sigacts;
 
    void *state;
    arm_thread_state_t *tstate32;
    mach_msg_type_number_t state_count;

    int stack_size = 0;
    int infostyle = UC_TRAD;
    int oonstack, flavor, error;
    
    proc_unlock(p);

    thread_t thread = current_thread();
    ut = get_bsdthread_info(thread);

    /*
     * Set up thread state info.
     */
    flavor = ARM_THREAD_STATE;
    state = (void *)&mctx32.ss;
    state_count = ARM_THREAD_STATE_COUNT;
    if (thread_getstatus(thread, flavor, (thread_state_t)state, &state_count) != KERN_SUCCESS)
        goto bad;

    flavor = ARM_EXCEPTION_STATE;
    state = (void *)&mctx32.es;
    state_count = ARM_EXCEPTION_STATE_COUNT;
    if (thread_getstatus(thread, flavor, (thread_state_t)state, &state_count) != KERN_SUCCESS)
        goto bad;

    flavor = ARM_VFP_STATE;
    state = (void *)&mctx32.fs;
    state_count = ARM_VFP_STATE_COUNT;
    if (thread_getstatus(thread, flavor, (thread_state_t)state, &state_count) != KERN_SUCCESS)
        goto bad;

    tstate32 = &mctx32.ss;

    /*
     * Set the signal style.
     */
    if (p->p_sigacts->ps_siginfo & sigmask(sig))
        infostyle = UC_FLAVOR;

    /*
     * Get the signal disposition.
     */
    trampact = ps->ps_trampact[sig];

    /*
     * Figure out where our new stack lives.
     */
    oonstack = ut->uu_sigstk.ss_flags & SA_ONSTACK;
    if ((ut->uu_flag & UT_ALTSTACK) && !oonstack && (ps->ps_sigonstack & sigmask(sig))) {
        ua_sp = ut->uu_sigstk.ss_sp;
        ua_sp += ut->uu_sigstk.ss_size;
        stack_size = ut->uu_sigstk.ss_size;
        ut->uu_sigstk.ss_flags |= SA_ONSTACK;
    } else {
        ua_sp = tstate32->sp;
    }

    /*
     * Set up the stack.
     */
    ua_sp -= UC_FLAVOR_SIZE;
    ua_mctxp = ua_sp;

    ua_sp -= sizeof (struct user_ucontext32);
    ua_uctxp = ua_sp;

    ua_sp -= sizeof (siginfo_t);
    ua_sip = ua_sp;

    /*
     * Align the stack pointer.
     */
    ua_sp = TRUNC_DOWN32(ua_sp, C_32_STK_ALIGN);

    /*
     * Build the signal context to be used by sigreturn.
     */
    uctx32.uc_onstack = oonstack;
    uctx32.uc_sigmask = mask;
    uctx32.uc_stack.ss_sp = ua_sp;
    uctx32.uc_stack.ss_size = stack_size;

    if (oonstack)
            uctx32.uc_stack.ss_flags |= SS_ONSTACK;

    uctx32.uc_link = 0;

    uctx32.uc_mcsize = UC_FLAVOR_SIZE;

    uctx32.uc_mcontext = ua_mctxp;

    /*
     * init siginfo
     */
    bzero((caddr_t)&sinfo32, sizeof(user_siginfo_t));

    sinfo32.si_signo = sig;
    sinfo32.pad[0]  = tstate32->sp;
    sinfo32.si_addr = tstate32->lr;

    switch (sig) {
        case SIGILL:
            sinfo32.si_code = ILL_NOOP;
            break;
        case SIGFPE:
            sinfo32.si_code = FPE_NOOP;
            break;
        case SIGBUS:
            sinfo32.si_code = BUS_ADRALN;
            break;
        case SIGSEGV:
            sinfo32.si_code = SEGV_ACCERR;
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
                sinfo32.si_pid = p->si_pid;
                p->si_pid = 0;
                status_and_exitcode = p->si_status;
                p->si_status = 0;
                sinfo32.si_uid = p->si_uid;
                p->si_uid = 0;
                sinfo32.si_code = p->si_code;
                p->si_code = 0;
                proc_unlock(p);
                if (sinfo32.si_code == CLD_EXITED) {
                    if (WIFEXITED(status_and_exitcode))
                        sinfo32.si_code = CLD_EXITED;
                    else if (WIFSIGNALED(status_and_exitcode)) {
                        if (WCOREDUMP(status_and_exitcode)) {
                            sinfo32.si_code = CLD_DUMPED;
                            status_and_exitcode = W_EXITCODE(status_and_exitcode, status_and_exitcode);
                        } else {
                            sinfo32.si_code = CLD_KILLED;
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
                sinfo32.si_status = WEXITSTATUS(status_and_exitcode);
                break;
            }
    }

    /*
     * Copy out context info.
     */
    if (copyout((caddr_t)&uctx32, ua_uctxp, sizeof(struct user_ucontext32))) 
        goto bad;
    if (copyout((caddr_t)&sinfo32, ua_sip, sizeof(user_siginfo_t)))
        goto bad;
    if (copyout((caddr_t)&mctx32, ua_mctxp, sizeof(struct mcontext))) 
        goto bad;
    if (copyout((caddr_t)&ua_uctxp, ua_sp, sizeof(user_addr_t)))
        goto bad;

    /*
     * Set up regsiters for the trampoline.
     */
    tstate32->r[0] = ua_catcher;
    tstate32->r[1] = infostyle;
    tstate32->r[2] = sig;
    tstate32->r[3] = ua_sip;
    tstate32->sp = ua_sp;

    if (trampact & 0x01) {
        tstate32->lr = trampact;
        tstate32->cpsr = 0x10; /* ARM_FIQ_MODE */
    } else {
        tstate32->lr = (trampact & ~0x01);
        tstate32->cpsr = 0x30; /* ARM_THUMB_MODE | ARM_USER_MODE */
    }

    /*
     * Call the trampoline.
     */
    flavor = ARM_THREAD_STATE;
    state_count = ARM_THREAD_STATE_COUNT;
    state = (void *)tstate32;
    if ((error = thread_setstatus(thread, flavor, (thread_state_t)state, state_count)) != KERN_SUCCESS)
        panic("sendsig: thread_setstatus failed, ret = %08X\n", error);

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
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper priviledges or to cause
 * a machine fault.
 */

int sigreturn(struct proc *p, struct sigreturn_args *uap, __unused int *retval)
{
    void *state;
    int onstack = 0;
    int error, flavor;
    thread_t thread;
    struct uthread *ut;
    struct mcontext mctx32;
    struct user_ucontext32 uctx32;
    mach_msg_type_number_t state_count;

    thread = current_thread();
    ut = get_bsdthread_info(thread);

    /*
     * Retrieve the user context that contains our machine context.
     */
    if ((error = copyin(uap->uctx, (void *)&uctx32, sizeof(struct user_ucontext32))))
        return (error);

    /*
     * Validate that our machine context is the right size.
     */
    if (uctx32.uc_mcsize != UC_FLAVOR_SIZE)
        return (EINVAL);

    /*
     * Populate our machine context info that we need to restore.
     */
    if ((error = copyin(uctx32.uc_mcontext, (void *)&mctx32, UC_FLAVOR_SIZE)))
        return (error);

    /*
     * Restore the signal mask.
     */
    ut->uu_sigmask = uctx32.uc_sigmask & ~sigcantmask;

    if ((uctx32.uc_onstack & 0x01))
        ut->uu_sigstk.ss_flags |= SA_ONSTACK;
    else
        ut->uu_sigstk.ss_flags &= ~SA_ONSTACK;

    if (ut->uu_siglist & ~ut->uu_sigmask)
        signal_setast(thread);

    /*
     * Restore the states from our machine context.
     * NOTE: we don't really need to check on infostyle since state restoring
     * for UC_TRAD and UC_FLAVOR is identical on this architecture.
     */
    flavor = ARM_THREAD_STATE;
    state = (void *)&mctx32.ss;
    state_count = ARM_THREAD_STATE_COUNT;
    if (thread_setstatus(thread, flavor, (thread_state_t)state, state_count) != KERN_SUCCESS)
        return (EINVAL);

    flavor = ARM_VFP_STATE;
    state = (void *)&mctx32.fs;
    state_count = ARM_VFP_STATE_COUNT;
    if (thread_setstatus(thread, flavor, (thread_state_t)state, state_count) != KERN_SUCCESS)
        return (EINVAL);

    return (EJUSTRETURN);
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
