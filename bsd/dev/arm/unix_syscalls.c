/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
 * Copyright 2013, winocm. <winocm@icloud.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 *   If you are going to use this software in any form that does not involve
 *   releasing the source to this project or improving it, let me know beforehand.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * BSD system calls for ARM.
 */

#include <mach/mach_types.h>
#include <mach/exception.h>

#include <kern/task.h>
#include <kern/thread.h>
#include <kern/assert.h>
#include <kern/clock.h>
#include <kern/locks.h>
#include <kern/sched_prim.h>
#include <kern/debug.h>

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

#include <arm/machine_routines.h>

#include <sys/kdebug.h>
#include <sys/sdt.h>

#include <security/audit/audit.h>

#include <mach/branch_predicates.h>

/* dynamically generated at build time based on syscalls.master */
extern const char *syscallnames[];

/*
 * Function:	unix_syscall
 *
 * Inputs:	regs	- pointer to arm save area
 *
 * Outputs:	none
 */
void unix_syscall(arm_saved_state_t * state)
{
    thread_t thread;
    void *vt;
    unsigned int code;
    struct sysent *callp;
    int error = 0;
    vm_offset_t params;
    struct proc *p;
    struct uthread *uthread;
    boolean_t args_in_uthread;
    boolean_t is_vfork;

    assert(state != NULL);
    thread = current_thread();
    uthread = get_bsdthread_info(thread);

    /*
     * Get the approriate proc; may be different from task's for vfork() 
     */
    is_vfork = uthread->uu_flag & UT_VFORK;
    if (__improbable(is_vfork != 0))
        p = current_proc();
    else
        p = (struct proc *) get_bsdtask_info(current_task());

    /*
     * Verify that we are not being called from a task without a proc 
     */
    if (__improbable(p == NULL)) {
        state->r[0] = EPERM;
        task_terminate_internal(current_task());
        thread_exception_return();
        /*
         * NOTREACHED 
         */
    }

    /*
     * Current syscall number is in r12 on ARM 
     */
    boolean_t shift_args = FALSE;

    state->cpsr &= ~(1 << 29);  /* C-bit IIRC, turn this into a Define */
    code = state->r[12];

    /*
     * Delayed binding of thread credential to process credential, if we
     * are not running with an explicitly set thread credential.
     */
    kauth_cred_uthread_update(uthread, p);

    callp = (code >= NUM_SYSENT) ? &sysent[63] : &sysent[code];
    if (__improbable(callp == sysent)) {
        /*
         * indirect system call... system call number
         * passed as 'arg0'
         */
        code = state->r[0];
        callp = (code >= NUM_SYSENT) ? &sysent[63] : &sysent[code];
        shift_args = TRUE;
    }

    if (callp->sy_narg) {
        int arg_count = callp->sy_narg;
        if (!shift_args) {
            int i;
            for (i = 0; i < 12; i++) {
                uthread->uu_arg[i] = state->r[i];
                if (i == (arg_count))
                    break;
            }
        } else {
            int i;
            for (i = 0; i < 11; i++) {
                uthread->uu_arg[i] = state->r[i + 1];
                if (i == (arg_count))
                    break;
            }            
        }
    }

    /*
     * Set return values 
     */
    uthread->uu_flag |= UT_NOTCANCELPT;
    uthread->uu_rval[0] = 0;
    uthread->uu_rval[1] = 0;

    AUDIT_SYSCALL_ENTER(code, p, uthread);
    error = (*(callp->sy_call)) (p, (void *) uthread->uu_arg, &(uthread->uu_rval[0]));
    AUDIT_SYSCALL_EXIT(code, p, uthread, error);

#if 0
    kprintf("SYSCALL: %s (%d, routine %p), args %p, return %x (%x, %x) pc 0x%08x\n", syscallnames[code >= NUM_SYSENT ? 63 : code], code, callp->sy_call, (void *) uthread->uu_arg, error, uthread->uu_rval[0], uthread->uu_rval[1], state->pc);
#endif

#if CONFIG_MACF
    mac_thread_userret(code, error, thread);
#endif

    if (error)
        state->cpsr |= (1 << 29);  /* C-bit IIRC, turn this into a Define */

    if (error == ERESTART) {
        panic("unix_syscall: restarting syscall\n");
    } else if(error != EJUSTRETURN) {
        if(error) {
            state->r[0] = error;
        } else { /* Not error. */
            switch(callp->sy_return_type) {
                case _SYSCALL_RET_INT_T:
                    state->r[0] = (int)uthread->uu_rval[0];                    
                    state->r[1] = (int)uthread->uu_rval[1];
                    break;
                case _SYSCALL_RET_UINT_T:
                    state->r[0] = (u_int)uthread->uu_rval[0];                    
                    state->r[1] = (u_int)uthread->uu_rval[1];
                    break;
                case _SYSCALL_RET_OFF_T:
                case _SYSCALL_RET_UINT64_T:
                    state->r[0] = uthread->uu_rval[0];                    
                    state->r[1] = uthread->uu_rval[1];
                    break;
                case _SYSCALL_RET_ADDR_T:
                case _SYSCALL_RET_SIZE_T:
                case _SYSCALL_RET_SSIZE_T: {
                    user_addr_t *retp = (user_addr_t *)&uthread->uu_rval[0];
                    state->r[0] = *retp;
                    state->r[1] = 0;
                }
                    break;
                case _SYSCALL_RET_NONE:
                    break;
                default:
                    panic("unix_syscall: unknown return type");
                    break;
            }
        }
    }

    uthread->uu_flag &= ~UT_NOTCANCELPT;

    /*
     * panic if funnel is held 
     */
    syscall_exit_funnelcheck();

    if (uthread->uu_lowpri_window) {
        /*
         * task is marked as a low priority I/O type
         * and the I/O we issued while in this system call
         * collided with normal I/O operations... we'll
         * delay in order to mitigate the impact of this
         * task on the normal operation of the system
         */
        throttle_lowpri_io(TRUE);
    }

    thread_exception_return();
}

