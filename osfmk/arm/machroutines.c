/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
 * Copyright 2013, winocm. <winocm@icloud.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 *   If you are going to use this software in any form that does not involve
 *   releasing the source to this project or improving it, let me know beforehand.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * ARM machine routines
 */

#include <arm/machine_routines.h>
#include <arm/io_map_entries.h>
#include <mach/processor.h>
#include <kern/processor.h>
#include <kern/machine.h>
#include <kern/cpu_data.h>
#include <kern/cpu_number.h>
#include <kern/thread.h>

#include <vm/pmap.h>

#include <arm/trap.h>
#include <mach/vm_param.h>
#include <arm/pmap.h>

#include <pexpert/arm/boot.h>

#define DBG(x...)	kprintf("DBG: " x)

uint32_t MutexSpin;

/**
 * ml_io_map
 *
 * Map memory mapped IO space to a virtual address.
 */
vm_offset_t ml_io_map(vm_offset_t phys_addr, vm_size_t size)
{
	return(io_map(phys_addr, size, VM_WIMG_IO));
}

/**
 * ml_static_malloc
 *
 * Unused.
 */
vm_offset_t ml_static_malloc(__unused vm_size_t size)
{
	return((vm_offset_t)NULL);
}

/**
 * ml_vtophys
 *
 * Shim for kvtophys.
 */
vm_offset_t ml_vtophys(vm_offset_t vaddr)
{
	return (vm_offset_t)kvtophys(vaddr);
}

/**
 * ml_init_interrupt
 *
 * Set interrupts enabled to true.
 */
void ml_init_interrupt(void)
{
	(void) ml_set_interrupts_enabled(TRUE);
}


/**
 * ml_cpu_up
 *
 * This is called from the machine-independent routine cpu_up()
 * to perform machine-dependent info updates. Defer to cpu_thread_init().
 */
void
ml_cpu_up(void)
{
	return;
}

/**
 * ml_cpu_down
 *
 * This is called from the machine-independent routine cpu_down()
 * to perform machine-dependent info updates.
 */
void
ml_cpu_down(void)
{
	return;
}

/**
 * ovbcopy
 *
 * Overlapped bcopy.
 */
void ovbcopy(const char *from, char *to, vm_size_t bytes)
{
	bcopy(from, to, bytes);
}

/**
 * bzero_phys
 *
 * Zero out a physical address. 
 */
void bzero_phys(addr64_t src64, uint32_t bytes)
{
#ifndef __LP64__
    bzero(phys_to_virt((uint32_t)src64), bytes);
#else
    bzero(phys_to_virt((uint64_t)src64), bytes);
#endif
}

/**
 * bcopy_phys
 *
 * The equivalent of bzero_phys but with additional copying features.
 * (patent pending)
 */
void bcopy_phys(addr64_t src64, addr64_t dst64, vm_size_t bytes)
{
#ifndef __LP64__
    bcopy(phys_to_virt((uint32_t)src64), phys_to_virt((uint32_t)dst64), bytes);
#else
    bcopy(phys_to_virt((uint64_t)src64), phys_to_virt((uint64_t)dst64), bytes);
#endif
    return;
}


/**
 * ml_init_lock_timeout
 */
void ml_init_lock_timeout(void)
{
	uint64_t	abstime;
	uint32_t	mtxspin;
	uint64_t	default_timeout_ns = NSEC_PER_SEC>>2;
	uint32_t	slto;
	uint32_t	prt;

	if (PE_parse_boot_argn("slto_us", &slto, sizeof (slto)))
		default_timeout_ns = slto * NSEC_PER_USEC;

	/* LockTimeOut is absolutetime, LockTimeOutTSC is in TSC ticks */
	nanoseconds_to_absolutetime(default_timeout_ns, &abstime);
	LockTimeOut = (uint32_t) abstime;
    
	if (PE_parse_boot_argn("mtxspin", &mtxspin, sizeof (mtxspin))) {
		if (mtxspin > USEC_PER_SEC>>4)
			mtxspin =  USEC_PER_SEC>>4;
		nanoseconds_to_absolutetime(mtxspin*NSEC_PER_USEC, &abstime);
	} else {
		nanoseconds_to_absolutetime(10*NSEC_PER_USEC, &abstime);
	}
	MutexSpin = (unsigned int)abstime;

}

