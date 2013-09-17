/*
 * 64-bit ARM lock definitions.
 */
/* eeeeeeeeeeep. */

#include <assym.s>
#include <arm/asm_help.h>

/**
 * hw_lock_init/arm_usimple_lock_init
 * 
 * Initialize a lock and all of its bits to zero.
 */
.align 6
.globl _hw_lock_init
.globl _arm_usimple_lock_init
_hw_lock_init:
_arm_usimple_lock_init:
    str     wzr, [x0]
    ret     lr

/**
 * hw_lock_held
 *
 * Check a lock and see if the interlock bit is set or not.
 */
.align 6
.globl _hw_lock_held
_hw_lock_held:
    ldr    w3, [x0]
    mov    x0, xzr        /* Clobber register. */
    ands   w0, w3, #1
    ret    lr

/**
 * lck_spin_unlock/hw_lock_unlock
 */
.align 6
.globl _lck_spin_unlock
.globl _hw_lock_unlock
.globl _lck_mtx_ilk_unlock
_lck_spin_unlock:
_lck_mtx_ilk_unlock:
_hw_lock_unlock:
    ldr    w3, [x0]
    movz   w2, #1
    bic    w3, w3, w2
    str    w3, [x0]
    b      __enable_preemption

/**
 * arm_usimple_lock and friends.
 */
.align 6
.globl _arm_usimple_lock
.globl _lck_spin_lock
.globl _hw_lock_lock
_arm_usimple_lock:
_lck_spin_lock:
_hw_lock_lock:
    /* Load the current thread register. */
    mrs    x4, tpidr_el1
    
    /* Increment thread preemption count. */
    ldr    w5, [x4, MACHINE_THREAD_PREEMPT_COUNT]
    adds   w5, w5, #1 
    str    w5, [x4, MACHINE_THREAD_PREEMPT_COUNT]

    /* Operation. */
    ldr    w3, [x0]
    movz   w2, #1
    orr    w1, w3, #1
    ands   w2, w2, w3
    b.eq   .L_lock_out

.L_lock_panic:
    mov    x1, x0
    ldr    w2, [x1]
    adr    x0, L_lock_panic_string
    bl     _panic
    b      .

.L_lock_out:
    str    w1, [x0]
    ret    lr

L_lock_panic_string:
    .asciz "hw_lock_lock(): LOCK 0x%08x = 0x%08x"


