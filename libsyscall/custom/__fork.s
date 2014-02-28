/*
 * Copyright (c) 1999-2010 Apple Inc. All rights reserved.
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
/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	libc/ppc/sys/fork.s
 *
 * HISTORY
 * 18-Nov-92  Ben Fathi (benf@next.com)
 *	Created from M88K sources
 *
 * 11-Jan-92  Peter King (king@next.com)
 *	Created from M68K sources
 */
 
/*
 * All of the asm stubs in this file have been adjusted so the pre/post
 * fork handlers and dyld fixup are done in C inside Libc. As such, Libc
 * expects the __fork asm to fix up the return code to be -1, 0 or pid
 * and errno if needed.
 */

#include "SYS.h"

#if defined(__i386__)

LEAF(___fork, 0)
	subl  $28, %esp   // Align the stack, with 16 bytes of extra padding that we'll need

	movl 	$ SYS_fork,%eax; 	// code for fork -> eax
	UNIX_SYSCALL_TRAP		// do the system call
	jnc	L1			// jump if CF==0

	CALL_EXTERN(cerror)
	movl	$-1,%eax
	addl	$28, %esp   // restore the stack
	ret
	
L1:
	orl	%edx,%edx	// CF=OF=0,  ZF set if zero result	
	jz	L2		// parent, since r1 == 0 in parent, 1 in child
	
	//child here...
	xorl	%eax,%eax	// zero eax
	REG_TO_EXTERN(%eax, __current_pid);
L2:
	addl	$28, %esp   // restore the stack
	// parent ends up here skipping child portion
	ret

#elif defined(__x86_64__)

LEAF(___fork, 0)
	subq  $24, %rsp   // Align the stack, plus room for local storage

	movl 	$ SYSCALL_CONSTRUCT_UNIX(SYS_fork),%eax; // code for fork -> rax
	UNIX_SYSCALL_TRAP		// do the system call
	jnc	L1			// jump if CF==0

	CALL_EXTERN(cerror)
	movq	$-1, %rax
	addq	$24, %rsp   // restore the stack
	ret
	
L1:
	orl	%edx,%edx	// CF=OF=0,  ZF set if zero result	
	jz	L2		// parent, since r1 == 0 in parent, 1 in child
	
	//child here...
	xorq	%rax, %rax
	PICIFY(__current_pid)
	movl	%eax,(%r11)
L2:
	// parent ends up here skipping child portion
	addq	$24, %rsp   // restore the stack
	ret

#elif defined(__arm__)
	
	.globl	cerror
	MI_ENTRY_POINT(_fork)
	stmfd	sp!, {r4, r7, lr}
	add	r7, sp, #4
	mov	r1, #1					// prime results
	mov	r12, #SYS_fork
	swi	#SWI_SYSCALL				// make the syscall
	bcs	Lbotch					// error?
	cmp	r1, #0					// parent (r1=0) or child(r1=1)
	beq	Lparent

	//child here...
	MI_GET_ADDRESS(r3, __current_pid)
	mov	r0, #0
	str	r0, [r3]		// clear cached pid in child

#if 0
#if defined(__DYNAMIC__)
// Here on the child side of the fork we need to tell the dynamic linker that
// we have forked.  To do this we call __dyld_fork_child in the dyanmic
// linker.  But since we can't dynamicly bind anything until this is done we
// do this by using the private extern __dyld_func_lookup() function to get the
// address of __dyld_fork_child (the 'C' code equivlent):
//
//	_dyld_func_lookup("__dyld_fork_child", &address);
//	address();
//
	.cstring
	.align 2
LC0:
	.ascii "__dyld_fork_child\0"
.text
.align 2
	sub	sp, sp, #4				// allocate space for the address parameter
	mov	r1, sp					// get the address of the allocated space
	ldr	r0, LP0					// get the name of the function to look up
L0:	add	r0, pc, r0
	bl	__dyld_func_lookup
	mov	lr, pc
	ldr	pc, [sp], #4				// call __dyld_fork_child indirectly and pop
#endif
	mov	r0, #0
#endif
        ldmfd   sp!, {r4, r7, pc}

Lbotch:
	MI_CALL_EXTERNAL(cerror)			// jump here on error
	mov	r0,#-1					// set the error
	// fall thru
	
Lparent:	
	ldmfd   sp!, {r4, r7, pc}			// pop and return

	.align 2
#if 0
#if defined(__DYNAMIC__)
LP0:
	.long	LC0-(L0+8)
#endif
#endif

#else
#error Unsupported architecture
#endif
