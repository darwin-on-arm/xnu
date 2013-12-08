/*
 * ARM lock definitions
 */

/*
 * !!! DO NOT MODIFY THIS FILE !!!
 * !!! IT WORKS PROPERLY, YOU MIGHT BREAK SOMETHING !!!
 */

#include <mach_assert.h>
#include <assym.s>
#include <arm/asm_help.h>
#include <mach/arm/asm.h>

/*
 * OMAP3530 on BeagleBoard xM has issues with STREX/LDREX. Do ourselves a favor
 * and not use them. It doesn't matter as this SoC is uniprocessor.
 */
#ifdef BOARD_CONFIG_OMAP3530
#undef BOARD_CONFIG_OMAP3530
#endif

#ifdef NO_EXCLUSIVES
#define ldrex           ldr
#endif

#if __ARM_ARCH == 6
#undef EnterThumb
#define EnterThumb EnterARM
#endif

/*
 * Lock bit definitions are not defined here. Please don't touch. Thanks.
 */

/* Yes, this was blatantly taken from the real kernel, sorry about that. */

/**
 * hw_lock_init/arm_usimple_lock_init
 *
 * Initialize a lock and all of its bits to zero.
 */ 
EnterThumb(hw_lock_init)
EnterThumb(arm_usimple_lock_init)
    mov     r1, #0
    str     r1, [r0]
    bx      lr

/**
 * hw_lock_held
 *
 * Check a lock and see if the interlock bit is set or not.
 */
EnterThumb(hw_lock_held)
    ldr     r3, [r0]
    mov     r2, #1
    and     r0, r2, r3
    bx      lr

.align 4

/**
 * arm_usimple_lock and friends
 */
EnterARM(arm_usimple_lock)
EnterARM(lck_spin_lock)
EnterARM(hw_lock_lock)
    LoadLockHardwareRegister(r12)
    IncrementPreemptLevel(r12, r2)
    ldr     r3, [r0]
    mov     r2, #1
    orr     r1, r3, #1
    ands    r2, r2, r3
    streq   r1, [r0]
    bxeq    lr
hw_lock_lock_panic:
    mov     r1, r0
    ldr     r2, [r1]
    adr     r0, _panicString
    blx     _panic
    b       .
_panicString:
    .asciz "hw_lock_lock(): PANIC: Lock 0x%08x = 0x%08x"


/**
 * hw_lock_unlock and friends
 */
EnterARM(lck_spin_unlock)
EnterARM(hw_lock_unlock)
    ldr     r3, [r0]
    bic     r3, r3, #1
    str     r3, [r0]
    b       __enable_preemption

/**
 * arm_usimple_lock_try and friends
 */
EnterARM(arm_usimple_lock_try)
EnterARM(lck_spin_try_lock)
EnterARM(hw_lock_try)
    mrs     r1, cpsr
    cpsid   if
    ldr     r3, [r0]
    mov     r2, #1
    orr     r12, r3, #1
    ands    r2, r2, r3
    streq   r12, [r0]
    bne     hw_lock_try_fail
    LoadLockHardwareRegister(r12)
    IncrementPreemptLevel(r12, r2)
    mov     r0, #1
    b       hw_lock_try_return
hw_lock_try_fail:
    mov     r0, #0
hw_lock_try_return:
    msr     cpsr_cf, r1
    bx      lr

/**
 * hw_lock_to
 */
EnterARM(hw_lock_to)
    LoadLockHardwareRegister(r12)
    IncrementPreemptLevel(r12, r2)
    ldr     r3, [r0]
    mov     r2, #1
    orr     r1, r3, #1
    ands    r2, r2, r3
    streq   r1, [r0]
    eors    r0, r2, #1
    beq     hw_lock_to_enable_preempt
    bx      lr
hw_lock_to_enable_preempt:
    stmfd   sp!, {r0,r1,r7,lr}
    add     r7, sp, #8
    blx     __enable_preemption
    ldmfd   sp!, {r0,r1,r7,lr}
    bx      lr

/**
 * lock_read and friends
 */
