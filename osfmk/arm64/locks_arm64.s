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
    .asciz "hw_lock_lock(): LOCK 0x%016x = 0x%08x"

/**
 * arm_usimple_lock_try and friends
 */
.align 6
.globl _arm_usimple_lock_try
.globl _lck_spin_try_lock
.globl _hw_lock_try
_arm_usimple_lock_try:
_lck_spin_try_lock:
_hw_lock_try:
    mrs     x1, daif

    /* Disable interrupts. */
    movz    x2, #0xC0
    bic     x3, x1, x2
    msr     daif, x3

    /* Operation. */
    ldr     w3, [x0]
    movz    w2, #1
    orr     w4, w3, #1
    ands    w2, w2, w3

    b.eq    .L_lock_try_store_lock
    b.ne    .L_lock_try_store_fail

.L_lock_try_increment_preempt:
    /* Increment preemption level. */
    mrs     x4, tpidr_el1

    /* Increment thread preemption count. */
    ldr     w5, [x4, MACHINE_THREAD_PREEMPT_COUNT]
    adds    w5, w5, #1 
    str     w5, [x4, MACHINE_THREAD_PREEMPT_COUNT]

    movz    x0, #1
    b       .L_lock_try_store_exit

.L_lock_try_store_lock:
    str     w4, [x0]
    b       .L_lock_try_increment_preempt

.L_lock_try_store_fail:
    movz    x0, #0

.L_lock_try_store_exit:
    msr     daif, x1
    ret     lr

/**
 * hw_lock_to
 */
.align 6
.globl _hw_lock_to
_hw_lock_to:
    /* Increment preemption level. */
    mrs     x4, tpidr_el1

    /* Increment thread preemption count. */
    ldr     w3, [x4, MACHINE_THREAD_PREEMPT_COUNT]
    adds    w3, w3, #1 
    str     w3, [x4, MACHINE_THREAD_PREEMPT_COUNT]

    /* Operation. */
    ldr     w3, [x0]
    movz    w2, #1
    orr     w1, w3, #1
    ands    w2, w2, w3
    b.eq    .L_lock_to_store

.L_lock_to_rejoin:
    tst     w2, #1
    b.ne    .L_lock_to_preempt
    ret     lr

.L_lock_to_store:
    str     w1, [x0]
    b       .L_lock_to_rejoin

