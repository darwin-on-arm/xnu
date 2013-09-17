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
    /* Don't have the ARM instruction manual in front of me. :\ */
#warning fix me
    ret    lr
