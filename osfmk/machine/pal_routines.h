/*
 * Copyright (c) 2009 Apple Inc. All rights reserved.
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
#ifndef _MACHINE_PAL_ROUTINES_H
#define _MACHINE_PAL_ROUTINES_H

#if defined (__i386__) || defined(__x86_64__)
#include "i386/pal_routines.h"
#elif defined (__arm__)

struct pal_cpu_data; /* Defined per-platform */
struct pal_pcb; /* Defined per-platform */
struct pal_apic_table; /* Defined per-platform */

/* serial / debug output routines */
extern int  pal_serial_init(void);
extern void pal_serial_putc(char);
extern int  pal_serial_getc(void);

/* Debug hook invoked in the page-fault path */
extern void pal_dbg_page_fault( thread_t thread, user_addr_t vadddr, 
				kern_return_t kr );

/* Set a task's name in the platform kernel debugger */
extern void pal_dbg_set_task_name( task_t task );

/* wind-back to the start of a system call */
void pal_syscall_restart(thread_t thread, arm_saved_state_t *state);

/* Hook for non-vfork exec */
void pal_execve_return(thread_t thread);

/* Called by thread_terminate_self() */
void pal_thread_terminate_self(thread_t thread);

/* Called by ast_check() */
void pal_ast_check(thread_t thread);

/* Called by sync_iss_to_iks */
extern void pal_get_kern_regs( arm_saved_state_t *state );

/* Called by load_machfile */
void pal_switch_pmap(thread_t, pmap_t, boolean_t);

/*
 * Platform-specific hlt/sti.
 */ 
extern void pal_hlt(void);
extern void pal_sti(void);
extern void pal_cli(void);

#else
#error architecture not supported
#endif

#endif /* _MACHINE_PAL_ROUTINES_H */
