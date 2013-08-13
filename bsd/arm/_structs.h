/*
 * Copyright (c) 2006-2007 Apple Inc. All rights reserved.
 */
#include <sys/appleapiopts.h>

#ifdef __need_mcontext_t
#ifndef __need_struct_mcontext
#define __need_struct_mcontext
#endif /* __need_struct_mcontext */
#endif /* __need_mcontext_t */

#if defined(__need_struct_mcontext)
#include <mach/arm/_structs.h>
#endif /* __need_struct_mcontext */

#ifdef __need_struct_mcontext
#undef __need_struct_mcontext

#ifndef _STRUCT_MCONTEXT
#if __DARWIN_UNIX03
#define _STRUCT_MCONTEXT        struct __darwin_mcontext
_STRUCT_MCONTEXT
{
	_STRUCT_ARM_EXCEPTION_STATE	__es;
	_STRUCT_ARM_THREAD_STATE	__ss;
	_STRUCT_ARM_VFP_STATE		__fs;
};

#else /* !__DARWIN_UNIX03 */
#define _STRUCT_MCONTEXT        struct mcontext
_STRUCT_MCONTEXT
{
	_STRUCT_ARM_EXCEPTION_STATE	es;
	_STRUCT_ARM_THREAD_STATE	ss;
	_STRUCT_ARM_VFP_STATE		fs;
};

#endif /* __DARWIN_UNIX03 */
#endif /* _STRUCT_MCONTEXT */
#endif /* __need_struct_mcontext */

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#ifndef ARM_MCONTEXT_SIZE
#define ARM_MCONTEXT_SIZE       (ARM_THREAD_STATE_COUNT + ARM_VFP_STATE_COUNT + ARM_EXCEPTION_STATE_COUNT) * sizeof(int)
#endif /* ARM_MCONTEXT_SIZE */
#endif /* (_POSIX_C_SOURCE && !_DARWIN_C_SOURCE) */