/**
 * ml_get_max_cpus
 *
 * Since this is a uniprocessor system, we return 0.
 */
int ml_get_max_cpus(void)
{
    return 0;
}


void ml_init_max_cpus(unsigned long max_cpus)
{
    return;
}

void ml_install_interrupt_handler(void *nub, int source, void *target, IOInterruptHandler handler, void *refCon)
{
	boolean_t current_state;

	current_state = ml_get_interrupts_enabled();

	(void) ml_set_interrupts_enabled(current_state);

	initialize_screen(NULL, kPEAcquireScreen);
}

/**
 * ml_processor_register
 *
 * Register a processor with the system and add it to the master list.
 */
/*
 * initialize and bring up the CPU.
 */
kern_return_t ml_processor_register(
    cpu_id_t cpu_id,
    processor_t *processor_out,
	ipi_handler_t* ipi_handler)
{
    return KERN_SUCCESS;
}

/*
 * Stubbed.
 */
void ml_thread_policy(__unused thread_t thread,
                      __unused unsigned policy_id,
                      __unused unsigned policy_info)
{
    kprintf("ml_thread_policy is unimplemented\n");
}

int ml_get_max_affinity_sets(void) {
    return 1;
}

processor_set_t ml_affinity_to_pset(uint32_t affinity_num) 
{
	return PROCESSOR_SET_NULL;
}

vm_offset_t ml_static_ptovirt(vm_offset_t paddr) {
    return phys_to_virt(paddr);
}

void ml_get_power_state(boolean_t *icp, boolean_t *pidlep)
{
    *pidlep = FALSE;
}

void machine_track_platform_idle(boolean_t entry) {
    return;
}

void
ml_static_mfree(
        vm_offset_t vaddr,
        vm_size_t size)
{
	return;
}

/*
 *	kvtophys(addr)
 *
 *	Convert a kernel virtual address to a physical address
 */
addr64_t
kvtophys(vm_offset_t addr)
{
	pmap_paddr_t pa;
	pa = ((pmap_paddr_t)pmap_extract(kernel_pmap, addr));
    return (addr64_t)pa;
}

/*
 *	Routine:        ml_nofault_copy
 *	Function:	Perform a physical mode copy if the source and
 *			destination have valid translations in the kernel pmap.
 *			If translations are present, they are assumed to
 *			be wired; i.e. no attempt is made to guarantee that the
 *			translations obtained remained valid for
 *			the duration of the copy process.
 */

vm_size_t ml_nofault_copy(vm_offset_t virtsrc, vm_offset_t virtdst, vm_size_t size)
{
	/* BAD. */

    /* XXX fix this soon please to make KDP happy */
    ovbcopy(virtsrc, virtdst, size);

    return size;
}

/*
 * ml_phys_read_*.
 */

#define ml_phys_read_write_comb_gen(ret, cast, type, suffix)        \
    ret ml_phys_read ##suffix (type paddr) {                        \
        return (ret)(*(cast*)(phys_to_virt(paddr)));                \
    }                                                               \
    void ml_phys_write ##suffix (type paddr, ret data) {            \
        (*(volatile cast*)(phys_to_virt(paddr))) = (cast)data;      \
    }

ml_phys_read_write_comb_gen(unsigned int, unsigned char, vm_offset_t, _byte)
ml_phys_read_write_comb_gen(unsigned int, unsigned char, addr64_t, _byte_64)

ml_phys_read_write_comb_gen(unsigned int, unsigned short, vm_offset_t, _half)
ml_phys_read_write_comb_gen(unsigned int, unsigned short, addr64_t, _half_64)

ml_phys_read_write_comb_gen(unsigned int, unsigned int, vm_offset_t, )
ml_phys_read_write_comb_gen(unsigned int, unsigned int, addr64_t, _64)
ml_phys_read_write_comb_gen(unsigned int, unsigned int, vm_offset_t, _word)
ml_phys_read_write_comb_gen(unsigned int, unsigned int, addr64_t, _word_64)

ml_phys_read_write_comb_gen(unsigned long long, unsigned long long, vm_offset_t, _double)
ml_phys_read_write_comb_gen(unsigned long long, unsigned long long, addr64_t, _double_64)
