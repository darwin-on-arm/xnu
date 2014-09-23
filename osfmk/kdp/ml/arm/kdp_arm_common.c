/*
 * Copyright (c) 2008 Apple Inc. All rights reserved.
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

#include <libsa/types.h>

#include <mach/vm_map.h>
#include <mach/vm_param.h>
#include <mach/mach_types.h>
#include <mach/vm_attributes.h>

#include "kdp_arm_common.h"

#include <kdp/kdp_core.h>
#include <kdp/kdp_internal.h>

#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_protos.h>

#include <machine/pal_routines.h>
#include <libkern/kernel_mach_header.h>

struct kern_dump_preflight_context {
	uint32_t	region_count;
	uint64_t	dumpable_bytes;
};

struct kern_dump_send_context {
	uint64_t	hoffset;
	uint64_t	foffset;
	uint64_t	header_size;
};

int kern_dump_pmap_traverse_preflight_callback(vm_map_offset_t start, vm_map_offset_t end, void *context);
int kern_dump_pmap_traverse_send_callback(vm_map_offset_t start, vm_map_offset_t end, void *context);

int pmap_traverse_present_mappings(pmap_t pmap, vm_map_offset_t start, vm_map_offset_t end, pmap_traverse_callback callback, void *context)
{
	int ret = KERN_SUCCESS;
	vm_map_offset_t vcurstart, vcur;
	boolean_t lastvavalid = FALSE;

	/* Assumes pmap is locked, or being called from the kernel debugger */

	if (start > end) {
		return (KERN_INVALID_ARGUMENT);
	}

	if (start & PAGE_MASK_64) {
		return (KERN_INVALID_ARGUMENT);
	}

	for (vcur = vcurstart = start; (ret == KERN_SUCCESS) && (vcur < end); ) {
		ppnum_t ppn = pmap_find_phys(pmap, vcur);

		if (ppn != 0 && !pmap_valid_page(ppn)) {
			/* not something we want */
			ppn = 0;
		}

		if (ppn != 0) {
			if (!lastvavalid) {
				/* Start of a new virtual region */
				vcurstart = vcur;
				lastvavalid = TRUE;
			}
		} else {
			if (lastvavalid) {
				/* end of a virtual region */
				ret = callback(vcurstart, vcur, context);
				lastvavalid = FALSE;
			}
		}

		vcur += PAGE_SIZE;
	}

	if ((ret == KERN_SUCCESS)
		&& lastvavalid) {
		/* send previous run */

		ret = callback(vcurstart, vcur, context);
	}
	return (ret);
}

int kern_dump_pmap_traverse_preflight_callback(vm_map_offset_t start, vm_map_offset_t end, void *context)
{
	struct kern_dump_preflight_context *kdc = (struct kern_dump_preflight_context *)context;
	int ret = KERN_SUCCESS;

	kdc->region_count++;
	kdc->dumpable_bytes += (end - start);

	return (ret);
}

int kern_dump_pmap_traverse_send_callback(vm_map_offset_t start, vm_map_offset_t end, void *context)
{
	struct kern_dump_send_context *kdc = (struct kern_dump_send_context *)context;
	int ret = KERN_SUCCESS;
	kernel_segment_command_t sc;
	vm_size_t size = (vm_size_t)(end - start);

	if (kdc->hoffset + sizeof(sc) > kdc->header_size) {
		return (KERN_NO_SPACE);
	}

	/*
	 *	Fill in segment command structure.
	 */

	sc.cmd = LC_SEGMENT_KERNEL;
	sc.cmdsize = sizeof(kernel_segment_command_t);
	sc.segname[0] = 0;
	sc.vmaddr = (vm_address_t)start;
	sc.vmsize = size;
	sc.fileoff = (vm_address_t)kdc->foffset;
	sc.filesize = size;
	sc.maxprot = VM_PROT_READ;
	sc.initprot = VM_PROT_READ;
	sc.nsects = 0;
	sc.flags = 0;

	if ((ret = kdp_send_crashdump_pkt (KDP_SEEK, NULL, sizeof(kdc->hoffset) , &kdc->hoffset)) < 0) { 
		printf ("kdp_send_crashdump_pkt failed with error %d\n", ret);
		goto out;
	}

	if ((ret = kdp_send_crashdump_data (KDP_DATA, NULL, sizeof(kernel_segment_command_t) , (caddr_t) &sc)) < 0) {
		printf ("kdp_send_crashdump_data failed with error %d\n", ret);
		goto out;
	}

	kdc->hoffset += sizeof(kernel_segment_command_t);

	if ((ret = kdp_send_crashdump_pkt (KDP_SEEK, NULL, sizeof(kdc->foffset) , &kdc->foffset)) < 0) {
		printf ("kdp_send_crashdump_pkt failed with error %d\n", ret);
		goto out;
	}

	if ((ret = kdp_send_crashdump_data (KDP_DATA, NULL, (unsigned int)size, (caddr_t)(uintptr_t)start)) < 0)	{
		printf ("kdp_send_crashdump_data failed with error %d\n", ret);
		goto out;
	}

	kdc->foffset += size;

out:
	return (ret);
}

