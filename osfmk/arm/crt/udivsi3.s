/*
 * Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
 *
 * This program and the accompanying materials
 * are licensed and made available under the terms and conditions of the BSD License
 * which accompanies this distribution.  The full text of the license may be found at
 * http://opensource.org/licenses/bsd-license.php
 *
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 */

#include <arm/asm_help.h>

EnterARM(__udivsi3)
  cmp  r1, #0
  cmpne  r0, #0
  stmfd  sp!, {r4, r5, r7, lr}
  add  r7, sp, #8
  beq  l2
  clz  r2, r1
  clz  r3, r0
  rsb  r3, r3, r2
  cmp  r3, #31
  bhi  l2
  ldmfdeq  sp!, {r4, r5, r7, pc}
  add  r5, r3, #1
  rsb  r3, r3, #31
  mov  lr, #0
  mov  r2, r0, asl r3
  mov  ip, r0, lsr r5
  mov  r4, lr
  b  l8
l9:
  mov  r0, r2, lsr #31
  orr  ip, r0, ip, asl #1
  orr  r2, r3, lr
  rsb  r3, ip, r1
  sub  r3, r3, #1
  and  r0, r1, r3, asr #31
  mov  lr, r3, lsr #31
  rsb  ip, r0, ip
  add  r4, r4, #1
l8:
  cmp  r4, r5
  mov  r3, r2, asl #1
  bne  l9
  orr  r0, r3, lr
  ldmfd  sp!, {r4, r5, r7, pc}
l2:
  mov  r0, #0
  ldmfd  sp!, {r4, r5, r7, pc}