EnterARM(lock_read)
EnterARM(lck_rw_lock_shared)
#ifdef NO_EXCLUSIVES
    mrs         r9, cpsr
    orr         r3, r9, #0xc0
    msr         cpsr_cf, r3
#endif
    mov     r3, #0xD
rwlsloop:
    ldrex   r1, [r0]
    ands    r2, r1, r3
    bne     rwlsopt
rwlsloopres:
    add     r1, r1, #0x10000
#ifndef BOARD_CONFIG_OMAP3530
    strex   r2, r1, [r0]
    movs    r2, r2
    bxeq    lr
    b       rwlsloop
    str     r1, [r0]
    bx      lr
#endif
    str     r1, [r0]
    bx      lr
rwlsopt:
    mov     r2, #0x8001
    ands    r2, r1, r2
    LOAD_ADDR(r12, lck_rw_lock_shared_gen)
    bx      r12
    movs    r2, r1, lsr#16
    bne     rwlsloopres
rwlsloolow:
    LOAD_ADDR(r12, lck_rw_lock_shared_gen)
    bx      r12

/**
 * lock_write and friends
 */
EnterARM(lock_write)
EnterARM(lck_rw_lock_exclusive)
    LoadConstantToReg(0xFFFF000D, r3)
#ifdef NO_EXCLUSIVES
    mrs         r12, cpsr
    orr         r2, r12, #0xc0
    msr         cpsr_cf, r2
#endif
rwleloop:
    ldrex       r1, [r0]
    ands        r2, r1, r3
    bne         rwleslow
    orr         r1, r1, #8
#ifndef NO_EXCLUSIVES
    strex       r2, r1, [r0]
    movs        r2, r2
    bxeq        lr
    b           rwleloop
#else
    str         r1, [r0]
    msr         cpsr_cf, r12
    bx          lr
#endif
rwleslow:
    LOAD_ADDR(r12, lck_rw_lock_exclusive_gen)
    bx          r12

/**
 * lock_done and friends
 */
EnterARM(lock_done)
EnterARM(lck_rw_done)
#ifdef BOARD_CONFIG_OMAP3530
    mrs         r9, cpsr
    cpsid       if
#endif
_lock_done_enter:
    ldrex       r1, [r0]
    ands        r2, r1, #1
    bne         rwldpanic
    LoadConstantToReg(0xFFFF0000, r3)
    ands        r2, r1, r3
    beq         rwldexcl
    sub         r1, r1, #0x10000
    ands        r2, r1, r3
    mov         r12, #0
    mov         r3, #1
    bne         rwldshared1
    ands        r12, r1, #2
    bics        r1, r1, #2
rwldshared1:
    b           rwldstore
rwldexcl:
    ands        r2, r1, #4
    orrne       r2, r2, #2
    bne         rwldexcl1
    mov         r2, #0xA
rwldexcl1:
    mov         r3, #2
    and         r12, r1, #2
    bic         r1, r1, r2
rwldstore:
#ifndef BOARD_CONFIG_OMAP3530
    strex       r2, r1, [r0]
    movs        r2, r2
    bne         _lock_done_enter
#else
    str         r1, [r0]
#endif
#ifdef BOARD_CONFIG_OMAP3530
    msr         cpsr_cf, r9
#endif
    movs        r12, r12
    moveq       r0, r3
    bxeq        lr
    stmfd       sp!,{r0,r3,r7,lr}
    add         r0, r0, #8
    blx         _thread_wakeup
    ldmfd       sp!,{r0,r3,r7,lr}
    mov         r0, r3
    bx          lr
rwldpanic:
    mov         r2, r1
    mov         r1, r0
    adr         r0, lckPanicString
    blx         _panic
lckPanicString:
    .asciz "lck_rw_done(): lock (0x%08x: 0x%08x)"

/**
 * lock_read_to_write and friends
 */
EnterARM(lock_read_to_write)
EnterARM(lck_rw_lock_shared_to_exclusive)
    ldrex       r1, [r0]
    LoadConstantToReg(0xFFFF0000, r3)
    ands        r2, r1, r3
    beq         rwlsepanic
    bic         r1, r1, r3
    LOAD_ADDR(r12, lck_rw_lock_shared_gen)
    subs        r2, r2, #0x10000
    bxne        r12
    ands        r3, r1, #5
    bxne        r12
    orr         r1, r1, #4