int kern_dump(void)
{
	int			ret;
	struct kern_dump_preflight_context kdc_preflight;
	struct kern_dump_send_context kdc_send;
	uint32_t	segment_count;
	size_t		command_size = 0, header_size = 0, tstate_size = 0;
	uint64_t	hoffset = 0, foffset = 0;
	kernel_mach_header_t	mh;


	kdc_preflight.region_count = 0;
	kdc_preflight.dumpable_bytes = 0;

	ret = pmap_traverse_present_mappings(kernel_pmap,
										 VM_MIN_KERNEL_AND_KEXT_ADDRESS,
										 VM_MAX_KERNEL_ADDRESS,
										 kern_dump_pmap_traverse_preflight_callback,
										 &kdc_preflight);
	if (ret) {
		printf("pmap traversal failed: %d\n", ret);
		return (ret);
	}

	printf("Kernel dump region count: %u\n", kdc_preflight.region_count);
	printf("Kernel dump byte count: %llu\n", kdc_preflight.dumpable_bytes);

	segment_count = kdc_preflight.region_count;

	tstate_size = sizeof(struct thread_command) + kern_collectth_state_size();

	command_size = segment_count * sizeof(kernel_segment_command_t) +
				tstate_size;

	header_size = command_size + sizeof(kernel_mach_header_t);

	/*
	 *	Set up Mach-O header for currently executing kernel.
	 */
	printf ("Generated Mach-O header size was %lu\n", header_size);

	mh.magic = _mh_execute_header.magic;
	mh.cputype = _mh_execute_header.cputype;;
	mh.cpusubtype = _mh_execute_header.cpusubtype;
	mh.filetype = MH_CORE;
	mh.ncmds = segment_count + 1 /* thread */;
	mh.sizeofcmds = (uint32_t)command_size;
	mh.flags = 0;

#if defined(__LP64__)
	mh.reserved = 0;
#endif

	hoffset = 0;	/* offset into header */
	foffset = (uint32_t)round_page(header_size);	/* offset into file */

	/* Transmit the Mach-O MH_CORE header, and seek forward past the
	 * area reserved for the segment and thread commands
	 * to begin data transmission
	 */
	if ((ret = kdp_send_crashdump_pkt (KDP_SEEK, NULL, sizeof(hoffset) , &hoffset)) < 0) {
		printf ("kdp_send_crashdump_pkt failed with error %d\n", ret);
		goto out;
	}
	if ((ret = kdp_send_crashdump_data (KDP_DATA, NULL, sizeof(kernel_mach_header_t), (caddr_t) &mh) < 0)) {
		printf ("kdp_send_crashdump_data failed with error %d\n", ret);
		goto out;
	}

	hoffset += sizeof(kernel_mach_header_t);

	if ((ret = kdp_send_crashdump_pkt (KDP_SEEK, NULL, sizeof(foffset) , &foffset) < 0)) {
		printf ("kdp_send_crashdump_pkt failed with error %d\n", ret);
		goto out;
	}

	printf ("Transmitting kernel state, please wait: ");

	kdc_send.hoffset = hoffset;
	kdc_send.foffset = foffset;
	kdc_send.header_size = header_size;

	ret = pmap_traverse_present_mappings(kernel_pmap,
										 VM_MIN_KERNEL_AND_KEXT_ADDRESS,
										 VM_MAX_KERNEL_ADDRESS,
										 kern_dump_pmap_traverse_send_callback,
										 &kdc_send);
	if (ret) {
		kprintf("pmap traversal failed: %d\n", ret);
		return (ret);
	}

	/* Reload mutated offsets */
	hoffset = kdc_send.hoffset;
	foffset = kdc_send.foffset;

	/*
	 * Now send out the LC_THREAD load command, with the thread information
	 * for the current activation.
	 */
	if (tstate_size > 0) {
		char tstate[tstate_size];

		kern_collectth_state (current_thread(), tstate, tstate_size);

		if ((ret = kdp_send_crashdump_pkt (KDP_SEEK, NULL, sizeof(hoffset), &hoffset)) < 0) {
			printf ("kdp_send_crashdump_pkt failed with error %d\n", ret);
			goto out;
		}

		if ((ret = kdp_send_crashdump_data (KDP_DATA, NULL, tstate_size, tstate)) < 0) {
			printf ("kdp_send_crashdump_data failed with error %d\n", ret);
			goto out;
		}

		hoffset += tstate_size;
	}

	/* last packet */
	if ((ret = kdp_send_crashdump_pkt (KDP_EOF, NULL, 0, ((void *) 0))) < 0)
	{
		printf ("kdp_send_crashdump_pkt failed with error %d\n", ret);
		goto out;
	}

out:
	return (ret);
}
