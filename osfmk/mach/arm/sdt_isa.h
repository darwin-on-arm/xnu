/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
 */
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _MACH_ARM_SDT_ISA_H
#define	_MACH_ARM_SDT_ISA_H

/* #pragma ident	"@(#)sdt.h	1.7	05/06/08 SMI" */

/*
 * Only define when testing.  This makes the calls into actual calls to
 * test functions.
 */
/* #define DTRACE_CALL_TEST */

#define DTRACE_STRINGIFY(s) #s
#define DTRACE_TOSTRING(s) DTRACE_STRINGIFY(s)

#define DTRACE_LABEL(p, n)									\
    ".align 4\n" \
	"__dtrace_probe$" DTRACE_TOSTRING(%=__LINE__) DTRACE_STRINGIFY(_##p##___##n) ":"	"\n\t"

#ifdef DTRACE_CALL_TEST

#define DTRACE_CALL(p,n)	\
	DTRACE_LABEL(p,n)	\
	DTRACE_CALL_INSN(p,n)

#else	/* !DTRACE_CALL_TEST */

#define DTRACE_CALL(p,n)	\
	DTRACE_LABEL(p,n)	\
	DTRACE_NOPS

#endif	/* !DTRACE_CALL_TEST */

#ifdef __arm__

#define DTRACE_NOPS			\
	"nop"			"\n\t"

#define DTRACE_CALL_INSN(p,n)						\
	"blx _dtracetest" DTRACE_STRINGIFY(_##p##_##n)	"\n\t"

#ifdef __thumb__
#define DTRACE_ALLOC_STACK(n)		\
	"add sp, #-" #n		"\n\t"
#define DTRACE_DEALLOC_STACK(n)		\
	"add sp, #" #n		"\n\t"
#else
#define DTRACE_ALLOC_STACK(n)		\
	"sub sp, sp, #" #n	"\n\t"
#define DTRACE_DEALLOC_STACK(n)		\
	"add sp, sp, #" #n	"\n\t"
#endif

#define ARG1_EXTENT	1
#define ARGS2_EXTENT	2
#define ARGS3_EXTENT	3
#define ARGS4_EXTENT	4
#define ARGS5_EXTENT	5
#define ARGS6_EXTENT	6
#define ARGS7_EXTENT	7
#define ARGS8_EXTENT	8
#define ARGS9_EXTENT	9
#define ARGS10_EXTENT	10	

#ifndef __LP64__

#define DTRACE_CALL0ARGS(provider, name)			\
	asm volatile (						\
		DTRACE_CALL(provider, name)			\
		"# eat trailing nl+tab from DTRACE_CALL"	\
		:						\
		:						\
	);

#define DTRACE_CALL1ARG(provider, name)				\
	asm volatile ("ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0"								\
	);

#define DTRACE_CALL2ARGS(provider, name)			\
	asm volatile ("ldr r1, [%0, #4]"						"\n\t"	\
		      "ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0", "r1"							\
	);

#define DTRACE_CALL3ARGS(provider, name)			\
	asm volatile ("ldr r2, [%0, #8]"						"\n\t"	\
		      "ldr r1, [%0, #4]"						"\n\t"	\
		      "ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0", "r1", "r2"						\
	);

#define DTRACE_CALL4ARGS(provider, name)			\
	asm volatile ("ldr r3, [%0, #12]"						"\n\t"	\
		      "ldr r2, [%0, #8]"						"\n\t"	\
		      "ldr r1, [%0, #4]"						"\n\t"	\
		      "ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0", "r1", "r2", "r3"					\
	);

#define DTRACE_CALL5ARGS(provider, name)			\
	asm volatile (										\
		      DTRACE_ALLOC_STACK(4)							\
		      "ldr r0, [%0, #16]"						"\n\t"	\
		      "str r0, [sp]"							"\n\t"	\
		      "ldr r3, [%0, #12]"						"\n\t"	\
		      "ldr r2, [%0, #8]"						"\n\t"	\
		      "ldr r1, [%0, #4]"						"\n\t"	\
		      "ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      DTRACE_DEALLOC_STACK(4)							\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0", "r1", "r2", "r3"					\
	);

#define DTRACE_CALL6ARGS(provider, name)			\
	asm volatile (										\
		      DTRACE_ALLOC_STACK(8)							\
		      "ldr r1, [%0, #20]"						"\n\t"	\
		      "ldr r0, [%0, #16]"						"\n\t"	\
		      "str r1, [sp, #4]"						"\n\t"	\
		      "str r0, [sp]"							"\n\t"	\
		      "ldr r3, [%0, #12]"						"\n\t"	\
		      "ldr r2, [%0, #8]"						"\n\t"	\
		      "ldr r1, [%0, #4]"						"\n\t"	\
		      "ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      DTRACE_DEALLOC_STACK(8)							\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0", "r1", "r2", "r3"					\
	);

#define DTRACE_CALL7ARGS(provider, name)			\
	asm volatile (										\
		      DTRACE_ALLOC_STACK(12)							\
		      "ldr r2, [%0, #24]"						"\n\t"	\
		      "ldr r1, [%0, #20]"						"\n\t"	\
		      "ldr r0, [%0, #16]"						"\n\t"	\
		      "str r2, [sp, #8]"						"\n\t"	\
		      "str r1, [sp, #4]"						"\n\t"	\
		      "str r0, [sp]"							"\n\t"	\
		      "ldr r3, [%0, #12]"						"\n\t"	\
		      "ldr r2, [%0, #8]"						"\n\t"	\
		      "ldr r1, [%0, #4]"						"\n\t"	\
		      "ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      DTRACE_DEALLOC_STACK(12)							\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0", "r1", "r2", "r3"					\
	);

#define DTRACE_CALL8ARGS(provider, name)			\
	asm volatile (										\
		      DTRACE_ALLOC_STACK(16)							\
		      "ldr r3, [%0, #28]"						"\n\t"	\
		      "ldr r2, [%0, #24]"						"\n\t"	\
		      "ldr r1, [%0, #20]"						"\n\t"	\
		      "ldr r0, [%0, #16]"						"\n\t"	\
		      "str r3, [sp, #12]"						"\n\t"	\
		      "str r2, [sp, #8]"						"\n\t"	\
		      "str r1, [sp, #4]"						"\n\t"	\
		      "str r0, [sp]"							"\n\t"	\
		      "ldr r3, [%0, #12]"						"\n\t"	\
		      "ldr r2, [%0, #8]"						"\n\t"	\
		      "ldr r1, [%0, #4]"						"\n\t"	\
		      "ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      DTRACE_DEALLOC_STACK(16)							\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0", "r1", "r2", "r3"					\
	);

#define DTRACE_CALL9ARGS(provider, name)			\
	asm volatile (										\
		      DTRACE_ALLOC_STACK(20)							\
		      "ldr r0, [%0, #32]"						"\n\t"	\
		      "str r0, [sp, #16]"						"\n\t"	\
		      "ldr r3, [%0, #28]"						"\n\t"	\
		      "ldr r2, [%0, #24]"						"\n\t"	\
		      "ldr r1, [%0, #20]"						"\n\t"	\
		      "ldr r0, [%0, #16]"						"\n\t"	\
		      "str r3, [sp, #12]"						"\n\t"	\
		      "str r2, [sp, #8]"						"\n\t"	\
		      "str r1, [sp, #4]"						"\n\t"	\
		      "str r0, [sp]"							"\n\t"	\
		      "ldr r3, [%0, #12]"						"\n\t"	\
		      "ldr r2, [%0, #8]"						"\n\t"	\
		      "ldr r1, [%0, #4]"						"\n\t"	\
		      "ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      DTRACE_DEALLOC_STACK(20)							\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0", "r1", "r2", "r3"					\
	);

#define DTRACE_CALL10ARGS(provider, name)			\
	asm volatile (										\
		      DTRACE_ALLOC_STACK(24)							\
		      "ldr r1, [%0, #36]"						"\n\t"	\
		      "ldr r0, [%0, #32]"						"\n\t"	\
		      "str r1, [sp, #20]"						"\n\t"	\
		      "str r0, [sp, #16]"						"\n\t"	\
		      "ldr r3, [%0, #28]"						"\n\t"	\
		      "ldr r2, [%0, #24]"						"\n\t"	\
		      "ldr r1, [%0, #20]"						"\n\t"	\
		      "ldr r0, [%0, #16]"						"\n\t"	\
		      "str r3, [sp, #12]"						"\n\t"	\
		      "str r2, [sp, #8]"						"\n\t"	\
		      "str r1, [sp, #4]"						"\n\t"	\
		      "str r0, [sp]"							"\n\t"	\
		      "ldr r3, [%0, #12]"						"\n\t"	\
		      "ldr r2, [%0, #8]"						"\n\t"	\
		      "ldr r1, [%0, #4]"						"\n\t"	\
		      "ldr r0, [%0]"							"\n\t"	\
		      DTRACE_CALL(provider, name)						\
		      DTRACE_DEALLOC_STACK(24)							\
		      :										\
		      : "l" (__dtrace_args)							\
		      : "memory", "r0", "r1", "r2", "r3"					\
	);

#else

/* We have no Dtrace on 64-bit. */
#define DTRACE_CALL0ARGS(provider, name)
#define DTRACE_CALL1ARG(provider, name)
#define DTRACE_CALL2ARGS(provider, name)
#define DTRACE_CALL3ARGS(provider, name)
#define DTRACE_CALL4ARGS(provider, name)
#define DTRACE_CALL5ARGS(provider, name)
#define DTRACE_CALL6ARGS(provider, name)
#define DTRACE_CALL7ARGS(provider, name)
#define DTRACE_CALL8ARGS(provider, name)
#define DTRACE_CALL9ARGS(provider, name)
#define DTRACE_CALL10ARGS(provider, name)


#endif

#endif /* __arm__ */

#endif	/* _MACH_ARM_SDT_ISA_H */