#ifndef BOARD_CONFIG_OMAP3530
    strex       r2, r1, [r0]
    movs        r2, r2
    bne         _lock_read_to_write
#else
    str         r1, [r0]
#endif
    mov         r0, #1
    bx          lr
rwlsepanic:
    mov         r1, r2
    mov         r1, r0
    adr         r0, lckReadToWritePanicString
    blx         _panic
lckReadToWritePanicString:
    .asciz "lck_rw_lock_shared_to_exclusive(): lock (0x%08x 0x%08x)"

/**
 * lock_write_to_read and friends
 */
EnterARM(lck_rw_lock_exclusive_to_shared)
EnterARM(lock_write_to_read)
#ifdef NO_EXCLUSIVES
    mrs         r12, cpsr
    orr         r2, r12, #0xc0
    msr         cpsr_cf, r2
#endif
    ldrex       r1, [r0]
    ands        r2, r1, #1
    bne         rwlstexit
    and         r2, r1, #2
    ands        r3, r1, #4
    mov         r3, #6
    moveq       r3, #0xA
    bic         r1, r1, r3
    orr         r1, r1, #0x10000
#ifndef BOARD_CONFIG_OMAP3530
    strex       r3, r1, [r0]
    movs        r3, r3
    bne         _lock_write_to_read
#else
    str         r1, [r0]
    msr         cpsr_cf, r12
#endif
    movs        r2, r2
    bxeq        lr
    add         r0, r0, #8
    LOAD_ADDR(r12, thread_wakeup)
    bx          r12
rwlstexit:
    mov         r2, r1
    mov         r1, r0
    adr         r0, rwlstePanicString
    blx         _panic
rwlstePanicString:
    .asciz "lck_rw_lock_exclusive_to_shared(): lock (0x%08x 0x%08x)"


/**
 * lck_mtx_unlock
 */
EnterARM(lck_mtx_unlock)
#ifdef NO_EXCLUSIVES
    mrs     r9, cpsr
    orr     r2, r9, #0xc0
    msr     cpsr_cf, r2
#endif
    mov     r2, #0
    LoadLockHardwareRegister(r12)
mluloop:
    ldrex       r1, [r0]
    ands        r3, r1, #3
    bne         lmuslow
    bic         r3, r1, #3
    cmp         r3, r12
    bne         lmupanic
#ifndef NO_EXCLUSIVES
    strex       r1, r2, [r0]
    movs        r1, r1
    bxeq        lr
#else
    str         r2, [r0] 
    msr         cpsr_cf, r9
    bx          lr
#endif
    
    b           mluloop
lmuslow:
#ifdef NO_EXCLUSIVES
    msr         cpsr_cf, r9
#endif
    stmfd       sp!,{r0,r1,r7,lr}
    add         r7, sp, #8
    LoadLockHardwareRegister(r12)
    IncrementPreemptLevel(r12, r2)
    ldr         r1, [r0]
    ands        r3, r1, #1
    bne         lmupanic
    orr         r3, r1, #1
    str         r3, [r0]
    bics        r1, r1, #3
    ands        r2, r3, #2
    beq         lmupanic
    blx         _lck_mtx_unlock_wakeup
    ldmfd       sp!,{r0,r1,r7,lr}
    ldr         r1, [r0]
    and         r3, r1, #2
    str         r3, [r0]
lmuret:
    b           __enable_preemption
lmupanic:
    mov         r1, r0
    ldr         r2, [r1]
    adr         r0, lmuPanicString
    blx         _panic
lmuPanicString:
    .asciz      "lck_mtx_unlock(): mutex (0x%08x, 0x%08x)"

/**
 * lck_mtx_lock
 */
EnterARM(lck_mtx_lock)
    LoadLockHardwareRegister(r12)
mlckretry:
#ifdef NO_EXCLUSIVES
    mrs     r2, cpsr
    orr     r3, r2, #0xc0
    msr     cpsr_cf, r3
#endif
    ldrex   r3, [r0]
    movs    r3, r3
    bne     mlckslow
