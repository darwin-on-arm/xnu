/*
 * ARM64 machine routines.
 */

#include <arm/asm_help.h>
#include <assym.s>

/** 
 * __disable_preemption
 *
 * Disable preemption for a specified thread.
 */
.align 6
.globl __disable_preemption
.globl ___disable_preemption
__disable_preemption:
___disable_preemption:
    /* Load the thread register. */
    mrs    x0, tpidr_el1
    
    /* Increment the thread register. */
    ldr    w1, [x0, MACHINE_THREAD_PREEMPT_COUNT]
    adds   w1, w1, #1
    str    w1, [x0, MACHINE_THREAD_PREEMPT_COUNT]

    /* Return. */
    ret   lr

/**
 * get_preemption_level
 *
 * Return the current thread's preemption level.
 */
.align 6
.globl _get_preemption_level
_get_preemption_level:
    mrs    x1, tpidr_el1
    ldr    w0, [x1, MACHINE_THREAD_PREEMPT_COUNT]
    ret    lr

/**
 * set_mmu_ttb/friends.
 *
 * You should know what this does.
 */
.align 6
.globl _set_mmu_ttb
.globl _set_mmu_ttb_alt
.globl _set_mmu_ttbcr
_set_mmu_ttb:
    msr    ttbr0_el1, x0
    ret    lr
_set_mmu_ttb_alt:
    msr    ttbr1_el1, x0
    ret    lr
_set_mmu_ttbcr:
    msr    tcr_el1, x0
    ret    lr

/**
 * ml_get_interupts_enabled
 *
 * Get the current interrupt state.
 */
.align 6
.globl _ml_get_interrupts_enabled
_ml_get_interrupts_enabled:
    mrs    x0, daif
    movz   x0, #1
    bic    x0, x0, x1, lsr #7
    ret    lr

/**
 * __enable_preemption
 *
 * Enable task preemption.
 */
.align 6
.globl __enable_preemption
.globl ___enable_preemption
__enable_preemption:
___enable_preemption:
    mrs    x0, tpidr_el1

    ldr    w1, [x0, MACHINE_THREAD_PREEMPT_COUNT]
    subs   w1, w1, #1
    str    w1, [x0, MACHINE_THREAD_PREEMPT_COUNT]

    b.ne   .L_enable_preemption_out

    /* Check for interrupts */
    mrs    x2, daif
    tst    x2, #0x80
    b.ne   .L_enable_preemption_store_count

    /* Get CPU data and add an AST. */
    ldr    x3, [x0, MACHINE_THREAD_CPU_DATA]
    add    x4, x3, CPU_PENDING_AST
    str    w1, [x0, MACHINE_THREAD_PREEMPT_COUNT]

    ands   x2, x4, #4
    b.ne   .L_enable_preemption_preempt
    ret    lr

.L_enable_preemption_preempt:
    orr    x2, x2, #(1 << 6)
    msr    daif, x2
    isb    sy

    movz   x0, #7
    movz   x1, #1
    b      _ast_taken

.L_enable_preemption_store_count:
    str    w1, [x0, MACHINE_THREAD_PREEMPT_COUNT]

.L_enable_preemption_out:
    ret    lr

/* lol. */
.align 6
.globl _ml_cause_interrupt
_ml_cause_interrupt:
    ret    lr


/**
 * current_thread
 *
 * Return the core thread structure of the currently executing thread.
 */
.align 6
.globl _current_thread
_current_thread:
    mrs     x0, tpidr_el1
    ret     lr
