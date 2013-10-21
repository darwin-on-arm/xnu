/*
 * Copyright (c) 2000-2008 Apple Inc. All rights reserved.
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
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

/*
 */
#ifdef	KERNEL_PRIVATE

#ifndef _ARM_MP_H_
#define _ARM_MP_H_

#define MAX_CPUS	32          /* (8*sizeof(long)) */

#ifndef	ASSEMBLER
#include <stdint.h>
#include <sys/cdefs.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/arm/thread_status.h>
#include <mach/vm_types.h>
#include <kern/lock.h>

extern unsigned int real_ncpus; /* real number of cpus */
extern unsigned int max_ncpus;  /* max number of cpus */
decl_simple_lock_data(extern, kdb_lock);    /* kdb lock     */

extern void console_init(void);
extern void *console_cpu_alloc(boolean_t boot_cpu);
extern void console_cpu_free(void *console_buf);

extern int kdb_cpu;             /* current cpu running kdb  */
extern int kdb_debug;
extern int kdb_active[];

extern volatile boolean_t mp_kdp_trap;
extern volatile boolean_t force_immediate_debugger_NMI;
extern volatile boolean_t pmap_tlb_flush_timeout;
extern volatile usimple_lock_t spinlock_timed_out;
extern volatile uint32_t spinlock_owner_cpu;

extern uint64_t LastDebuggerEntryAllowance;

extern boolean_t mp_recent_debugger_activity(void);
#endif

#endif

#endif                          /* KERNEL_PRIVATE */