#ifndef NO_EXCLUSIVES
    strex   r1, r12, [r0]
    movs    r1, r1
    bne     mlckretry
#else
    str     r12, [r0]
    msr     cpsr_cf, r2
#endif 
    bx      lr
mlckslow:
#ifdef NO_EXCLUSIVES
    msr     cpsr_cf, r2
#endif
    stmfd   sp!,{r0,r1,r7,lr}
    add     r7, sp, #8
    LoadLockHardwareRegister(r12)
    IncrementPreemptLevel(r12, r2)
    ldr     r3, [r0]
    ands    r2, r3, #1
    bne     mlckpanic
    bics    r1, r3, #2
    bne     mlckwait
    orr     r3, r3, #1
    str     r3, [r0]
    blx     _lck_mtx_lock_acquire
    ands    r0, r0, r0
    ldmfd   sp!,{r0,r1,r7,lr}
    LoadLockHardwareRegister(r12)
    mov     r3, r12
    orrne   r3, r3, #2
    str     r3, [r0]
    b       __enable_preemption
mlckwait:
    orr     r3, r3, #0
    str     r3, [r0]
    blx     _lck_mtx_lock_wait
    ldmfd   sp!,{r0,r1,r7,lr}
    LoadLockHardwareRegister(r12)
    b       mlckretry
mlckpanic:
    mov     r1, r0
    ldr     r2, [r1]
    adr     r0, mlckpanicString
    blx     _panic
mlckpanicString:
    .asciz  "lck_mtx_lock(): mutex (0x%08x, 0x%08x)"

/**
 * lck_mtx_try_lock
 */
EnterARM(lck_mtx_try_lock)
    LoadLockHardwareRegister(r12)
#ifdef NO_EXCLUSIVES
    mrs         r2, cpsr
    orr         r3, r2, #0xc0
    msr         cpsr_cf, r3
#endif
lmtstart:
    ldrex       r3, [r0]
    movs        r3, r3
    bne         lmtslow
#ifndef NO_EXCLUSIVES
    strex       r1, r12, [r0]
    movs        r1, r1
    bne         lmtstart
#else
    str         r12, [r0]
    msr         cpsr_cf, r2
#endif
    mov         r0, #1
    bx          lr
lmtslow:
#ifdef NO_EXCLUSIVES
    msr         cpsr_cf, r2
#endif
    mov         r1, #0
    stmfd       sp!, {r0,r1,r7,lr}
    add         r7, sp, #8
    IncrementPreemptLevel(r12, r2)
    ldr         r3, [r0]
    ands        r2, r3, #1
    bne         lmtpanic
    bics        r2, r3, #2
    bne         lmtret
    orr         r3, r3, #1
    str         r3, [r0]
    blx         _lck_mtx_lock_acquire
    ands        r0, r0, r0
    ldmfd       sp!,{r0,r1,r7,lr}
    LoadLockHardwareRegister(r12)
    mov         r3, r12
    orrne       r3, r3, #2
    str         r3, [r0]
    mov         r1, #1
    stmfd       sp!,{r0,r1,r7,lr}
    add         r7, sp, #8
lmtret:
    bl          __enable_preemption
    ldmfd       sp!,{r0,r1,r7,lr}
    mov         r0, r1
    bx          lr
lmtpanic:
    mov         r1, r0
    ldr         r2, [r1]
    adr         r0, lmtPanicString
    blx         _panic
lmtPanicString:
    .asciz "lck_mtx_try_lock(): mutex (0x%08x, 0x%08x)"

/**
 * lck_mtx_assert
 */
EnterARM(lck_mtx_assert)
    ldr     r12, [r0]
    bics    r3, r12, #3
    cmp     r1, #1
    bne     lck_mtx_assert_owned
    LoadLockHardwareRegister(r12)
    cmp     r3, r12
    bxeq    lr
    mov     r1, r0
    adr     r0, panicString_lockNotOwned
    blx     _panic
lck_mtx_assert_owned:
    cmp     r1, #2
    bne     lck_mtx_assert_bad_arg
    LoadLockHardwareRegister(r12)
    cmp     r3, r12
    bxne    lr
    mov     r1, r0
    adr     r0, panicString_lockOwned
    blx     _panic
