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
 * ARMv7 Tagged TLB support (ASID) for pmap.
 */

#include <mach_debug.h>
#include <debug.h>
#include <mach/vm_types.h>
#include <mach/vm_param.h>
#include <mach/thread_status.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/cpu_number.h>
#include <kern/thread.h>
#include <arm/pmap.h>
#include <arm/misc_protos.h>
#include <kern/ledger.h>
#include <kern/zalloc.h>
#include <kern/lock.h>
#include <kern/kalloc.h>
#include <vm/vm_protos.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <arm/cpu_capabilities.h>
#include <arm/arch.h>
#include <mach/branch_predicates.h>
#include <arm/mp.h>
#include <arm/cpufunc.h>
#include "proc_reg.h"

/*
 * asid (Process context identifier) aka tagged TLB support.
 * On processors with this feature, unless disabled via the -pmap_asid_disable
 * boot-arg, the following algorithm is in effect:
 * Each processor maintains an array of tag refcounts indexed by tag.
 * Each address space maintains an array of tags indexed by CPU number.
 * Each address space maintains a coherency vector, indexed by CPU
 * indicating that the TLB state for that address space has a pending
 * invalidation.
 * On a context switch, a refcounted tag is lazily assigned to the newly
 * dispatched (CPU, address space) tuple.
 * When an inactive address space is invalidated on a remote CPU, it is marked
 * for invalidation upon the next dispatch. Some invalidations are
 * also processed at the user/kernel boundary.
 * Provisions are made for the case where a CPU is overcommmitted, i.e.
 * more active address spaces exist than the number of logical tags
 * provided for by the processor architecture (currently 4096).
 * The algorithm assumes the processor remaps the logical tags
 * to physical TLB context IDs in an LRU fashion for efficiency. (DRK '10)
 *
 * asid support was originally used in x86_64, but has been adapted for use in
 * ARMv7 platforms.
 */

boolean_t pmap_asid_disabled = FALSE;

#define PMAP_INVALID ((pmap_t)0xDEAD7347)
#define PMAP_ASID_INVALID_ASID	(0xDEAD)
#define	PMAP_ASID_MAX_REFCOUNT (0xF0)
#define	PMAP_ASID_MIN_ASID (1)

uint32_t pmap_asid_ncpus = 0;

void pmap_asid_invalidate_all_cpus(pmap_t tpmap) {
    unsigned i;
    assert((sizeof(tpmap->pmap_asid_coherency_vector) >= real_ncpus) && (!(sizeof(tpmap->pmap_asid_coherency_vector) & 7)));
	for (i = 0; i < real_ncpus; i+=8) {
          *(uint64_t *)(uintptr_t)&tpmap->pmap_asid_coherency_vector[i] = (~0ULL);
    }
}

void pmap_asid_validate_current(void) {
    int	ccpu = cpu_number();
    volatile uint8_t *cptr = cpu_datap(ccpu)->cpu_pmap_asid_coherentp;
    assert(cptr == &(current_thread()->map->pmap->pmap_asid_coherency_vector[ccpu]));
    if (cptr) {
        *cptr = 0;
    }
}

void pmap_asid_invalidate_cpu(pmap_t tpmap, int ccpu) {
	tpmap->pmap_asid_coherency_vector[ccpu] = 0xFF;
}

void pmap_asid_validate_cpu(pmap_t tpmap, int ccpu) {
	tpmap->pmap_asid_coherency_vector[ccpu] = 0;
}

void pmap_asid_configure(void)
{
    int ccpu = cpu_number();

    kprintf("[ASID configuration start]\n");
    printf("PMAP: enabled ASID support\n");

    assert(ml_get_interrupts_enabled() == FALSE || get_preemption_level() != 0);
    if(!pmap_asid_disabled) {
        if (OSIncrementAtomic(&pmap_asid_ncpus) == machine_info.max_cpus) {
            kprintf("All ASID/asids enabled: real_ncpus: %d, pmap_asid_ncpus: %d\n", real_ncpus, pmap_asid_ncpus);
        }

        arm_tlb_flushID();

        cpu_datap(ccpu)->cpu_pmap_asid_coherentp =
            cpu_datap(ccpu)->cpu_pmap_asid_coherentp_kernel =
            &(kernel_pmap->pmap_asid_coherency_vector[ccpu]);
        cpu_datap(ccpu)->cpu_asid_refcounts[0] = 1;
    }
}

void pmap_asid_initialize(pmap_t p) {
    unsigned i;
    unsigned nc = sizeof(p->pmap_asid_cpus)/sizeof(asid_t);

    assert(nc >= real_ncpus);
    for (i = 0; i < nc; i++) {
        p->pmap_asid_cpus[i] = PMAP_ASID_INVALID_ASID;
    }
}

void pmap_asid_initialize_kernel(pmap_t p) {
    unsigned i;
    unsigned nc = sizeof(p->pmap_asid_cpus)/sizeof(asid_t);

    for (i = 0; i < nc; i++) {
        p->pmap_asid_cpus[i] = 0;
    }
}

