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

#if defined(BOARD_CONFIG_OMAP3530) || defined(BOARD_CONFIG_OMAP335X)
//#define ldrex    ldr
#endif

/**
 * hw_compare_and_store
 *
 * Take the value in the destination, compare it to the old value, if equal
 * atomically write the new value to the address. 
 */
EnterARM(hw_compare_and_store)
EnterARM(OSCompareAndSwap)
    mov     r12, r0
loop:
    ldrex   r3, [r2]
    mov     r0, #0
    cmp     r3, r12
    bxne    lr
    strex   r0, r1, [r2]
    eors    r0, r0, #1
    bxne    lr
    b       loop

/*
 * Atomic functions
 */
    AtomicMachine(sub, sub)
    AtomicMachine(add, add)
    AtomicMachine(and_noret, and)
    AtomicMachine(or_noret, orr)
