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
 * BSD stuff for ARM.
 */

#include <mach_rt.h>
#include <mach_debug.h>
#include <mach_ldebug.h>

#include <mach/kern_return.h>
#include <mach/mach_traps.h>
#include <mach/thread_status.h>
#include <mach/vm_param.h>

#include <kern/counters.h>
#include <kern/cpu_data.h>
#include <kern/mach_param.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/debug.h>
#include <kern/spl.h>
#include <kern/syscall_sw.h>
#include <ipc/ipc_port.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>

#define USRSTACK    0x2FE00000

/*
 * thread_userstack:
 *
 * Return the user stack pointer from the machine
 * dependent thread state info.
 */
kern_return_t thread_userstack(thread_t thread, int flavor,
                               thread_state_t tstate, unsigned int count,
                               mach_vm_offset_t * user_stack, int *customstack)
{
    struct arm_thread_state *state;

    if (!(*user_stack))
        *user_stack = USRSTACK;

    switch (flavor) {
    case ARM_THREAD_STATE:
        if (count < ARM_THREAD_STATE_COUNT)
            return KERN_INVALID_ARGUMENT;

        state = (struct arm_thread_state *) tstate;
        *user_stack = state->sp ? state->sp : USRSTACK;
        break;
    default:
        return KERN_INVALID_ARGUMENT;
    }

    return KERN_SUCCESS;
}

uint32_t sanitise_cpsr(uint32_t cpsr)
{
    uint32_t final_cpsr;

    final_cpsr = (cpsr) & ~(1 << 9);    /* Data endianness */
    final_cpsr &= ~(1 << 7);    /* IRQ */
    final_cpsr &= ~(1 << 6);    /* FIQ */
    final_cpsr &= 0xFFFFFFE0;   /* Mode bits */
    final_cpsr |= 0x10;         /* Set user mode */

    return final_cpsr;
}

kern_return_t thread_entrypoint(thread_t thread, int flavor,
                                thread_state_t tstate, unsigned int count,
                                mach_vm_offset_t * entry_point)
{
    struct arm_thread_state *state;

    /*
     * Set a default.
     */
    if (*entry_point == 0)
        *entry_point = VM_MIN_ADDRESS;

    switch (flavor) {
    case ARM_THREAD_STATE:
        if (count < ARM_THREAD_STATE_COUNT)
            return (KERN_INVALID_ARGUMENT);
        state = (struct arm_thread_state *) tstate;
        *entry_point = state->pc ? state->pc : VM_MIN_ADDRESS;
        break;
    default:
        return (KERN_INVALID_ARGUMENT);
    }

    return (KERN_SUCCESS);
}

kern_return_t thread_userstackdefault(thread_t thread,
                                      mach_vm_offset_t * default_user_stack)
{
    *default_user_stack = USRSTACK;
    return (KERN_SUCCESS);
}

/*
 *  thread_getstatus:
 *
 *  Get the status of the specified thread.
 */

kern_return_t machine_thread_get_state(thread_t thr_act, thread_flavor_t flavor,
                                       thread_state_t tstate,
                                       mach_msg_type_number_t * count)
{
    switch (flavor) {
    case THREAD_STATE_FLAVOR_LIST:
        {
            if (*count < 3)
                return (KERN_INVALID_ARGUMENT);

            tstate[0] = ARM_THREAD_STATE;
            tstate[1] = ARM_VFP_STATE;
            tstate[2] = ARM_EXCEPTION_STATE;
            *count = 3;
            break;
        }

    case THREAD_STATE_FLAVOR_LIST_NEW:
        {
            if (*count < 4)
                return (KERN_INVALID_ARGUMENT);

            tstate[0] = ARM_THREAD_STATE;
            tstate[1] = ARM_VFP_STATE;
            tstate[2] = ARM_EXCEPTION_STATE;
            tstate[3] = ARM_DEBUG_STATE;

            *count = 4;
            break;
        }

    case ARM_THREAD_STATE:
        {
            struct arm_thread_state *state;
            struct arm_thread_state *saved_state;

            if (*count < ARM_THREAD_STATE_COUNT)
                return (KERN_INVALID_ARGUMENT);

            state = (struct arm_thread_state *) tstate;
            saved_state = (struct arm_thread_state *) thr_act->machine.uss;

            /*
             * First, copy everything:
             */
            *state = *saved_state;

            *count = ARM_THREAD_STATE_COUNT;
            break;
        }
    case ARM_VFP_STATE:
        {
            struct arm_vfp_state *state;
            struct arm_vfp_state *saved_state;

            if (*count < ARM_VFP_STATE_COUNT)
                return (KERN_INVALID_ARGUMENT);

            state = (struct arm_vfp_state *) tstate;
            saved_state = (struct arm_vfp_state *) &thr_act->machine.vfp_regs;

            /*
             * First, copy everything:
             */
            *state = *saved_state;

            *count = ARM_VFP_STATE_COUNT;
            break;
        }

    default:
        return (KERN_INVALID_ARGUMENT);
    }

    return (KERN_SUCCESS);
}