lck_mtx_assert_bad_arg:
    adr     r0, panicString_lockBadArgument
    blx     _panic

    .align 4
panicString_lockNotOwned:
    .asciz  "lck_mtx_assert(): mutex (0x%08X) not owned\n"
    .align 4
panicString_lockOwned:
    .asciz  "lck_mtx_assert(): mutex (0x%08X) owned\n"
    .align 4
panicString_lockBadArgument:
    .asciz  "lck_mtx_assert(): arg1 (0x%08X) invalid\n"

/**
 * lck_rw_try_lock_shared
 */
EnterARM(lck_rw_try_lock_shared)
#ifdef NO_EXCLUSIVES
    mrs     r9, cpsr
    orr     r1, r9, #0xc0
    msr     cpsr_cf, r1
#endif
    ldrex   r1, [r0]
    ands    r2, r1, #1
    bne     rwtlspanic
    ands    r2, r1, #0xC
    bne     rwtlsopt
rwtlsloopres:
    add     r1, r1, #0x10000
#ifndef NO_EXCLUSIVES
    strex   r2, r1, [r0]
    movs    r2, r2
    bne     _lck_rw_try_lock_shared
#else
    str     r1, [r0]
    msr     cpsr_cf, r9
#endif
    mov     r0, #1
    bx      lr
rwtlsopt:
    ands    r2, r1, #0x8000
    bne     rwtlsfail
    LoadConstantToReg(0xFFFF0000, r3)
    ands    r2, r1, r3
    bne     rwtlsloopres
rwtlsfail:
    mov     r0, #0
    bx      lr
rwtlspanic:
    mov     r2, r1
    mov     r1, r0
    adr     r0, rwtlsPanicString
    blx     _panic
rwtlsPanicString:
    .asciz  "lck_rw_try_lock_shared: lock (0x%08x, 0x%08x)"

/**
 * lck_rw_try_lock_exclusive
 */
EnterARM(lck_rw_try_lock_exclusive)
#ifdef NO_EXCLUSIVES
    mrs     r9, cpsr
    orr     r1, r9, #0xc0
    msr     cpsr_cf, r1
#endif
    ldrex   r1, [r0]
    ands    r2, r1, #1
    bne     rwtlepanic
    LoadConstantToReg(0xFFFF0009, r3)
    ands    r2, r1, r3
    bne     rwtlefail
    orr     r1, r1, #8
#ifndef NO_EXCLUSIVES
    strex   r2, r1, [r0]
    movs    r2, r2
    bne     _lck_rw_try_lock_exclusive
#else
    str     r1, [r0]
    msr     cpsr_cf, r9
#endif
    mov     r0, #1
    bx      lr
rwtlefail:
    mov     r0, #0
    bx      lr
rwtlepanic:
    mov     r2, r1
    mov     r1, r0
    adr     r0, rwtlePanicString
    blx     _panic
rwtlePanicString:
    .asciz  "lck_rw_try_lock_exclusive: lock (0x%08x, 0x%08x)"
    
/**
 * bitlock
 */
EnterARM(hw_lock_bit)
    mov     r12, #1
    mov     r12, r12, lsl r1
    LoadLockHardwareRegister(r9)
    IncrementPreemptLevel(r9, r2)
    ldrex    r3, [r0]
    orr     r1, r3, r12
    ands    r2, r12, r3
    streq   r1, [r0]
    bxeq    lr
hwlbitpanic:
    mov     r0, r1
    adr     r0, hwlbitPanicString
    blx     _panic
hwlbitPanicString:
    .asciz "hw_lock_bit: lock 0x%08x"

EnterARM(hw_unlock_bit)
    mov     r12, #1
    mov     r12, r12, lsl r1
    ldr     r3, [r0]
    bic     r3, r3, r12
    str     r3, [r0]
    b       __enable_preemption

LOAD_ADDR_GEN_DEF(thread_wakeup)
LOAD_ADDR_GEN_DEF(lck_rw_lock_shared_gen)
LOAD_ADDR_GEN_DEF(lck_rw_lock_exclusive_gen)
