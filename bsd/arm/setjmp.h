/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
 */
/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	setjmp.h
 *
 *	Declaration of setjmp routines and data structures.
 */
#ifndef _BSD_ARM_SETJMP_H
#define _BSD_ARM_SETJMP_H

#include <sys/cdefs.h>
#include <machine/signal.h>

/*
 *	_JBLEN is number of ints required to save the following:
 *	r4-r8, r10, fp, sp, lr, sig  == 10 register_t sized
 *	s16-s31 == 16 register_t sized + 1 int for FSTMX
 *	1 extra int for future use
 */
#define _JBLEN		(10 + 16 + 2)
#define _JBLEN_MAX	_JBLEN

typedef int jmp_buf[_JBLEN];
typedef int sigjmp_buf[_JBLEN + 1];

__BEGIN_DECLS
extern int setjmp(jmp_buf env);
extern void longjmp(jmp_buf env, int val);

#ifndef _ANSI_SOURCE
int	_setjmp(jmp_buf env);
void	_longjmp(jmp_buf, int val);
int sigsetjmp(sigjmp_buf env, int val);
void siglongjmp(sigjmp_buf env, int val);
#endif /* _ANSI_SOURCE  */

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE)
void	longjmperror(void);
#endif /* neither ANSI nor POSIX */
__END_DECLS
#endif /* !_BSD_ARM_SETJMP_H */
