/*
 * Copyright 2013, winocm. <rms@velocitylimitless.org>
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
 * PE semihosting bringup interface
 */

#include <arm/arch.h>
#include <arm/asm_help.h>

.code 16
.text
.align 4

/*
 * lol buffer
 */
_semihost_buffer:
    .space (8192), 0
_semihost_buffer_ptr:
    .long _semihost_buffer
_semihost_buffer_ptr_ptr:
    .long _semihost_buffer_ptr


/**
 * PE_semihost_write_char
 *
 * Writes character output to debugger standard out.
 */
EnterThumb(PE_semihost_write_char)
#ifdef BOARD_CONFIG_OMAP3530
    ldr     r1, _semihost_buffer_ptr
    strb    r0, [r1]
    add     r1, r1, #1
    ldr     r2, _semihost_buffer_ptr_ptr
    str     r1, [r2]
#endif
#ifdef BOARD_CONFIG_ARMPBA8
    ldr     r1, _semihost_buffer_ptr
    strb    r0, [r1]
    mov     r0, #0x03       // SYS_WRITEC
    svc     0xab
#endif
    bx      lr
