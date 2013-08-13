/*
 * OSAtomic operations.
 */

.code 32
.arm
.syntax unified

.align 4
.globl _OSCompareAndSwap64
_OSCompareAndSwap64:
    ldr     r12, [sp]
    stmfd   sp!,{r4,r5,lr}
_loop:
    /* ldrexd  r4, [r12] */
#ifdef BOARD_CONFIG_OMAP3530
    ldrd    r4, [r12]
#else
    .long 0xe1bc4f9f
#endif
    teq     r0, r4
    teqeq   r1, r5
    movne   r0, #0
    bne     ret
    /* strexd  r4, r2, [r12] */
#ifdef BOARD_CONFIG_OMAP3530
    strd    r2, [r12]
#else
    .long 0xe1ac4f92
#endif
    cmp     r3, #0
    bne     _loop
    mov     r0, #1
ret:
    ldmfd   sp!,{r4,r5,pc}

.align 4
.globl _OSAddAtomic64
_OSAddAtomic64:
    stmfd   sp!, {r4-r9,lr}
loop:
#ifdef BOARD_CONFIG_OMAP3530
    ldrd    r4, [r2]
#else
    ldrexd  r4, r5, [r2]
#endif
    adds    r8, r4, r0
    adc     r9, r5, r1
#ifdef BOARD_CONFIG_OMAP3530
    strd    r8, r9, [r2]
#else
    strexd  r3, r8, r9, [r2]
#endif
    cmp     r3, #0
    bne     loop
    mov     r0, r4
    mov     r1, r5
    ldmfd   sp!, {r4-r9,pc}

.code 16
.thumb_func _OSCompareAndSwapPtr
.align 4
.globl _OSCompareAndSwapPtr
_OSCompareAndSwapPtr:
    stmfd   sp!,{r7,lr}
    blx     _hw_compare_and_store
    ldmfd   sp!,{r7,lr}
    bx      lr

.align 4
.thumb_func _OSAddAtomicLong
.globl _OSAddAtomicLong
_OSAddAtomicLong:
    stmfd   sp!,{r7,lr}
    blx     _OSAddAtomic
    ldmfd   sp!,{r7,lr}
    bx      lr