asid_t  pmap_asid_allocate_asid(int ccpu) {
    int i;
    asid_ref_t  cur_min = 0xFF;
    uint32_t    cur_min_index = ~1;
    asid_ref_t  *cpu_asid_refcounts = &cpu_datap(ccpu)->cpu_asid_refcounts[0];
    asid_ref_t  old_count;

    if ((i = cpu_datap(ccpu)->cpu_asid_free_hint) != 0) {
        if (cpu_asid_refcounts[i] == 0) {
            (void)__sync_fetch_and_add(&cpu_asid_refcounts[i], 1);
            cpu_datap(ccpu)->cpu_asid_free_hint = 0;
            return i;
        }
    }
    /* Linear scan to discover free slot, with hint. Room for optimization
     * but with intelligent prefetchers this should be
     * adequately performant, as it is invoked
     * only on first dispatch of a new address space onto
     * a given processor. DRKTODO: use larger loads and
     * zero byte discovery -- any pattern != ~1 should
     * signify a free slot.
     */
    for (i = PMAP_ASID_MIN_ASID; i < PMAP_ASID_MAX_ASID; i++) {
        asid_ref_t cur_refcount = cpu_asid_refcounts[i];

        assert(cur_refcount < PMAP_ASID_MAX_REFCOUNT);

        if (cur_refcount == 0) {
            (void)__sync_fetch_and_add(&cpu_asid_refcounts[i], 1);
            return i;
        }
        else {
            if (cur_refcount < cur_min) {
                cur_min_index = i;
                cur_min = cur_refcount;
            }
        }
    }
    assert(cur_min_index > 0 && cur_min_index < PMAP_ASID_MAX_ASID);
    /* Consider "rebalancing" tags actively in highly oversubscribed cases
     * perhaps selecting tags with lower activity.
     */

    old_count = __sync_fetch_and_add(&cpu_asid_refcounts[cur_min_index], 1);
    assert(old_count < PMAP_ASID_MAX_REFCOUNT);
    return cur_min_index;
}

void    pmap_asid_deallocate_asid(int ccpu, pmap_t tpmap) {
    asid_t asid;
    pmap_t lp;
    asid_ref_t prior_count;

    asid = tpmap->pmap_asid_cpus[ccpu];
    assert(asid != PMAP_ASID_INVALID_ASID);
    if (asid == PMAP_ASID_INVALID_ASID)
        return;

    lp = cpu_datap(ccpu)->cpu_asid_last_pmap_dispatched[asid];
    assert(asid > 0 && asid < PMAP_ASID_MAX_ASID);
    assert(cpu_datap(ccpu)->cpu_asid_refcounts[asid] >= 1);

    if (lp == tpmap)
        (void)__sync_bool_compare_and_swap(&cpu_datap(ccpu)->cpu_asid_last_pmap_dispatched[asid], tpmap, PMAP_INVALID);

    if ((prior_count = __sync_fetch_and_sub(&cpu_datap(ccpu)->cpu_asid_refcounts[asid], 1)) == 1) {
            cpu_datap(ccpu)->cpu_asid_free_hint = asid;
    }
    assert(prior_count <= PMAP_ASID_MAX_REFCOUNT);
}

void    pmap_destroy_asid_sync(pmap_t p) {
    int i;
    assert(ml_get_interrupts_enabled() == FALSE || get_preemption_level() !=0);
    for (i = 0; i < PMAP_ASID_MAX_CPUS; i++)
        if (p->pmap_asid_cpus[i] != PMAP_ASID_INVALID_ASID)
            pmap_asid_deallocate_asid(i, p);
}

asid_t  asid_for_pmap_cpu_tuple(pmap_t pmap, int ccpu) {
    return pmap->pmap_asid_cpus[ccpu];
}

void    pmap_asid_activate(pmap_t tpmap, int ccpu) {
    asid_t      new_asid = tpmap->pmap_asid_cpus[ccpu];
    pmap_t      last_pmap;
    boolean_t   asid_conflict = FALSE, pending_flush = FALSE;

    if (__improbable(new_asid == PMAP_ASID_INVALID_ASID)) {
        new_asid = tpmap->pmap_asid_cpus[ccpu] = pmap_asid_allocate_asid(ccpu);
    }
    assert(new_asid != PMAP_ASID_INVALID_ASID);
    cpu_datap(ccpu)->cpu_active_asid = new_asid;

    pending_flush = (tpmap->pmap_asid_coherency_vector[ccpu] != 0);
    if (__probable(pending_flush == FALSE)) {
        last_pmap = cpu_datap(ccpu)->cpu_asid_last_pmap_dispatched[new_asid];
        asid_conflict = ((last_pmap != NULL) &&(tpmap != last_pmap));
    }
    if (__improbable(pending_flush || asid_conflict)) {
        pmap_asid_validate_cpu(tpmap, ccpu);
    }
    /* Consider making this a unique id */
    cpu_datap(ccpu)->cpu_asid_last_pmap_dispatched[new_asid] = tpmap;

    assert(new_asid < PMAP_ASID_MAX_ASID);
    assert(((tpmap ==  kernel_pmap) && new_asid == 0) || ((new_asid != PMAP_ASID_INVALID_ASID) && (new_asid != 0)));
    tpmap->pm_asid = new_asid;

    if (!pending_flush) {
        pending_flush = (tpmap->pmap_asid_coherency_vector[ccpu] != 0);
        if (__improbable(pending_flush != 0)) {
            pmap_asid_validate_cpu(tpmap, ccpu);
            tpmap->pm_asid = new_asid;
            kprintf("pending_flush not needed, handle this! new_asid: %x\n", new_asid);
        }
    }
    cpu_datap(ccpu)->cpu_pmap_asid_coherentp = &(tpmap->pmap_asid_coherency_vector[ccpu]);
}
