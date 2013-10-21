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
 * ARM thread state
 */

#ifndef _ARM_THREAD_H_
#define _ARM_THREAD_H_

#include <mach/boolean.h>
#include <mach/arm/vm_types.h>
#include <mach/thread_status.h>

#include <kern/lock.h>
#include <machine/pal_routines.h>

#include <arm/cpu_data.h>

/*
 * Maps state flavor to number of words in the state:
 */
__private_extern__ unsigned int _MachineStateCount[];

/*
 * Kernel stack stuff.
 */
#define FP_SIZE     sizeof(arm_saved_state_t)

#define STACK_IKS(stack)		\
    ((vm_offset_t)(((vm_offset_t)stack)+KERNEL_STACK_SIZE)-FP_SIZE)

/*
 * The machine-dependent thread state - registers and all platform-dependent
 * state - is saved in the machine thread structure which is embedded in
 * the thread data structure. For historical reasons this is also referred to
 * as the PCB.
 */
struct machine_thread {
    arm_saved_state_t *iss;

    arm_saved_state_t *uss;
    arm_vfp_state_t *uvfp;

    arm_saved_state_t user_regs;

    arm_exception_state_t es;

    cpu_data_t *cpu_data;

    int vfp_dirty;
    int vfp_enable;
    arm_vfp_state_t vfp_regs;

    uint32_t preempt_count;

#ifdef	MACH_BSD
    uint64_t cthread_self;      /* for use of cthread package  */
#endif
};
typedef struct machine_thread *pcb_t;

#define	THREAD_TO_PCB(Thr)	(&(Thr)->machine)

#define USER_STATE(Thr)		((Thr)->machine.iss)

/*
 * Return address of the function that called current function, given
 *	address of the first parameter of current function.
 */
#define	GET_RETURN_PC(addr)	(__builtin_return_address(0))

#endif
