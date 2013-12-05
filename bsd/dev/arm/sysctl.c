/*
 * Copyright (c) 2003-2011 Apple Inc. All rights reserved.
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

#include <string.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

extern uint64_t pmap_pv_hashlist_walks;
extern uint64_t pmap_pv_hashlist_cnts;
extern uint32_t pmap_pv_hashlist_max;
uint32_t pmap_kernel_text_ps = PAGE_SIZE;
uint32_t pv_hashed_kern_low_water_mark = 100;

/*extern struct sysctl_oid_list sysctl__machdep_pmap_children;*/

SYSCTL_NODE(_machdep, OID_AUTO, pmap, CTLFLAG_RW|CTLFLAG_LOCKED, 0,
	"PMAP info");

SYSCTL_QUAD    (_machdep_pmap, OID_AUTO, hashwalks, CTLFLAG_RD | CTLFLAG_KERN | CTLFLAG_LOCKED, &pmap_pv_hashlist_walks, "");
SYSCTL_QUAD    (_machdep_pmap, OID_AUTO, hashcnts, CTLFLAG_RD | CTLFLAG_KERN | CTLFLAG_LOCKED, &pmap_pv_hashlist_cnts, "");
SYSCTL_INT     (_machdep_pmap, OID_AUTO, hashmax, CTLFLAG_RD | CTLFLAG_KERN | CTLFLAG_LOCKED, &pmap_pv_hashlist_max, 0, "");
SYSCTL_INT     (_machdep_pmap, OID_AUTO, kernel_text_ps, CTLFLAG_RD | CTLFLAG_KERN | CTLFLAG_LOCKED, &pmap_kernel_text_ps, 0, "");
SYSCTL_INT     (_machdep_pmap, OID_AUTO, kern_pv_reserve, CTLFLAG_RW | CTLFLAG_KERN | CTLFLAG_LOCKED, &pv_hashed_kern_low_water_mark, 0, "");

SYSCTL_NODE(_machdep, OID_AUTO, memmap, CTLFLAG_RD|CTLFLAG_LOCKED, NULL, "physical memory map");
