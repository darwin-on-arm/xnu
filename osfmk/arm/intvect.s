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
 * ARM interrupt vector table
 */

#include <arm/arch.h>

/*
 * PLEASE DO NOT MESS WITH THE STRUCTURE OF THIS FILE. IT IS MAPPED
 * TO HIGH MEMORY AND IF YOU MESS ANYTHING UP, EXCEPTION HANDLERS
 * WILL NOT WORK. PLEASE DO NOT TOUCH THE ALIGNMENT OR ANYTHING.
 *
 * IF YOU ARE INTENT ON CHANGING THE lowGloVerification STRING
 * ALSO LOOK AT arm_vm_init.c. THANK YOU FOR LISTENING.
 */

.align 12
.text
.code 32
/*
 * I honestly wish llvm supported the "ldr rX, =var" syntax.
 */
.globl  _ExceptionVectorsBase
_ExceptionVectorsBase:
    ldr     pc, [pc, #24]       // reset
    ldr     pc, [pc, #24]       // undef
    ldr     pc, [pc, #24]       // swi
    ldr     pc, [pc, #24]       // prefetch
    ldr     pc, [pc, #24]       // data abort
    ldr     pc, [pc, #24]       // dataexc
    ldr     pc, [pc, #24]       // irq
    mov     pc, r9

_vectorTable:
    .long   _fleh_reset
    .long   _fleh_undef
    .long   _fleh_swi
    .long   _fleh_prefabt
    .long   _fleh_dataabt
    .long   _fleh_dataexc
    .long   _fleh_irq
    .long   0x0

lowGloVerification:
    .asciz  "Scolecit"

.org 4096
