/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 *
 * HISTORY
 * 20-Apr-92 Bruce Martin (bmartin@next.com)
 * Created from M68K sources.
 */

#include "asm_help.h"
#include "setjmp.h"

/*
 * C library -- _setjmp, _longjmp
 *
 * _longjmp(a,v)
 * will generate a "return(v)" from the
 * last call to _setjmp(a)
 * by restoring registers from the stack.
 * The previous signal state is NOT restored.
 *
 */

EnterThumb(_setjmp)
    /* Store core registers */
    stmia  r0!, {r4-r8, r10-r11, sp, lr}

    mov    r0, #0
    bx     lr

EnterThumb(_longjmp)
    /* Restore core registers */
    ldmia  r0!, {r4-r8, r10-r11, sp, lr}

    /* Set return value */
    movs   r0, r1
    moveq  r0, #1
    bx     lr
