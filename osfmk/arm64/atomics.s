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
 * ARM atomic hardware functions.
 */

#include <arm/arch.h>
#include <arm/asm_help.h>

/**
 * hw_compare_and_store
 *
 * Take the value in the destination, compare it to the old value, if equal
 * atomically write the new value to the address. 
 */
.globl _hw_compare_and_store
.globl _OSCompareAndSwap
_hw_compare_and_store:
_OSCompareAndSwap:
    mov     x4, x0
loop:
    ldr     w3, [x2]
    mov     x0, #0
    cmp     w3, w4
    b.ne    loop_exit
    b.eq    loop_store
    b       loop
loop_exit:
    ret     lr
loop_store:
    str     w1, [x2]
    mov     x0, #1
    ret     lr

/* The others. */
.align 6
.globl _hw_atomic_sub
.globl _hw_atomic_add
.globl _hw_atomic_and_noret
.globl _hw_atomic_or_noret
_hw_atomic_sub:
    mov     x2, x0
try_sub:
    ldr     x0, [x2]
    sub     x0, x0, x1
    str     x0, [x2]
    ret     lr 
_hw_atomic_add:
    mov     x2, x0
try_add:
    ldr     x0, [x2]
    add     x0, x0, x1
    str     x0, [x2]
    ret     lr 
_hw_atomic_or_noret:
    mov     x2, x0
try_or:
    ldr     x0, [x4]
    orr     x0, x0, x1
    str     x0, [x2]
    ret     lr 
_hw_atomic_and_noret:
    mov     x2, x0
try_and:
    ldr     x0, [x2]
    and     x0, x0, x1
    str     x0, [x2]
    ret     lr 

    