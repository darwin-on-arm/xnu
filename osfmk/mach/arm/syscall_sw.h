/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#ifdef PRIVATE

#ifndef	_MACH_ARM_SYSCALL_SW_H_
#define _MACH_ARM_SYSCALL_SW_H_

#undef kernel_trap

#define mach_syscall0to4_int(num)			\
	mov	r12, #(num)			;	\
	swi	#0x80				;	\
	bx	lr

#define mach_syscall5_int(num)			\
	mov	r12, sp				;	\
	stmfd	sp!,{r4-r5}			;	\
	ldr	r4, [r12]			;	\
	mov	r12, #(num)			;	\
	swi	#0x80				;	\
	ldmfd	sp!,{r4-r5}			;	\
	bx	lr

#define mach_syscall_large_int(num, save_regs, arg_regs)\
	mov	r12, sp				;	\
	stmfd	sp!,{save_regs}			;	\
	ldmia	r12,{arg_regs}			;	\
	mov	r12, #(num)			;	\
	swi	#0x80				;	\
	ldmfd	sp!,{save_regs}			;	\
	bx	lr

#define COMMA ,

#define _mach_sys_0(num)	mach_syscall0to4_int(num)
#define _mach_sys_1(num)	mach_syscall0to4_int(num)
#define _mach_sys_2(num)	mach_syscall0to4_int(num)
#define _mach_sys_3(num)	mach_syscall0to4_int(num)
#define _mach_sys_4(num)	mach_syscall0to4_int(num)
#define _mach_sys_5(num)	mach_syscall5_int(num)
#define _mach_sys_6(num)	mach_syscall_large_int(num, r4-r5, r4-r5)
#define _mach_sys_7(num)	mach_syscall_large_int(num, r4-r6 COMMA r8, r4-r6)
#define _mach_sys_8(num)	mach_syscall_large_int(num, r4-r6 COMMA r8, r4-r6 COMMA r8)
#define _mach_sys_9(num)	mach_syscall_large_int(num, r4-r6 COMMA r8, r4-r6 COMMA r8)

/* select the appropriate syscall code, based on the number of arguments */
#define _mach_sys(num, args)	_mach_sys_##args(num)

#define kernel_trap(trap_name, trap_number, nargs) \
	.globl	_##trap_name			;	\
	.align 2				;	\
	.text 					;	\
_##trap_name:					;	\
	_mach_sys(trap_number, nargs)

#endif	/* _MACH_ARM_SYSCALL_SW_H_ */

#endif /* PRIVATE */
