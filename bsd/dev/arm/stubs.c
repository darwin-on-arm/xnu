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
 * Copyright (c) 1997 by Apple Computer, Inc., all rights reserved
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/ucred.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <vm/vm_map.h>
#include <machine/machine_routines.h>

/* XXX should be elsewhere (cpeak) */
extern void *get_bsduthreadarg(thread_t);
extern int *get_bsduthreadrval(thread_t);
extern void *find_user_regs(thread_t);

int copywithin(void *src, void *dst, size_t count)
{
    bcopy(src, dst, count);
    return 0;
}

void *get_bsduthreadarg(thread_t th)
{
    void *arg_ptr;
    struct uthread *ut;

    ut = get_bsdthread_info(th);

    if (ml_thread_is64bit(th) == TRUE)
        arg_ptr = (void *) saved_state64(find_user_regs(th));
    else
        arg_ptr = (void *) (ut->uu_arg);

    return (arg_ptr);
}

int *get_bsduthreadrval(thread_t th)
{
    struct uthread *ut;

    ut = get_bsdthread_info(th);
    return (&ut->uu_rval[0]);
}
