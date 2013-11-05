/*
 * Copyright (c) 2000-2010 Apple Inc. All rights reserved.
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

#include <platforms.h>
#include <mach_ldebug.h>

/*
 * Pass field offsets to assembly code.
 */
#include <kern/ast.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <kern/lock.h>
#include <kern/locks.h>
#include <kern/host.h>
#include <kern/misc_protos.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <vm/vm_map.h>
#include <arm/pmap.h>
#include <arm/mp.h>
#include <arm/thread.h>
#include <arm/cpu_data.h>
#include <arm/cpu_capabilities.h>
#include <mach/arm/vm_param.h>
#include <mach/arm/thread_status.h>
#include <machine/commpage.h>
#include <pexpert/arm/boot.h>

#if	CONFIG_DTRACE
#define NEED_DTRACE_DEFS
#include <../bsd/sys/lockstat.h>
#endif

/*
 * genassym.c is used to produce an
 * assembly file which, intermingled with unuseful assembly code,
 * has all the necessary definitions emitted. This assembly file is
 * then postprocessed with sed to extract only these definitions
 * and thus the final assyms.s is created.
 *
 * This convoluted means is necessary since the structure alignment
 * and packing may be different between the host machine and the
 * target so we are forced into using the cross compiler to generate
 * the values, but we cannot run anything on the target machine.
 */

#undef	offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE)0)->MEMBER)

#if  0
#define DECLARE(SYM,VAL) \
	__asm("#DEFINITION#\t.set\t" SYM ",\t%0" : : "n" ((u_int)(VAL)))
#else
#define DECLARE(SYM,VAL) \
	__asm("#DEFINITION##define " SYM "\t%0" : : "n" ((u_int)(VAL)))
#endif

int main(int argc, char **argv);