kern_return_t machine_thread_set_state(thread_t thread, thread_flavor_t flavor,
                                       thread_state_t tstate,
                                       mach_msg_type_number_t count)
{
    struct arm_thread_state *ts;

    switch (flavor) {           /* Validate the count before we do anything else */
    case ARM_THREAD_STATE:
        if (count < ARM_THREAD_STATE_COUNT) {   /* Is it too short? */
            return KERN_INVALID_ARGUMENT;   /* Yeah, just leave... */
        }
        break;
    case ARM_DEBUG_STATE:
        if (count < ARM_DEBUG_STATE_COUNT) {    /* Is it too short? */
            return KERN_INVALID_ARGUMENT;   /* Yeah, just leave... */
        }
    case ARM_VFP_STATE:
        if (count < ARM_VFP_STATE_COUNT) {  /* Is it too short? */
            return KERN_INVALID_ARGUMENT;   /* Yeah, just leave... */
        }
        break;
    case ARM_EXCEPTION_STATE:
        if (count < ARM_EXCEPTION_STATE_COUNT) {    /* Is it too short? */
            return KERN_INVALID_ARGUMENT;   /* Yeah, just leave... */
        }
        break;
    default:
        return KERN_INVALID_ARGUMENT;
    }

    /*
     * Now set user registers. 
     */
    assert(thread != NULL);
    assert(tstate);

    thread->machine.uss = &thread->machine.user_regs;

    switch (flavor) {
    case ARM_THREAD_STATE:
        ts = (struct arm_thread_state *) tstate;
        thread->machine.user_regs.r[0] = ts->r[0];
        thread->machine.user_regs.r[1] = ts->r[1];
        thread->machine.user_regs.r[2] = ts->r[2];
        thread->machine.user_regs.r[3] = ts->r[3];
        thread->machine.user_regs.r[4] = ts->r[4];
        thread->machine.user_regs.r[5] = ts->r[5];
        thread->machine.user_regs.r[6] = ts->r[6];
        thread->machine.user_regs.r[7] = ts->r[7];
        thread->machine.user_regs.r[8] = ts->r[8];
        thread->machine.user_regs.r[9] = ts->r[9];
        thread->machine.user_regs.r[10] = ts->r[10];
        thread->machine.user_regs.r[11] = ts->r[11];
        thread->machine.user_regs.r[12] = ts->r[12];
        thread->machine.user_regs.sp = ts->sp;
        thread->machine.user_regs.lr = ts->lr;
        thread->machine.user_regs.pc = ts->pc;
        thread->machine.user_regs.cpsr = sanitise_cpsr(ts->cpsr);
        return KERN_SUCCESS;
    default:
        return KERN_INVALID_ARGUMENT;
    }

    return KERN_INVALID_ARGUMENT;
}

void thread_setuserstack(thread_t thread, mach_vm_address_t user_stack)
{
    assert(thread);
    thread->machine.user_regs.sp = CAST_DOWN(unsigned int, user_stack);
}

uint64_t thread_adjuserstack(thread_t thread, int adj)
{
    assert(thread);
    thread->machine.user_regs.sp += adj;
    return thread->machine.user_regs.sp;
}

void thread_setentrypoint(thread_t thread, uint32_t entry)
{
    assert(thread);
    thread->machine.user_regs.pc = entry;
}


/** 
 * ACT support.
 */

void *
act_thread_csave(void)
{
    kern_return_t kret;
    mach_msg_type_number_t val;
    thread_t thr_act = current_thread();
    struct arm_thread_state_t *ts;

    ts = (struct arm_thread_state_t *)kalloc(sizeof(struct arm_thread_state));

    if (ts == (struct arm_thread_state_t *)NULL)
        return((void *)0);

    val = ARM_THREAD_STATE; 
    kret = machine_thread_get_state(thr_act, ARM_THREAD_STATE,
           (thread_state_t) &ts, &val);
    if (kret != KERN_SUCCESS) {
        kfree(ts, sizeof(struct arm_thread_state));
        return((void *)0);
    }

    return ts;
}


