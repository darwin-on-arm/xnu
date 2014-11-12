/*
 * Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
 * Copyright (c) 2014, ARM Ltd. All rights reserved.<BR>
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

EnterARM(memset)
  cmp  r2, #0
  bxeq lr

  /*
   * args = 0, pretend = 0, frame = 0
   * frame_needed = 1, uses_anonymous_args = 0
   */

l10:
  strb  r1, [r0], #1
  subs  r2, r2, #1
  /* While size is not 0 */
  bne  l10
  bx   lr