int main(int argc, char **argv)
{

    DECLARE("AST_URGENT", AST_URGENT);
    DECLARE("AST_BSD", AST_BSD);

    DECLARE("MAX_CPUS", MAX_CPUS);

    /*
     * Mutex structure 
     */
    DECLARE("MUTEX_PTR", offsetof(lck_mtx_t *, lck_mtx_ptr));
    DECLARE("MUTEX_STATE", offsetof(lck_mtx_t *, lck_mtx_state));

    DECLARE("MUTEX_IND", LCK_MTX_TAG_INDIRECT);
    DECLARE("MUTEX_PTR", offsetof(lck_mtx_t *, lck_mtx_ptr));
    DECLARE("MUTEX_ASSERT_OWNED", LCK_MTX_ASSERT_OWNED);
    DECLARE("MUTEX_ASSERT_NOTOWNED", LCK_MTX_ASSERT_NOTOWNED);
    DECLARE("GRP_MTX_STAT_UTIL",
            offsetof(lck_grp_t *,
                     lck_grp_stat.lck_grp_mtx_stat.lck_grp_mtx_util_cnt));
    DECLARE("GRP_MTX_STAT_MISS",
            offsetof(lck_grp_t *,
                     lck_grp_stat.lck_grp_mtx_stat.lck_grp_mtx_miss_cnt));
    DECLARE("GRP_MTX_STAT_WAIT",
            offsetof(lck_grp_t *,
                     lck_grp_stat.lck_grp_mtx_stat.lck_grp_mtx_wait_cnt));

    /*
     * Per-mutex statistic element 
     */
    DECLARE("MTX_ACQ_TSC", offsetof(lck_mtx_ext_t *, lck_mtx_stat));

    /*
     * Mutex group statistics elements 
     */
    DECLARE("MUTEX_GRP", offsetof(lck_mtx_ext_t *, lck_mtx_grp));

    /*
     * Boot-args 
     */
    DECLARE("BOOT_ARGS_VIRTBASE", offsetof(boot_args *, virtBase));
    DECLARE("BOOT_ARGS_PHYSBASE", offsetof(boot_args *, physBase));
    DECLARE("BOOT_ARGS_MEMSIZE", offsetof(boot_args *, memSize));
    DECLARE("BOOT_ARGS_TOP_OF_KERNEL", offsetof(boot_args *, topOfKernelData));
    DECLARE("BOOT_ARGS_DEVICETREEP", offsetof(boot_args *, deviceTreeP));

    /*
     * The use of this field is somewhat at variance with the alias.
     */
    DECLARE("GRP_MTX_STAT_DIRECT_WAIT",
            offsetof(lck_grp_t *,
                     lck_grp_stat.lck_grp_mtx_stat.lck_grp_mtx_held_cnt));

    DECLARE("GRP_MTX_STAT_HELD_MAX",
            offsetof(lck_grp_t *,
                     lck_grp_stat.lck_grp_mtx_stat.lck_grp_mtx_held_max));
    /*
     * Reader writer lock types 
     */
    DECLARE("RW_SHARED", LCK_RW_TYPE_SHARED);
    DECLARE("RW_EXCL", LCK_RW_TYPE_EXCLUSIVE);

    DECLARE("TH_RECOVER", offsetof(thread_t, recover));
    DECLARE("TH_CONTINUATION", offsetof(thread_t, continuation));
    DECLARE("TH_KERNEL_STACK", offsetof(thread_t, kernel_stack));
    DECLARE("TH_MUTEX_COUNT", offsetof(thread_t, mutex_count));
    DECLARE("TH_WAS_PROMOTED_ON_WAKEUP",
            offsetof(thread_t, was_promoted_on_wakeup));

    DECLARE("TH_SYSCALLS_MACH", offsetof(thread_t, syscalls_mach));
    DECLARE("TH_SYSCALLS_UNIX", offsetof(thread_t, syscalls_unix));

    DECLARE("TASK_VTIMERS", offsetof(struct task *, vtimers));

    /*
     * These fields are being added on demand 
     */
    DECLARE("MACHINE_THREAD", offsetof(thread_t, machine));
    DECLARE("MACHINE_THREAD_PREEMPT_COUNT",
            offsetof(thread_t, machine.preempt_count));
    DECLARE("MACHINE_THREAD_CPU_DATA", offsetof(thread_t, machine.cpu_data));
    DECLARE("MACHINE_THREAD_CTHREAD_SELF",
            offsetof(thread_t, machine.cthread_self));

    DECLARE("CPU_PENDING_AST", offsetof(cpu_data_t *, cpu_pending_ast));
    DECLARE("CPU_PREEMPT_COUNT", offsetof(cpu_data_t *, cpu_preemption_level));

    DECLARE("CPU_PMAP", offsetof(cpu_data_t *, user_pmap));

    DECLARE("TH_TASK", offsetof(thread_t, task));
    DECLARE("TH_AST", offsetof(thread_t, ast));
    DECLARE("TH_MAP", offsetof(thread_t, map));
    DECLARE("TH_PCB_ISS", offsetof(thread_t, machine.iss));
    DECLARE("TH_PCB_USS", offsetof(thread_t, machine.uss));

#if NCOPY_WINDOWS > 0
    DECLARE("TH_COPYIO_STATE", offsetof(thread_t, machine.copyio_state));
    DECLARE("WINDOWS_CLEAN", WINDOWS_CLEAN);
#endif

    DECLARE("MAP_PMAP", offsetof(vm_map_t, pmap));

    DECLARE("VM_MIN_ADDRESS", VM_MIN_ADDRESS);
    DECLARE("VM_MAX_ADDRESS", VM_MAX_ADDRESS);
    DECLARE("KERNELBASE", VM_MIN_KERNEL_ADDRESS);
    DECLARE("LINEAR_KERNELBASE", LINEAR_KERNEL_ADDRESS);
    DECLARE("KERNEL_STACK_SIZE", KERNEL_STACK_SIZE);

    DECLARE("ASM_COMM_PAGE32_BASE_ADDRESS", _COMM_PAGE32_BASE_ADDRESS);
    DECLARE("ASM_COMM_PAGE32_START_ADDRESS", _COMM_PAGE32_START_ADDRESS);

    DECLARE("CPU_THIS", offsetof(cpu_data_t *, cpu_this));
    DECLARE("CPU_ACTIVE_THREAD", offsetof(cpu_data_t *, cpu_active_thread));
    DECLARE("CPU_ACTIVE_STACK", offsetof(cpu_data_t *, cpu_active_stack));
    DECLARE("CPU_KERNEL_STACK", offsetof(cpu_data_t *, cpu_kernel_stack));
    DECLARE("CPU_INT_STACK_TOP", offsetof(cpu_data_t *, cpu_int_stack_top));

#if	MACH_RT
    DECLARE("CPU_PREEMPTION_LEVEL",
            offsetof(cpu_data_t *, cpu_preemption_level));
#endif                          /* MACH_RT */
    DECLARE("CPU_PROCESSOR", offsetof(cpu_data_t *, cpu_processor));
    DECLARE("CPU_ONFAULT", offsetof(cpu_data_t *, cpu_onfault));

    /*
     *  usimple_lock fields
     */
    DECLARE("INTSTACK_SIZE", INTSTACK_SIZE);

    /*
     * values from kern/timer.h 
     */
    DECLARE("TIMER_TSTAMP", offsetof(struct timer *, tstamp));

#if	CONFIG_DTRACE
    DECLARE("LS_LCK_MTX_LOCK_ACQUIRE", LS_LCK_MTX_LOCK_ACQUIRE);
    DECLARE("LS_LCK_MTX_TRY_SPIN_LOCK_ACQUIRE",
            LS_LCK_MTX_TRY_SPIN_LOCK_ACQUIRE);
    DECLARE("LS_LCK_MTX_UNLOCK_RELEASE", LS_LCK_MTX_UNLOCK_RELEASE);
    DECLARE("LS_LCK_MTX_TRY_LOCK_ACQUIRE", LS_LCK_MTX_TRY_LOCK_ACQUIRE);
    DECLARE("LS_LCK_RW_LOCK_SHARED_ACQUIRE", LS_LCK_RW_LOCK_SHARED_ACQUIRE);
    DECLARE("LS_LCK_RW_DONE_RELEASE", LS_LCK_RW_DONE_RELEASE);
    DECLARE("LS_LCK_MTX_EXT_LOCK_ACQUIRE", LS_LCK_MTX_EXT_LOCK_ACQUIRE);
    DECLARE("LS_LCK_MTX_TRY_EXT_LOCK_ACQUIRE", LS_LCK_MTX_TRY_EXT_LOCK_ACQUIRE);
    DECLARE("LS_LCK_MTX_EXT_UNLOCK_RELEASE", LS_LCK_MTX_EXT_UNLOCK_RELEASE);
    DECLARE("LS_LCK_RW_LOCK_EXCL_ACQUIRE", LS_LCK_RW_LOCK_EXCL_ACQUIRE);
    DECLARE("LS_LCK_RW_LOCK_SHARED_TO_EXCL_UPGRADE",
            LS_LCK_RW_LOCK_SHARED_TO_EXCL_UPGRADE);
    DECLARE("LS_LCK_RW_TRY_LOCK_EXCL_ACQUIRE", LS_LCK_RW_TRY_LOCK_EXCL_ACQUIRE);
    DECLARE("LS_LCK_RW_TRY_LOCK_SHARED_ACQUIRE",
            LS_LCK_RW_TRY_LOCK_SHARED_ACQUIRE);
    DECLARE("LS_LCK_MTX_LOCK_SPIN_ACQUIRE", LS_LCK_MTX_LOCK_SPIN_ACQUIRE);
#endif

    return (0);
}
