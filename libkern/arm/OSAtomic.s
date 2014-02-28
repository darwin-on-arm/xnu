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
 * OSAtomic operations.
 */

#include <arm/arch.h>

.code 32
.arm
.syntax unified

#if __ARM_ARCH == 6
#define NO_EXCLUSIVES 1
#endif

#if defined(BOARD_CONFIG_OMAP3530) || defined(BOARD_CONFIG_OMAP335X)
//#define NO_EXCLUSIVES 1
#endif

.align 4
.globl _OSCompareAndSwap64
_OSCompareAndSwap64:
    ldr     r12, [sp]
    stmfd   sp!,{r4,r5,lr}
_loop:
    /* ldrexd  r4, [r12] */
#ifdef NO_EXCLUSIVES
    ldrd    r4, [r12]
#else
    .long 0xe1bc4f9f
#endif
    teq     r0, r4
    teqeq   r1, r5
    movne   r0, #0
    bne     ret
    /* strexd  r4, r2, [r12] */
#ifdef NO_EXCLUSIVES
    strd    r2, [r12]
    mov     r4, #0
#else
    .long 0xe1ac4f92
#endif
    cmp     r4, #0
    bne     _loop
    mov     r0, #1
ret:
    ldmfd   sp!,{r4,r5,pc}

.align 4
.globl _OSAddAtomic64
_OSAddAtomic64:
    stmfd   sp!, {r4-r9,lr}
loop:
#ifdef NO_EXCLUSIVES
    ldrd    r4, [r2]
#else
    ldrexd  r4, r5, [r2]
#endif
    adds    r8, r4, r0
    adc     r9, r5, r1
#ifdef NO_EXCLUSIVES
    strd    r8, r9, [r2]
    mov     r3, #0
#else
    strexd  r3, r8, r9, [r2]
#endif
    cmp     r3, #0
    bne     loop
    mov     r0, r4
    mov     r1, r5
    ldmfd   sp!, {r4-r9,pc}

#ifndef _ARM_ARCH_6
.code 16
.thumb_func _OSCompareAndSwapPtr
#endif
.align 4
.globl _OSCompareAndSwapPtr
_OSCompareAndSwapPtr:
    stmfd   sp!,{r7,lr}
    blx     _hw_compare_and_store
    ldmfd   sp!,{r7,lr}
    bx      lr

#ifndef _ARM_ARCH_6
.align 4
.thumb_func _OSAddAtomicLong
#endif
.globl _OSAddAtomicLong
_OSAddAtomicLong:
    stmfd   sp!,{r7,lr}
    blx     _OSAddAtomic
    ldmfd   sp!,{r7,lr}
    bx      lr
