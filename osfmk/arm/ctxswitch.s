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
 * ARM context switching.
 */

#include <arm/arch.h>
#include <arm/asm_help.h>
#include <assym.s>
#include <mach/arm/asm.h>

/**
 * Call_continuation
 *
 * Switch the current context to the thread continuation and terminate on return.
 */
EnterARM(Call_continuation)
    /* Set the new stack pointer. */
    LoadThreadRegister(r9)
    ldr     sp, [r9, TH_PCB_ISS]

    /* Zero out frame pointer */
    mov     r7, #0

    /* Set arguments */
    mov     r6, r0
    mov     r0, r1
    mov     r1, r2

    /* Branch to continuation. */
    blx      r6

    /* Terminate thread. */
    mrc     p15, 0, r0, c13, c0, 4
    blx     _thread_terminate
    b       .

/**
 * Switch_context
 *
 * Switch the current processor context to that of the new thread.
 * Does not switch r0-r3 function argument registers.
 */
EnterARM(Switch_context)
    /* Switch context to the new thread and save the continuation. */
    teq     r1, #0
    strne   r1, [r0, TH_CONTINUATION]

    /* Only store registers if there is a continuation */
    ldreq   r3, [r0, TH_PCB_ISS]
    addeq   r3, r3, #16
    stmiaeq r3, {r4-lr}

    /* Save current thread */
    LOAD_ADDR(r4, CurrentThread)
    str     r2, [r4]

    /* Set old/new threads */
    mcr     p15, 0, r2, c13, c0, 4

    ldr     r3, [r2, MACHINE_THREAD_CTHREAD_SELF]
    mcr     p15, 0, r3, c13, c0, 3

    /* Load registers and go. */
    ldr     r3, [r2, TH_PCB_ISS]
    add     r3, r3, #16
    ldmia   r3!, {r4-lr}
    bx      lr

/**
 * machine_load_context
 *
 * Load the registers and prepare for a context switch.
 */
EnterARM(machine_load_context)
    /* Save current thread */
    LOAD_ADDR(r4, CurrentThread)
    str     r0, [r4]

    /* Set thread */
    mcr     p15, 0, r0, c13, c0, 4

    /* Set cthread value */
    ldr     r1, [r0, MACHINE_THREAD_CTHREAD_SELF]
    mrc     p15, 0, r2, c13, c0, 3

    /* Load registers and go. */
    ldr     r3, [r0, TH_PCB_ISS]
    mov     r0, #0

    add     r3, r3, #16
    ldmia   r3!, {r4-lr}
    bx      lr

/**
 * thread_syscall_return
 */ 
EnterARM(thread_syscall_return)
    cpsid   i
    LoadThreadRegister(r9)
    ldr     r4, [r9, TH_PCB_USS]
    str     r0, [r4]
    b       thread_return_join

/**
 * thread_exception_return
 *
 * Used to bootstrap a user thread.
 */
EnterARM(thread_exception_return)
EnterARM(thread_bootstrap_return)
    /* Disable interrupts */
    cpsid   i
    
thread_return_join:
    /* Check for pending ast */
    mrc     p15, 0, r9, c13, c0, 4
    
    ldr     r8, [r9, MACHINE_THREAD_CPU_DATA]
    ldr     r5, [r8, CPU_PENDING_AST]
    
    cmp     r5, #0
    beq     return_to_user

    /* There's an ast. */
    
    mov     r0, r5
    mov     r1, #1
    blx     _ast_taken
    
    b       _thread_exception_return

return_to_user:
    /* Restore registers */
    LoadThreadRegister(r5)
    ldr     sp, [r5, TH_PCB_USS]
    ldr     r0, [sp, #0x40]
    
    msr     spsr_cxsf, r0
#ifdef _ARM_ARCH_7
    clrex
#endif
    ldr     lr, [sp, #0x3C]
    
    ldmfd   sp, {r0-lr}^
    movs    pc, lr
    
LOAD_ADDR_GEN_DEF(CurrentThread)