void unix_syscall_return(int error)
{
    thread_t thread_act;
    struct uthread *uthread;
    struct proc *proc;
    arm_saved_state_t *state;
    unsigned int code = 0;
    struct sysent *callp;

    thread_act = current_thread();
    proc = current_proc();
    uthread = get_bsdthread_info(thread_act);

    state = find_user_regs(thread_act);

    if (state->r[12] != 0)
        code = state->r[12];

    callp = (code >= NUM_SYSENT) ? &sysent[63] : &sysent[code];

    AUDIT_SYSCALL_EXIT(code, proc, uthread, error);

    if (error == ERESTART) {
        panic("unix_syscall: restarting syscall\n");
    } else if(error != EJUSTRETURN) {
        if(error) {
            state->r[0] = error;
        } else { /* Not error. */
            switch(callp->sy_return_type) {
                case _SYSCALL_RET_INT_T:
                    state->r[0] = uthread->uu_rval[0];                    
                    state->r[1] = uthread->uu_rval[1];
                    break;
                case _SYSCALL_RET_UINT_T:
                    state->r[0] = (u_int)uthread->uu_rval[0];                    
                    state->r[1] = (u_int)uthread->uu_rval[1];
                    break;
                case _SYSCALL_RET_OFF_T:
                case _SYSCALL_RET_UINT64_T:
                    state->r[0] = uthread->uu_rval[0];                    
                    state->r[1] = uthread->uu_rval[1];
                    break;
                case _SYSCALL_RET_ADDR_T:
                case _SYSCALL_RET_SIZE_T:
                case _SYSCALL_RET_SSIZE_T: {
                    user_addr_t *retp = (user_addr_t *)&uthread->uu_rval[0];
                    state->r[0] = *retp;
                    state->r[1] = 0;
                }
                    break;
                case _SYSCALL_RET_NONE:
                    break;
                default:
                    panic("unix_syscall: unknown return type");
                    break;
            }
            state->cpsr &= ~(1 << 29);  /* C-bit IIRC, turn this into a Define */
        }
    }
    
    uthread->uu_flag &= ~UT_NOTCANCELPT;

#if FUNNEL_DEBUG    
    /*
     * if we're holding the funnel panic
     */
    syscall_exit_funnelcheck();
#endif /* FUNNEL_DEBUG */

    if (uthread->uu_lowpri_window) {
        /*
         * task is marked as a low priority I/O type
         * and the I/O we issued while in this system call
         * collided with normal I/O operations... we'll
         * delay in order to mitigate the impact of this
         * task on the normal operation of the system
         */
        throttle_lowpri_io(TRUE);
    }
    if (kdebug_enable && (code != 180)) {
        if (callp->sy_return_type == _SYSCALL_RET_SSIZE_T)
            KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_EXCP_SC, code) | DBG_FUNC_END, error, uthread->uu_rval[1], 0, 0, 0);
        else
            KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_EXCP_SC, code) | DBG_FUNC_END, error, uthread->uu_rval[0], uthread->uu_rval[1], 0, 0);
    }

    thread_exception_return();
    /*
     * NOTREACHED 
     */
}
