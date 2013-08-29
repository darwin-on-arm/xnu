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
 * ARM machine routines
 */

#include <arm/arch.h>
#include <arm/asm_help.h>
#include <assym.s>

/*
 * This is ARMv6 and later ONLY.
 */

/**
 * lck_mtx_ilk_unlock
 */
EnterARM(lck_mtx_ilk_unlock)
    ldr     r2, [r0]
    bic     r3, r2, #1
    str     r3, [r0]
    LoadConstantToReg(__enable_preemption, pc)

/**
 * ml_set_interrupts_enabled
 *
 * Disable or enable interrupts based on input boolean.
 */
EnterARM(ml_set_interrupts_enabled)
    mrs     r2, cpsr

    cmp     r0, #0
    beq     disable_interrupts

astloop:
    mrc     p15, 0, r0, c13, c0, 4
    add     r1, r0, MACHINE_THREAD_CPU_DATA
    add     r0, r1, CPU_PENDING_AST
    
    stmfd   sp!,{r0,r2,r7,lr}
    bl      _get_preemption_level
    cmp     r0, #0
    ldmfd   sp!,{r0,r2,r7,lr}

    bne     enable_interrupts

    ldr     r0, [r0]
    and     r0, r0, #4
    cmp     r0, #0
    beq     enable_interrupts

    stmfd   sp!,{r0-r7,lr}
    movs    r0, #7
    movs    r1, #0
    blx     _ast_taken
    ldmfd   sp!,{r0-r7,lr}

    b       astloop

enable_interrupts:
    cpsie   i
    b       out

disable_interrupts:
    cpsid   i
    b       out

out:
    mov     r0, #1
    bic     r0, r0, r2, lsr#7
    bx      lr

/**
 * ml_get_interrupts_enabled
 *
 * Get current interrupt state.
 */
EnterARM(ml_get_interrupts_enabled)
    mrs     r1, cpsr
    mov     r0, #1
    bic     r0, r0, r1, lsr#7
    bx      lr

/**
 * __disable_preemption
 *
 * Disable preemption for a specified thread.
 */
EnterARM(__disable_preemption)
EnterARM(_disable_preemption)
    LoadThreadRegister(r12)
    IncrementPreemptLevel(r12, r2)
    bx      lr

/**
 * get_preemption_level
 *
 * Get current thread's preemption level.
 */
EnterARM(get_preemption_level)
    LoadThreadRegister(r12)
    ldr     r0, [r12, MACHINE_THREAD_PREEMPT_COUNT]
    bx      lr

/**
 * __enable_preemption
 *
 * Enable preemption for a specified thread.
 */
EnterARM(__enable_preemption)
EnterARM(_enable_preemption)
    /* Get thread ID */
    LoadThreadRegister(r12)
    DecrementPreemptLevel(r12, r2)
    bxne    lr

    /* Check for interrupts */
    mrs     r3, cpsr
    tst     r3, #0x80
    strne   r2, [r12, MACHINE_THREAD_PREEMPT_COUNT]
    bxne    lr

    /* Get CPU data and add an AST. */
    ldr     r1, [r12, MACHINE_THREAD_CPU_DATA]
    add     r0, r1, CPU_PENDING_AST
    str     r2, [r12, MACHINE_THREAD_PREEMPT_COUNT]

    ands    r1, r0, #4
    bne     __preempt
    msr     cpsr_cf, r3
    bx      lr

__preempt:
    /* Reenable interrupts */
    cpsie   f
    mov     r0, #7
    mov     r1, #1
    LoadConstantToReg(_ast_taken + 1, pc)

/**
 * current_thread
 *
 * Return the core thread structure of the currently executing thread.
 * The reason this doesn't use the thread register is because the current
 * "executing" thread may not be the one in the register. Just get the one
 * saved from machine_set_current_thread.
 */
EnterARM(current_thread)
    LoadConstantToReg(_CurrentThread, r0)
    ldr     r0, [r0]
    bx      lr

/**
 * set_mmu_ttb/ttb_alt/ttbcr
 *
 * Set the translation table base register (and alternate TTBR) to point
 * to the physical address of a translation-table.
 *
 * Set the current TTB control register for a split.
 */
EnterARM(set_mmu_ttb)
    orr     r0, r0, #0x18
    mcr     p15, 0, r0, c2, c0, 0
    bx      lr
EnterARM(set_mmu_ttb_alt)
    orr     r0, r0, #0x18
    mcr     p15, 0, r0, c2, c0, 1
    bx      lr
EnterARM(set_mmu_ttbcr)
    mcr     p15, 0, r0, c2, c0, 2
    bx      lr

/**
 * flush_mmu_single
 *
 * Flush a MVA specific entry from the TLB.
 */
 EnterARM(flush_mmu_single)
    /* Shove the lowest 12-bits off the VA */
    mov     r0, r0, lsr #12
    mov     r0, r0, lsl #12
    /* Clean it */
    mcr     p15, 0, r0, c8, c7, 1
    dsb     sy
    isb     sy
    bx      lr

/*
 * Things I put here because I am far too lazy to write them in C.
 */
EnterThumb(ml_cause_interrupt)
    bx      lr
