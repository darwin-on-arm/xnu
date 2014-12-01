/*
 * Copyright (c) 1998-2010 Apple Computer, Inc. All rights reserved.
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

#include <arm/asm_help.h>

/*
 * void
 * OSSpinLockUnlock(p)
 *      int *p;
 *
 * Unlock the lock pointed to by p.
 */

EnterARM(_OSSpinLockUnlock)
EnterARM_NoAlign(_IOSpinUnlock)
EnterARM_NoAlign(_ev_unlock)
    mov    r1, #0
    str    r1, [r0]
    bx     lr

/*
 * int
 * OSSpinLockTry(p)
 *      int *p;
 *
 * Try to lock p.  Return zero if not successful.
 */

EnterARM(_OSSpinLockTry)
EnterARM_NoAlign(_IOTrySpinLock)
EnterARM_NoAlign(_ev_try_lock)
    mov    r1, #0
    swp    r1, r1, [r0]
    cmp    r1, #0
    moveq  r0, #1
    movne  r0, #0
    bx     lr
