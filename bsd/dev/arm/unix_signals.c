/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1992 NeXT, Inc.
 *
 * HISTORY
 * 13 May 1992 ? at NeXT
 *	Created.
 */

#include <mach/mach_types.h>
#include <mach/exception.h>

#include <kern/thread.h>

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <sys/sysproto.h>
#include <sys/sysent.h>
#include <sys/ucontext.h>
#include <sys/wait.h>
#include <mach/thread_act.h>    /* for thread_abort_safely */
#include <mach/thread_status.h>

/*
 * machine_exception() performs MD translation
 * of a mach exception to a unix signal and code.
 */

boolean_t machine_exception(int exception, mach_exception_code_t code, __unused mach_exception_subcode_t subcode, int *unix_signal, mach_exception_code_t * unix_code)
{
    panic("machine_exception signal handling code not implemented, panicking for now...\n");

    switch (exception) {

    case EXC_BAD_ACCESS:
        *unix_signal = SIGSEGV;
        *unix_code = code;
        break;

    case EXC_BAD_INSTRUCTION:
        *unix_signal = SIGILL;
        *unix_code = code;
        break;

    case EXC_ARITHMETIC:
        *unix_signal = SIGFPE;
        *unix_code = code;
        break;

    case EXC_SOFTWARE:
        *unix_signal = SIGTRAP;
        *unix_code = code;

    default:
        return (FALSE);
    }

    return (TRUE);
}