.L_lock_to_preempt:
    stp     fp, lr, [sp, #-16]!
    add     fp, sp, #0
    bl      __enable_preemption
    add     sp, fp, #0
    ldp     fp, lr, [sp], #16
    ret     lr

/**
 * lock_read and friends
 */
.align 6
.globl _lock_read
.globl _lck_rw_lock_shared
_lock_read:
_lck_rw_lock_shared:
    movz    w3, #0xD
rwlsloop:
    ldxr    w3, [x0]
    ands    w2, w1, w3
    b.ne    rwlsopt
rwlsloopres:
    add     w1, w1, #0x10000
    stxr    w2, w1, [x0]
    cmp     w2, #0
    b.eq    rwlsloopexit
    b       rwlsloop
rwlsopt:
    movz    w2, #0x8001
    ands    w2, w1, w2
rwlsexit:
    stp     fp, lr, [sp, #-16]!
    add     fp, sp, #0
    bl      _lck_rw_lock_shared_gen
    add     sp, fp, #0
    ldp     fp, lr, [sp], #16
    ret     lr
rwlsloopexit:
    ret     lr

/**
 * lock_done and friends
 */
.align 6
.globl _lock_done
.globl _lck_rw_done
    ldxr    w1, [x0]
    ands    w2, w1, #1
    b.ne    rwldpanic
    movz    w3, #0xFFFF
    lsr     w3, w3, #16
    ands    w2, w1, w3
    b.eq    rwldexcl
    sub     w1, w1, #0x10000
    ands    w2, w1, w3
    mov     w4, wzr
    movz    w3, #1
    b.ne    rwldshared1
    ands    w4, w1, #2
    movz    w5, #2
    bic     w1, w1, w5
rwldshared1:
    b       rwldstore
rwldexcl:
    ands    w2, w1, #4
    b.ne    rwldexclne
    b       rwldjoin
rwldexclne:
    orr     w3, w3, #2
    b       rwldexcl1
rwldjoin:
    movz    w2, #0xA
rwldexcl1:
    movz    w3, #0x2
    and     w3, w1, #2
    bic     w1, w1, w2
rwldstore:
    stxr    w2, w1, [x0]
    cmp     w2, #0
    b.ne    rwld_jump_lock_done
    cmp     w12, #0
    b.eq    rwldexit
    stp     fp, lr, [sp, #-16]!
    stp     x0, x3, [sp, #-16]!
    add     fp, sp, #0
    bl      _lck_rw_lock_shared_gen
    add     sp, fp, #0
    ldp     x0, x3, [sp], #16
    ldp     fp, lr, [sp], #16
    ret     lr
rwldexit:
    mov     x0, x3
    ret     lr
rwldpanic:
    mov     x2, x1
    mov     x1, x0
    adr     x0, L_rwldpanicstring
    bl      _panic
rwld_jump_lock_done:
    b       _lock_done
L_rwldpanicstring:
    .asciz "lck_rw_done(): lock (0x%016x: 0x%08x)"

/**
 * lock_read_to_write/lck_rw_lock_shared_to_exclusive
 */
.align 6
.globl _lock_read_to_write
.globl _lck_rw_lock_shared_to_exclusive
_lock_read_to_write:
_lck_rw_lock_shared_to_exclusive:
    ldr     w1, [x0]
    movz    w3, #0xFFFF
    lsr     w3, w3, #16
    ands    w2, w1, w3
    b.ne    rwlsepanic
    bic     w1, w1, w3
    subs    w2, w2, #0x10000
    b.ne    rwlsejump
    movz    w3, #5
    ands    w3, w1, w4
    b.ne    rwlsejump
    orr     w1, w1, #4
    str     w1, [x0]
    movz    x0, #1
    ret     lr
rwlsepanic:
    mov     x2, x1
    mov     x1, x0
    adr     x0, L_rwlsepanicstring
    bl      _panic
rwlsejump:
    stp     fp, lr, [sp, #-16]!
    add     fp, sp, #0
    bl      _lck_rw_lock_shared_gen
    add     sp, fp, #0
    ldp     fp, lr, [sp], #16
    ret     lr
L_rwlsepanicstring:
    .asciz "lck_rw_lock_shared_to_exclusive(): LOCK 0x%016x = 0x%016x"

/**
 * lck_mtx_unlock
 */
.align 6
.globl _lck_mtx_unlock 
_lck_mtx_unlock:
    mrs     x4, daif

    /* Disable interrupts. */
    movz    x2, #0xC0
    bic     x3, x1, x2
    msr     daif, x3

    movz    w2, #0
    mrs     x5, tpidr_el1
mluloop:
    ldxr    w1, [x0]
    ands    w3, w1, #3
    b.ne    lmuslow
    movz    w6, #3
    bic     w3, w1, w5
    cmp     w3, w12
    b.ne    lmupanic    // This is broken.
    stxr    w1, w2, [x0]
    cmp     w1, #0
    b.eq    mluexit
    b       mluloop 
lmuslow:
    stp     x0, x1, [sp, #-16]!
    stp     fp, lr, [sp, #-16]!
    add     fp, sp, #0
    mrs     x5, tpidr_el1
    ldr     w6, [x5, MACHINE_THREAD_PREEMPT_COUNT]
    adds    w6, w6, #1 
    str     w6, [x5, MACHINE_THREAD_PREEMPT_COUNT]
    ldr     w1, [x0]
    ands    w3, w1, #1
    b.ne    lmupanic
    orr     w3, w1, #1
    str     w3, [x0]
    movz    w2, #3
    bic     w1, w1, w2
    ands    w2, w3, #2
    b.ne    lmupanic
    bl      _lck_mtx_unlock_wakeup
    add     sp, fp, #0
    ldp     fp, lr, [sp], #16
    ldp     x0, x1, [sp], #16
    ldr     w1, [x0]
    and     w3, w1, #2
    str     w3, [x0]
    b       __enable_preemption
lmupanic:
    mov     x1, x0
    ldr     w2, [x1]
    adr     x0, L_lmupanicstr
    bl      _panic
L_lmupanicstr:
    .asciz "lck_mtx_unlock(): MUTEX 0x%08x 0x%08x"
mluexit:
    ret     lr