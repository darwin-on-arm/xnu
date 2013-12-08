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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas at 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * ARM physical memory map.
 *
 * Version 1.2b2, 'The Rewrite'.
 *
 * I'm sorry. This pmap sucks, but it sucks 'less' than the previous one did.
 *
 * Todo: fix pmap_nest, pmap_copy, pmap_unnest, pmap_enter_options, pmap_remove/pmap_remove_region
 *
 * And make pmap_create use an ASID bitmap too ifdef _ARM_ARCH_7
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
#include "cpufunc_armv7.h"
#include "proc_reg.h"

/*
 * The pv_head_table contains a 'trunk' of mappings for each physical
 * page, one mapping exists for each page. Pages that are mapped in
 * multiple pmaps (i.e: nested pmaps from say, the Dyld shared region)
 * have multiple 'pv_nexts'. These are considered leaf mappings. Code should
 * go through the leaf mappings if accessing/modifying page entries.
 *
 * -- With love, winocm.
 */

#define VM_MEM_WIRED            0x4
#define _1KB                    1 * 1024
#define _1MB                    1 * 1024 * _1KB

/** Core Structures */
typedef struct __pv_entry__ {
    struct __pv_entry__ *pv_next;   /* Next PV entry. */
    pmap_t pv_pmap;             /* Where does our mapping lie? */
    vm_offset_t pv_address_va;  /* Virtual Address for the mapping. */
    uint32_t pv_flags;          /* Pmap Flags */
} pv_entry, *pv_entry_t;

typedef enum {
    ARM_PAGE_TRANSLATION_FAULT = 0x00,  /* 0b00 */
    ARM_PAGE_PAGE_TABLE = 0x01, /* 0b01 */
    ARM_PAGE_SECTION = 0x02,    /* 0b10 */
    ARM_PAGE_MASK_VALUE = 0x03, /* 0b11 */
} pmap_arm_l1_page_types_t;

typedef enum {
    ARM_PTE_DESCRIPTOR_64K = 0x01,  /* 0b01 */
    ARM_PTE_DESCRIPTOR_4K = 0x02,   /* 0b1X */
} pmap_arm_l2_page_types_t;

extern vm_offset_t vm_kernel_stext;
extern vm_offset_t vm_kernel_etext;

/** Global variables */
boolean_t pmap_initialized = FALSE; /* Is the pmap system initialized? */
static struct vm_object pmap_object_store;  /* Storage object for the actual VM thing. */
vm_object_t pmap_object;        /* The real VM object. */
extern uint32_t first_avail, avail_end; /* End/begin of Managed RAM space. */
struct zone *pmap_zone;         /* Zone of pmap structures */
struct zone *pve_zone;          /* Pmap Virtual Entry zone. */
pv_entry_t pv_head_table;       /* Start of PV entries. */
static pmap_paddr_t avail_remaining;    /* Remaining avaialable pages. */
uint32_t virt_begin, virt_end;  /* Virtual Address Space. */
uint32_t avail_start, vm_first_phys;
vm_page_t commpage;
uint64_t pmap_nesting_size_min = 0x8000000;
uint64_t pmap_nesting_size_max = 0x8000000;

int allow_data_exec = 0;        /* no exec from data, embedded is hardcore like that */
int allow_stack_exec = 0;       /* No apps may execute from the stack by default */
int nx_enabled = 1;

/* THE kernel pmap. */
struct pmap kernel_pmap_store;
pmap_t kernel_pmap = &kernel_pmap_store;

/** Locking Primitives */
lock_t pmap_system_lock;
#define SPLVM(spl)          spl = splhigh();
#define SPLX(spl)           splx(spl);

#define PMAP_LOCK(pmap) {               \
    simple_lock(&(pmap)->lock);         \
}

#define PMAP_UNLOCK(pmap) {             \
    simple_unlock(&(pmap)->lock);       \
}

/** The Free List. */
pv_entry_t pv_free_list;        /* The free list should be populated when the pmaps are not locked. */
decl_simple_lock_data(, pv_free_list_lock);

#define PV_ALLOC(pv_e) {                \
    simple_lock(&pv_free_list_lock);    \
    if((pv_e = pv_free_list) != 0) {    \
        pv_free_list = pv_e->pv_next;   \
    }                                   \
    simple_unlock(&pv_free_list_lock);  \
}

#define PV_FREE(pv_e) {                 \
    simple_lock(&pv_free_list_lock);    \
    pv_e->pv_next = pv_free_list;       \
    pv_free_list = pv_e;                \
    simple_unlock(&pv_free_list_lock);  \
}

/*
 *  For each vm_page_t, there is a list of all currently
 *  valid virtual mappings of that page.  An entry is
 *  a pv_rooted_entry_t; the list is the pv_table.
 *
 *      N.B.  with the new combo rooted/hashed scheme it is
 *      only possibly to remove individual non-rooted entries
 *      if they are found via the hashed chains as there is no
 *      way to unlink the singly linked hashed entries if navigated to
 *      via the queue list off the rooted entries.  Think of it as
 *      hash/walk/pull, keeping track of the prev pointer while walking
 *      the singly linked hash list.  All of this is to save memory and
 *      keep both types of pv_entries as small as possible.
 */

/*

PV HASHING Changes - JK 1/2007

Pve's establish physical to virtual mappings.  These are used for aliasing of a 
physical page to (potentially many) virtual addresses within pmaps. In the previous 
implementation the structure of the pv_entries (each 16 bytes in size) was

typedef struct pv_entry {
    struct pv_entry_t    next;
    pmap_t                    pmap;
    vm_map_offset_t   va;
} *pv_entry_t;

An initial array of these is created at boot time, one per physical page of memory, 
indexed by the physical page number. Additionally, a pool of entries is created from a 
pv_zone to be used as needed by pmap_enter() when it is creating new mappings.  
Originally, we kept this pool around because the code in pmap_enter() was unable to 
block if it needed an entry and none were available - we'd panic.  Some time ago I 
restructured the pmap_enter() code so that for user pmaps it can block while zalloc'ing 
a pv structure and restart, removing a panic from the code (in the case of the kernel 
pmap we cannot block and still panic, so, we keep a separate hot pool for use only on 
kernel pmaps).  The pool has not been removed since there is a large performance gain 
keeping freed pv's around for reuse and not suffering the overhead of zalloc for every new pv we need.

As pmap_enter() created new mappings it linked the new pve's for them off the fixed 
pv array for that ppn (off the next pointer).  These pve's are accessed for several 
operations, one of them being address space teardown.  In that case, we basically do this

    for (every page/pte in the space) {
        calc pve_ptr from the ppn in the pte
        for (every pv in the list for the ppn) {
            if (this pv is for this pmap/vaddr) {
                do housekeeping
                unlink/free the pv
            }
        }
    }

The problem arose when we were running, say 8000 (or even 2000) apache or other processes 
and one or all terminate. The list hanging off each pv array entry could have thousands of 
entries.  We were continuously linearly searching each of these lists as we stepped through 
the address space we were tearing down.  Because of the locks we hold, likely taking a cache 
miss for each node,  and interrupt disabling for MP issues the system became completely 
unresponsive for many seconds while we did this.

Realizing that pve's are accessed in two distinct ways (linearly running the list by ppn 
for operations like pmap_page_protect and finding and modifying/removing a single pve as 
part of pmap_enter processing) has led to modifying the pve structures and databases.

There are now two types of pve structures.  A "rooted" structure which is basically the 
original structure accessed in an array by ppn, and a ''hashed'' structure accessed on a 
hash list via a hash of [pmap, vaddr].  These have been designed with the two goals of 
minimizing wired memory and making the lookup of a ppn faster.  Since a vast majority of 
pages in the system are not aliased and hence represented by a single pv entry I've kept 
the rooted entry size as small as possible because there is one of these dedicated for 
every physical page of memory.  The hashed pve's are larger due to the addition of the hash 
link and the ppn entry needed for matching while running the hash list to find the entry we 
are looking for.  This way, only systems that have lots of aliasing (like 2000+ httpd procs) 
will pay the extra memory price. Both structures have the same first three fields allowing 
some simplification in the code.

They have these shapes

typedef struct pv_rooted_entry {
        queue_head_t qlink;
        vm_map_offset_t va;
        pmap_t          pmap;
} *pv_rooted_entry_t;

typedef struct pv_hashed_entry {
  queue_head_t qlink;
  vm_map_offset_t va;
  pmap_t        pmap;
  ppnum_t ppn;
  struct pv_hashed_entry *nexth;
} *pv_hashed_entry_t;

The main flow difference is that the code is now aware of the rooted entry and the hashed 
entries.  Code that runs the pv list still starts with the rooted entry and then continues 
down the qlink onto the hashed entries.  Code that is looking up a specific pv entry first 
checks the rooted entry and then hashes and runs the hash list for the match. The hash list 
lengths are much smaller than the original pv lists that contained all aliases for the specific ppn.

*/

/*
 * OS level page bits.
 */
typedef enum {
    PMAP_OSPTE_TYPE_VALID = 0x0,
    PMAP_OSPTE_TYPE_WIRED = 0x1,
    PMAP_OSPTE_TYPE_REFERENCED = 0x2,
    PMAP_OSPTE_TYPE_MODIFIED = 0x4,
    PMAP_OSPTE_TYPE_NOENCRYPT = 0x8,
    PMAP_OSPTE_TYPE_NOCACHE = 0x10,
    PMAP_OSPTE_TYPE_PTA = 0x20,
} __internal_pmap_ospte_bits_t;

/*
 * The PV rooted hash stuff is from xnu-1228/osfmk/i386/pmap.c
 */

typedef struct pv_rooted_entry {    /* first three entries must match pv_hashed_entry_t */
    queue_head_t qlink;
    vm_map_offset_t va;         /* virtual address for mapping */
    pmap_t pmap;                /* pmap where mapping lies */
    uint32_t flags;             /* address flags */
} *pv_rooted_entry_t;

#define PV_ROOTED_ENTRY_NULL    ((pv_rooted_entry_t) 0)

pv_rooted_entry_t pv_head_hash_table;   /* array of entries, one per page */

typedef struct pv_hashed_entry {    /* first three entries must match pv_rooted_entry_t */
    queue_head_t qlink;
    vm_map_offset_t va;
    pmap_t pmap;
    ppnum_t ppn;
    struct pv_hashed_entry *nexth;
} *pv_hashed_entry_t;

#define PV_HASHED_ENTRY_NULL ((pv_hashed_entry_t)0)

#define NPVHASH 4095            /* MUST BE 2^N - 1 */
pv_hashed_entry_t *pv_hash_table;   /* hash lists */

uint32_t npvhash = 0;

/* #define PV_DEBUG 1   uncomment to enable some PV debugging code */
#define PV_DEBUG 1

#ifdef PV_DEBUG
#define CHK_NPVHASH() if(0 == npvhash) panic("npvhash uninitialized");
#else
#define CHK_NPVHASH()
#endif

/*
 *  pv_list entries are kept on a list that can only be accessed
 *  with the pmap system locked (at SPLVM, not in the cpus_active set).
 *  The list is refilled from the pv_hashed_list_zone if it becomes empty.
 */
pv_rooted_entry_t pv_hash_free_list = PV_ROOTED_ENTRY_NULL; /* free list at SPLVM */
pv_hashed_entry_t pv_hashed_free_list = PV_HASHED_ENTRY_NULL;
pv_hashed_entry_t pv_hashed_kern_free_list = PV_HASHED_ENTRY_NULL;
decl_simple_lock_data(, pv_hashed_free_list_lock)
    decl_simple_lock_data(, pv_hashed_kern_free_list_lock)
    decl_simple_lock_data(, pv_hash_table_lock)

int pv_free_count = 0;
int pv_hashed_free_count = 0;
int pv_kern_free_count = 0;
int pv_hashed_kern_free_count = 0;
#define PV_HASHED_LOW_WATER_MARK 5000
#define PV_HASHED_KERN_LOW_WATER_MARK 100
#define PV_HASHED_ALLOC_CHUNK 2000
#define PV_HASHED_KERN_ALLOC_CHUNK 50
thread_call_t mapping_adjust_call;
static thread_call_data_t mapping_adjust_call_data;
uint32_t mappingrecurse = 0;

#define PV_HASHED_ALLOC(pvh_e) { \
    simple_lock(&pv_hashed_free_list_lock); \
    if ((pvh_e = pv_hashed_free_list) != 0) { \
      pv_hashed_free_list = (pv_hashed_entry_t)pvh_e->qlink.next;   \
            pv_hashed_free_count--; \
            if (pv_hashed_free_count < PV_HASHED_LOW_WATER_MARK) \
              if (hw_compare_and_store(0,1,(u_int *)&mappingrecurse)) \
                thread_call_enter(mapping_adjust_call); \
    } \
    simple_unlock(&pv_hashed_free_list_lock); \
}

#define PV_HASHED_FREE_LIST(pvh_eh, pvh_et, pv_cnt) {   \
    simple_lock(&pv_hashed_free_list_lock); \
    pvh_et->qlink.next = (queue_entry_t)pv_hashed_free_list;    \
    pv_hashed_free_list = pvh_eh; \
        pv_hashed_free_count += pv_cnt; \
    simple_unlock(&pv_hashed_free_list_lock); \
}

#define PV_HASHED_KERN_ALLOC(pvh_e) { \
    simple_lock(&pv_hashed_kern_free_list_lock); \
    if ((pvh_e = pv_hashed_kern_free_list) != 0) { \
      pv_hashed_kern_free_list = (pv_hashed_entry_t)pvh_e->qlink.next;  \
            pv_hashed_kern_free_count--; \
            if (pv_hashed_kern_free_count < PV_HASHED_KERN_LOW_WATER_MARK) \
              if (hw_compare_and_store(0,1,(u_int *)&mappingrecurse)) \
                thread_call_enter(mapping_adjust_call); \
    } \
    simple_unlock(&pv_hashed_kern_free_list_lock); \
}

#define PV_HASHED_KERN_FREE_LIST(pvh_eh, pvh_et, pv_cnt) {   \
    simple_lock(&pv_hashed_kern_free_list_lock); \
    pvh_et->qlink.next = (queue_entry_t)pv_hashed_kern_free_list;   \
    pv_hashed_kern_free_list = pvh_eh; \
        pv_hashed_kern_free_count += pv_cnt; \
    simple_unlock(&pv_hashed_kern_free_list_lock); \
}

zone_t pv_hashed_list_zone;     /* zone of pv_hashed_entry structures */

#define pvhash(idx)         (&pv_hash_table[idx])

/** Useful Macros */
#define pa_index(pa)        (atop(pa))
#define pai_to_pvh(pai)     (&pv_head_hash_table[pai - atop(gPhysBase)])

/*
 *  Each entry in the pv_head_table is locked by a bit in the
 *  pv_lock_table.  The lock bits are accessed by the physical
 *  address of the page they lock.
 */

char *pv_lock_table;            /* pointer to array of bits */
#define pv_lock_table_size(n)   (((n)+BYTE_SIZE-1)/BYTE_SIZE)

char *pv_hash_lock_table;
#define pv_hash_lock_table_size(n)  (((n)+BYTE_SIZE-1)/BYTE_SIZE)

/*
 * Locking protocols
 */

#define lock_pvh_pai(pai)
#define unlock_pvh_pai(pai)

#define LOCK_PV_HASH(hash)
#define UNLOCK_PV_HASH(hash)

#define LOCK_PVH(index)     {          \
    mp_disable_preemption();           \
    lock_pvh_pai(index);               \
}

#define UNLOCK_PVH(index)  {      \
    unlock_pvh_pai(index);        \
    mp_enable_preemption();       \
}

/** ASID stuff */

#define KERNEL_ASID_PID 0

static vm_offset_t pm_asid_hint = KERNEL_ASID_PID + 1;
static u_long pm_asid_bitmap[256 / (sizeof(u_long) * 8)];

static u_long pm_asid_max = 255;
static u_long pm_asids_free = 254;  /* One is reserved by the Kernel ASID */

#define __BITMAP_SET(bm, n) \
    ((bm)[(n) / (8*sizeof(bm[0]))] |= 1LU << ((n) % (8*sizeof(bm[0]))))
#define __BITMAP_CLR(bm, n) \
    ((bm)[(n) / (8*sizeof(bm[0]))] &= ~(1LU << ((n) % (8*sizeof(bm[0])))))
#define __BITMAP_ISSET_P(bm, n) \
    (((bm)[(n) / (8*sizeof(bm[0]))] & (1LU << ((n) % (8*sizeof(bm[0]))))) != 0)

#define TLBINFO_ASID_MARK_USED(ti, asid) \
    __BITMAP_SET((ti), (asid))
#define TLBINFO_ASID_INUSE_P(ti, asid) \
    __BITMAP_ISSET_P((ti), (asid))

/** Template PTEs */

/*
 * Protection flags for various requested VM definitions, all of them are in here.
 * These are per ARMv6/ARM11JZF-S defintions.
 */
arm_l2_t arm_pte_prot_templates[] = {
    {.l2.nx = TRUE,.l2.ap = 0x00,.l2.apx = 0},  /* Privileged   ---     User    --- */
    {.l2.nx = TRUE,.l2.ap = 0x01,.l2.apx = 0},  /* Privileged   RW-     User    --- */
    {.l2.nx = TRUE,.l2.ap = 0x02,.l2.apx = 0},  /* Privileged   RW-     User    R-- */
    {.l2.nx = TRUE,.l2.ap = 0x03,.l2.apx = 0},  /* Privileged   RW-     User    RW- */

    {.l2.nx = FALSE,.l2.ap = 0x00,.l2.apx = 0}, /* Privileged   --X     User    --X */
    {.l2.nx = FALSE,.l2.ap = 0x01,.l2.apx = 0}, /* Privileged   RWX     User    --X */
    {.l2.nx = FALSE,.l2.ap = 0x02,.l2.apx = 0}, /* Privileged   RWX     User    R-X */
    {.l2.nx = FALSE,.l2.ap = 0x03,.l2.apx = 0}, /* Privileged   RWX     User    RWX */

    {.l2.nx = TRUE,.l2.ap = 0x00,.l2.apx = 1},  /* Privileged   ---     User    --- */
    {.l2.nx = TRUE,.l2.ap = 0x01,.l2.apx = 1},  /* Privileged   R--     User    --- */
    {.l2.nx = TRUE,.l2.ap = 0x02,.l2.apx = 1},  /* Privileged   R--     User    R-- */
    {.l2.nx = TRUE,.l2.ap = 0x03,.l2.apx = 1},  /* Privileged   R--     User    R-- */

    {.l2.nx = FALSE,.l2.ap = 0x00,.l2.apx = 1}, /* Privileged   --X     User    --X */
    {.l2.nx = FALSE,.l2.ap = 0x01,.l2.apx = 1}, /* Privileged   R-X     User    --X */
    {.l2.nx = FALSE,.l2.ap = 0x02,.l2.apx = 1}, /* Privileged   R-X     User    R-X */
    {.l2.nx = FALSE,.l2.ap = 0x03,.l2.apx = 1}, /* Privileged   R-X     User    R-X */
};

uint64_t pmap_pv_hashlist_walks = 0;
uint64_t pmap_pv_hashlist_cnts = 0;
uint32_t pmap_pv_hashlist_max = 0;

unsigned int inuse_ptepages_count = 0;
unsigned int bootstrap_wired_pages = 0;
int pt_fake_zone_index = -1;

uint32_t alloc_ptepages_count __attribute__ ((aligned(8))) = 0LL;   /* aligned for atomic access */

/* 
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!!!!!!! Make SURE this remains in sync with arm_pte_prot_templates. !!!!!!!!! 
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
typedef enum {
    ARM_PTE_PROT_KERNEL_NONE_USER_NONE,
    ARM_PTE_PROT_KERNEL_RW_USER_NONE,
    ARM_PTE_PROT_KERNEL_RW_USER_R,
    ARM_PTE_PROT_KERNEL_RW_USER_RW,
    ARM_PTE_PROT_KERNEL_X_USER_X,
    ARM_PTE_PROT_KERNEL_RWX_USER_X,
    ARM_PTE_PROT_KERNEL_RWX_USER_RX,
    ARM_PTE_PROT_KERNEL_RWX_USER_RWX,
    ARM_PTE_PROT_KERNEL_NONE_USER_NONE_2,
    ARM_PTE_PROT_KERNEL_R_USER_NONE,
    ARM_PTE_PROT_KERNEL_R_USER_R,
    ARM_PTE_PROT_KERNEL_R_USER_R_2,
    ARM_PTE_PROT_KERNEL_X_USER_X_2,
    ARM_PTE_PROT_KERNEL_RX_USER_X,
    ARM_PTE_PROT_KERNEL_RX_USER_X_2,
    ARM_PTE_PROT_KERNEL_RX_USER_RX,
    ARM_PTE_PROT_KERNEL_RX_USER_RX_2,
} arm_prot_pte_definitions;

/*
 * Type Extension bits for ARM V6 and V7 MMU
 *
 * TEX C B                                    Shared
 * 000 0 0  Strong order                      yes
 * 000 0 1  Shared device                     yes
 * 000 1 0  write through, no write alloc     S-bit
 * 000 1 1  write back, no write alloc        S-bit
 * 001 0 0  non-cacheable                     S-bit
 * 001 0 1  reserved
 * 001 1 0  reserved
 * 001 1 1  write back, write alloc           S-bit
 * 010 0 0  Non-shared device                 no
 * 010 0 1  reserved
 * 010 1 X  reserved
 * 011 X X  reserved
 * 1BB A A  BB for internal, AA for external  S-bit
 *
 *    BB    internal cache
 *    0 0   Non-cacheable non-buffered
 *    0 1   Write back, write alloc, buffered
 *    1 0   Write through, no write alloc, buffered
 *          (non-cacheable for MPCore)
 *    1 1   Write back, no write alloc, buffered
 *          (write back, write alloc for MPCore)
 *    
 *    AA    external cache 
 *    0 0   Non-cacheable non-buffered
 *    0 1   Write back, write alloc, buffered
 *    1 0   Write through, no write alloc, buffered
 *    1 1   Write back, no write alloc, buffered
 */
#define ARM_L2_C_BIT            0x00000004
#define ARM_L2_B_BIT            0x00000008
#define ARM_L2_4KB_TEX(x)       ((x & 0x7) << 6)  /* Type Extension */

#define ARM_CACHEBIT_NONE_NO_BUFFERED         0
#define ARM_CACHEBIT_WB_WA_BUFFERED           1
#define ARM_CACHEBIT_WT_NWA_BUFFERED          2
#define ARM_CACHEBIT_WB_NWA_BUFFERED          3

#define ARM_L2_TEX_000          0
#define ARM_L2_TEX_001          1
#define ARM_L2_TEX_010          2
#define ARM_L2_TEX_011          3
#define ARM_L2_TEX_100          4
#define ARM_L2_TEX_101          5
#define ARM_L2_TEX_110          6
#define ARM_L2_TEX_111          7

/** Functions */

extern int pt_fake_zone_index;
static inline void PMAP_ZINFO_PALLOC(pmap_t pmap, vm_size_t bytes)
{
    thread_t thr = current_thread();
    task_t task;
    zinfo_usage_t zinfo;

    pmap_ledger_credit(pmap, task_ledgers.tkm_private, bytes);

    if (pt_fake_zone_index != -1 && (task = thr->task) != NULL && (zinfo = task->tkm_zinfo) != NULL)
        OSAddAtomic64(bytes, (int64_t *) & zinfo[pt_fake_zone_index].alloc);
}

static inline void PMAP_ZINFO_PFREE(pmap_t pmap, vm_size_t bytes)
{
    thread_t thr = current_thread();
    task_t task;
    zinfo_usage_t zinfo;

    pmap_ledger_debit(pmap, task_ledgers.tkm_private, bytes);

    if (pt_fake_zone_index != -1 && (task = thr->task) != NULL && (zinfo = task->tkm_zinfo) != NULL)
        OSAddAtomic64(bytes, (int64_t *) & zinfo[pt_fake_zone_index].free);
}

static inline void PMAP_ZINFO_SALLOC(pmap_t pmap, vm_size_t bytes)
{
    pmap_ledger_credit(pmap, task_ledgers.tkm_shared, bytes);
}

static inline void PMAP_ZINFO_SFREE(pmap_t pmap, vm_size_t bytes)
{
    pmap_ledger_debit(pmap, task_ledgers.tkm_shared, bytes);
}

static inline uint32_t pvhashidx(pmap_t pmap, vm_map_offset_t va)
{
    return ((uint32_t) (uintptr_t) pmap ^ ((uint32_t) (va >> PAGE_SHIFT) & 0xFFFFFFFF)) & npvhash;
}

static inline void pv_hash_add(pv_hashed_entry_t pvh_e, pv_rooted_entry_t pv_h)
{
    pv_hashed_entry_t *hashp;
    int pvhash_idx;

    CHK_NPVHASH();
    pvhash_idx = pvhashidx(pvh_e->pmap, pvh_e->va);
    LOCK_PV_HASH(pvhash_idx);
    insque(&pvh_e->qlink, &pv_h->qlink);
    hashp = pvhash(pvhash_idx);
#if PV_DEBUG
    if (NULL == hashp)
        panic("pv_hash_add(%p) null hash bucket", pvh_e);
#endif
    pvh_e->nexth = *hashp;
    *hashp = pvh_e;
    UNLOCK_PV_HASH(pvhash_idx);
}

/*
 * unlinks the pv_hashed_entry_t pvh from the singly linked hash chain.
 * properly deals with the anchor.
 * must be called with the hash locked, does not unlock it
 */

static inline void pmap_pvh_unlink(pv_hashed_entry_t pvh)
{
    pv_hashed_entry_t curh;
    pv_hashed_entry_t *pprevh;
    int pvhash_idx;

    CHK_NPVHASH();
    pvhash_idx = pvhashidx(pvh->pmap, pvh->va);

    pprevh = pvhash(pvhash_idx);

#if PV_DEBUG
    if (NULL == *pprevh)
        panic("pvh_unlink null anchor");    /* JK DEBUG */
#endif
    curh = *pprevh;

    while (PV_HASHED_ENTRY_NULL != curh) {
        if (pvh == curh)
            break;
        pprevh = &curh->nexth;
        curh = curh->nexth;
    }
    if (PV_HASHED_ENTRY_NULL == curh)
        panic("pmap_pvh_unlink no pvh");
    *pprevh = pvh->nexth;
    return;
}

static inline void pv_hash_remove(pv_hashed_entry_t pvh_e)
{
    int pvhash_idx;

    CHK_NPVHASH();
    pvhash_idx = pvhashidx(pvh_e->pmap, pvh_e->va);
    LOCK_PV_HASH(pvhash_idx);
    remque(&pvh_e->qlink);
    pmap_pvh_unlink(pvh_e);
    UNLOCK_PV_HASH(pvhash_idx);
}

static inline boolean_t popcnt1(uint64_t distance)
{
    return ((distance & (distance - 1)) == 0);
}

/*
 * Routines to handle suppression of/recovery from some forms of pagetable corruption
 * incidents observed in the field. These can be either software induced (wild
 * stores to the mapwindows where applicable, use after free errors
 * (typically of pages addressed physically), mis-directed DMAs etc., or due
 * to DRAM/memory hierarchy/interconnect errors. Given the theoretical rarity of these errors,
 * the recording mechanism is deliberately not MP-safe. The overarching goal is to
 * still assert on potential software races, but attempt recovery from incidents
 * identifiable as occurring due to issues beyond the control of the pmap module.
 * The latter includes single-bit errors and malformed pagetable entries.
 * We currently limit ourselves to recovery/suppression of one incident per
 * PMAP_PAGETABLE_CORRUPTION_INTERVAL seconds, and details of the incident
 * are logged.
 * Assertions are not suppressed if kernel debugging is enabled. (DRK 09)
 */

typedef enum {
    PTE_VALID = 0x0,
    PTE_INVALID = 0x1,
    PTE_RSVD = 0x2,
    PTE_SUPERVISOR = 0x4,
    PTE_BITFLIP = 0x8,
    PV_BITFLIP = 0x10,
    PTE_INVALID_CACHEABILITY = 0x20
} pmap_pagetable_corruption_t;

typedef enum {
    ROOT_PRESENT = 0,
    ROOT_ABSENT = 1
} pmap_pv_assertion_t;

typedef enum {
    PMAP_ACTION_IGNORE = 0x0,
    PMAP_ACTION_ASSERT = 0x1,
    PMAP_ACTION_RETRY = 0x2,
    PMAP_ACTION_RETRY_RELOCK = 0x4
} pmap_pagetable_corruption_action_t;

#define PMAP_PAGETABLE_CORRUPTION_INTERVAL (6ULL * 3600ULL)
extern uint64_t pmap_pagetable_corruption_interval_abstime;

extern uint32_t pmap_pagetable_corruption_incidents;
#define PMAP_PAGETABLE_CORRUPTION_MAX_LOG (8)
typedef struct {
    pmap_pv_assertion_t incident;
    pmap_pagetable_corruption_t reason;
    pmap_pagetable_corruption_action_t action;
    pmap_t pmap;
    vm_map_offset_t vaddr;
    pt_entry_t pte;
    ppnum_t ppn;
    pmap_t pvpmap;
    vm_map_offset_t pvva;
    uint64_t abstime;
} pmap_pagetable_corruption_record_t;

pmap_pagetable_corruption_record_t pmap_pagetable_corruption_records[PMAP_PAGETABLE_CORRUPTION_MAX_LOG];
uint32_t pmap_pagetable_corruption_incidents;
uint64_t pmap_pagetable_corruption_last_abstime = (~(0ULL) >> 1);
uint64_t pmap_pagetable_corruption_interval_abstime;
thread_call_t pmap_pagetable_corruption_log_call;
static thread_call_data_t pmap_pagetable_corruption_log_call_data;
boolean_t pmap_pagetable_corruption_timeout = FALSE;

static inline void pmap_pagetable_corruption_log(pmap_pv_assertion_t incident, pmap_pagetable_corruption_t suppress_reason, pmap_pagetable_corruption_action_t action, pmap_t pmap, vm_map_offset_t vaddr, pt_entry_t * ptep, ppnum_t ppn, pmap_t pvpmap, vm_map_offset_t pvva)
{
    uint32_t pmap_pagetable_corruption_log_index;
    pmap_pagetable_corruption_log_index = pmap_pagetable_corruption_incidents++ % PMAP_PAGETABLE_CORRUPTION_MAX_LOG;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].incident = incident;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].reason = suppress_reason;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].action = action;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].pmap = pmap;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].vaddr = vaddr;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].pte = *ptep;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].ppn = ppn;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].pvpmap = pvpmap;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].pvva = pvva;
    pmap_pagetable_corruption_records[pmap_pagetable_corruption_log_index].abstime = mach_absolute_time();
    /*
     * Asynchronously log 
     */
    thread_call_enter(pmap_pagetable_corruption_log_call);
}

static inline pmap_pagetable_corruption_action_t pmap_classify_pagetable_corruption(pmap_t pmap, vm_map_offset_t vaddr, ppnum_t * ppnp, pt_entry_t * ptep, pmap_pv_assertion_t incident)
{
    pmap_pagetable_corruption_action_t action = PMAP_ACTION_ASSERT;
    pmap_pagetable_corruption_t suppress_reason = PTE_VALID;
    ppnum_t suppress_ppn = 0;
    pt_entry_t cpte = *ptep;
    ppnum_t cpn = pa_index((cpte) & L2_ADDR_MASK);
    ppnum_t ppn = *ppnp;
    pv_rooted_entry_t pv_h = pai_to_pvh((ppn));
    pv_rooted_entry_t pv_e = pv_h;
    uint32_t bitdex;
    pmap_t pvpmap = pv_h->pmap;
    vm_map_offset_t pvva = pv_h->va;
    boolean_t ppcd = FALSE;

    /*
     * Ideally, we'd consult the Mach VM here to definitively determine
     * * the nature of the mapping for this address space and address.
     * * As that would be a layering violation in this context, we
     * * use various heuristics to recover from single bit errors,
     * * malformed pagetable entries etc. These are not intended
     * * to be comprehensive.
     */

    /*
     * Correct potential single bit errors in either (but not both) element
     * of the PV
     */
    do {
        if ((popcnt1((uintptr_t) pv_e->pmap ^ (uintptr_t) pmap) && pv_e->va == vaddr) || (pv_e->pmap == pmap && popcnt1(pv_e->va ^ vaddr))) {
            pv_e->pmap = pmap;
            pv_e->va = vaddr;
            suppress_reason = PV_BITFLIP;
            action = PMAP_ACTION_RETRY;
            goto pmap_cpc_exit;
        }
    } while (((pv_e = (pv_rooted_entry_t) queue_next(&pv_e->qlink))) && (pv_e != pv_h));

    /*
     * Discover root entries with a Hamming
     * * distance of 1 from the supplied
     * * physical page frame.
     */
    for (bitdex = 0; bitdex < (sizeof(ppnum_t) << 3); bitdex++) {
        ppnum_t npn = cpn ^ (ppnum_t) (1ULL << bitdex);
        {
            pv_rooted_entry_t npv_h = pai_to_pvh((npn));
            if (npv_h->va == vaddr && npv_h->pmap == pmap) {
                suppress_reason = PTE_BITFLIP;
                suppress_ppn = npn;
                action = PMAP_ACTION_RETRY_RELOCK;
                UNLOCK_PVH((ppn));
                *ppnp = npn;
                goto pmap_cpc_exit;
            }
        }
    }

    if (pmap == kernel_pmap) {
        action = PMAP_ACTION_ASSERT;
        goto pmap_cpc_exit;
    }

    /*
     * Check for malformed/inconsistent entries 
     */

    if ((pmap != kernel_pmap) && ((cpte & L2_ACCESS_USER) == 0)) {
        action = PMAP_ACTION_IGNORE;
        suppress_reason = PTE_SUPERVISOR;
    }
 pmap_cpc_exit:
    PE_parse_boot_argn("-pmap_pagetable_corruption_deassert", &ppcd, sizeof(ppcd));

    if (debug_boot_arg && !ppcd) {
        action = PMAP_ACTION_ASSERT;
    }

    if ((mach_absolute_time() - pmap_pagetable_corruption_last_abstime) < pmap_pagetable_corruption_interval_abstime) {
        action = PMAP_ACTION_ASSERT;
        pmap_pagetable_corruption_timeout = TRUE;
    } else {
        pmap_pagetable_corruption_last_abstime = mach_absolute_time();
    }
    pmap_pagetable_corruption_log(incident, suppress_reason, action, pmap, vaddr, &cpte, *ppnp, pvpmap, pvva);
    return action;
}

/*
 * Remove pv list entry.
 * Called with pv_head_table entry locked.
 * Returns pv entry to be freed (or NULL).
 */
static inline __attribute__ ((always_inline)) pv_hashed_entry_t pmap_pv_remove(pmap_t pmap, vm_map_offset_t vaddr, ppnum_t * ppnp, pt_entry_t * pte)
{
    pv_hashed_entry_t pvh_e;
    pv_rooted_entry_t pv_h;
    pv_hashed_entry_t *pprevh;
    int pvhash_idx;
    uint32_t pv_cnt;
    ppnum_t ppn;

 pmap_pv_remove_retry:
    ppn = *ppnp;
    pvh_e = PV_HASHED_ENTRY_NULL;
    pv_h = pai_to_pvh((ppn));

    if (__improbable(pv_h->pmap == PMAP_NULL)) {
        pmap_pagetable_corruption_action_t pac = pmap_classify_pagetable_corruption(pmap, vaddr, ppnp, pte, ROOT_ABSENT);
        if (pac == PMAP_ACTION_IGNORE)
            goto pmap_pv_remove_exit;
        else if (pac == PMAP_ACTION_ASSERT)
            panic("pmap_pv_remove(%p,0x%x,0x%x, 0x%x, %p, %p): null pv_list!", pmap, vaddr, ppn, *pte, ppnp, pte);
        else if (pac == PMAP_ACTION_RETRY_RELOCK) {
            LOCK_PVH((*ppnp));
            goto pmap_pv_remove_retry;
        } else if (pac == PMAP_ACTION_RETRY)
            goto pmap_pv_remove_retry;
    }

    if (pv_h->va == vaddr && pv_h->pmap == pmap) {
        /*
         * Header is the pv_rooted_entry.
         * We can't free that. If there is a queued
         * entry after this one we remove that
         * from the ppn queue, we remove it from the hash chain
         * and copy it to the rooted entry. Then free it instead.
         */
        pvh_e = (pv_hashed_entry_t) queue_next(&pv_h->qlink);
        if (pv_h != (pv_rooted_entry_t) pvh_e) {
            /*
             * Entry queued to root, remove this from hash
             * and install as new root.
             */
            CHK_NPVHASH();
            pvhash_idx = pvhashidx(pvh_e->pmap, pvh_e->va);
            LOCK_PV_HASH(pvhash_idx);
            remque(&pvh_e->qlink);
            pprevh = pvhash(pvhash_idx);
            if (PV_HASHED_ENTRY_NULL == *pprevh) {
                panic("pmap_pv_remove(%p,0x%llx,0x%x): " "empty hash, removing rooted", pmap, vaddr, ppn);
            }
            pmap_pvh_unlink(pvh_e);
            UNLOCK_PV_HASH(pvhash_idx);
            pv_h->pmap = pvh_e->pmap;
            pv_h->va = pvh_e->va;   /* dispose of pvh_e */
        } else {
            /*
             * none queued after rooted 
             */
            pv_h->pmap = PMAP_NULL;
            pvh_e = PV_HASHED_ENTRY_NULL;
        }
    } else {
        /*
         * not removing rooted pv. find it on hash chain, remove from
         * ppn queue and hash chain and free it
         */
        CHK_NPVHASH();
        pvhash_idx = pvhashidx(pmap, vaddr);
        LOCK_PV_HASH(pvhash_idx);
        pprevh = pvhash(pvhash_idx);
        if (PV_HASHED_ENTRY_NULL == *pprevh) {
            panic("pmap_pv_remove(%p,0x%llx,0x%x, 0x%llx, %p): empty hash", pmap, vaddr, ppn, *pte, pte);
        }
        pvh_e = *pprevh;
        pmap_pv_hashlist_walks++;
        pv_cnt = 0;
        while (PV_HASHED_ENTRY_NULL != pvh_e) {
            pv_cnt++;
            if (pvh_e->pmap == pmap && pvh_e->va == vaddr && pvh_e->ppn == ppn)
                break;
            pprevh = &pvh_e->nexth;
            pvh_e = pvh_e->nexth;
        }

        if (PV_HASHED_ENTRY_NULL == pvh_e) {
            pmap_pagetable_corruption_action_t pac = pmap_classify_pagetable_corruption(pmap, vaddr, ppnp, pte, ROOT_PRESENT);

            if (pac == PMAP_ACTION_ASSERT)
                panic("pmap_pv_remove(%p, 0x%llx, 0x%x, 0x%llx, %p, %p): pv not on hash, head: %p, 0x%llx", pmap, vaddr, ppn, *pte, ppnp, pte, pv_h->pmap, pv_h->va);
            else {
                UNLOCK_PV_HASH(pvhash_idx);
                if (pac == PMAP_ACTION_RETRY_RELOCK) {
                    LOCK_PVH(ppn_to_pai(*ppnp));
                    goto pmap_pv_remove_retry;
                } else if (pac == PMAP_ACTION_RETRY) {
                    goto pmap_pv_remove_retry;
                } else if (pac == PMAP_ACTION_IGNORE) {
                    goto pmap_pv_remove_exit;
                }
            }
        }

        pmap_pv_hashlist_cnts += pv_cnt;
        if (pmap_pv_hashlist_max < pv_cnt)
            pmap_pv_hashlist_max = pv_cnt;
        *pprevh = pvh_e->nexth;
        remque(&pvh_e->qlink);
        UNLOCK_PV_HASH(pvhash_idx);
    }
 pmap_pv_remove_exit:
    return pvh_e;
}

__private_extern__ void pmap_pagetable_corruption_msg_log(int (*log_func) (const char *fmt, ...) __printflike(1, 2))
{
    if (pmap_pagetable_corruption_incidents > 0) {
        int i, e = MIN(pmap_pagetable_corruption_incidents, PMAP_PAGETABLE_CORRUPTION_MAX_LOG);
        (*log_func) ("%u pagetable corruption incident(s) detected, timeout: %u\n", pmap_pagetable_corruption_incidents, pmap_pagetable_corruption_timeout);
        for (i = 0; i < e; i++) {
            (*log_func) ("Incident 0x%x, reason: 0x%x, action: 0x%x, time: 0x%llx\n", pmap_pagetable_corruption_records[i].incident, pmap_pagetable_corruption_records[i].reason, pmap_pagetable_corruption_records[i].action, pmap_pagetable_corruption_records[i].abstime);
        }
    }
}

static inline void pmap_pagetable_corruption_log_setup(void)
{
    if (pmap_pagetable_corruption_log_call == NULL) {
        nanotime_to_absolutetime(20000, 0, &pmap_pagetable_corruption_interval_abstime);
        thread_call_setup(&pmap_pagetable_corruption_log_call_data, (thread_call_func_t) pmap_pagetable_corruption_msg_log, (thread_call_param_t) & printf);
        pmap_pagetable_corruption_log_call = &pmap_pagetable_corruption_log_call_data;
    }
}

/**
 * pmap_vm_prot_to_page_flags
 */
uint32_t pmap_vm_prot_to_page_flags(pmap_t pmap, vm_prot_t prot, int wired, int nx)
{
    arm_l2_t *current_l2 = &arm_pte_prot_templates[0];
    pt_entry_t pte = 0;

    /*
     * Pmaps other than the kernel one will always have user accessible pages.
     */
    if (pmap != kernel_pmap)
        pte |= L2_ACCESS_USER;
    pte |= L2_ACCESS_PRW;

    /*
     * Enforce Read-Write if necessary.
     */
    if (prot & VM_PROT_WRITE)
        pte &= ~(L2_ACCESS_APX);    /* APX-bit, RW? */
    else
        pte |= (L2_ACCESS_APX); /* APX-bit, R-? */

    /*
     * Enforce XN if necessary.
     */
    if (!(prot & VM_PROT_EXECUTE))
        pte |= L2_NX_BIT;       /* XN-bit, R?X */

    return pte;
}

/**
 * phys_attribute_clear and friends. These suck.
 */
void phys_attribute_clear(ppnum_t pn, int bits)
{
    int pai;
    pv_rooted_entry_t pv_h;

    assert(pn != vm_page_fictitious_addr);

    pv_h = pai_to_pvh(pn);
    pv_h->flags &= ~bits;

    return;
}

int phys_attribute_test(ppnum_t pn, int bits)
{
    int pai;
    pv_rooted_entry_t pv_h;

    assert(pn != vm_page_fictitious_addr);

    pv_h = pai_to_pvh(pn);
    if ((pv_h->flags & bits) == bits)
        return bits;

    return (pv_h->flags & bits);
}

void phys_attribute_set(ppnum_t pn, int bits)
{
    int pai;
    pv_rooted_entry_t pv_h;

    assert(pn != vm_page_fictitious_addr);

    pv_h = pai_to_pvh(pn);
    pv_h->flags |= bits;

    return;
}

/**
 * pmap_adjust_unnest_parameters
 *
 * Invoked by the Mach VM to determine the platform specific unnest region. This
 * is not used on ARM platforms.
 */
boolean_t pmap_adjust_unnest_parameters(pmap_t p, vm_map_offset_t * s, vm_map_offset_t * e)
{
    return FALSE;
}

/**
 * pmap_attributes
 *
 * Set/Get special memory attributes; Set/Get is not implemented.
 */
kern_return_t pmap_attribute(pmap_t pmap, vm_offset_t address, vm_size_t size, vm_machine_attribute_t atte, vm_machine_attribute_val_t * attrp)
{
    return KERN_INVALID_ADDRESS;
}

/**
 * pmap_attribute_cache_sync
 *
 * Flush appropriate cache based on page number sent.
 */
kern_return_t pmap_attribute_cache_sync(ppnum_t pn, vm_size_t size, vm_machine_attribute_t attr, vm_machine_attribute_val_t * attrp)
{
    Debugger("pmap_attribute_cache_sync");
    return KERN_SUCCESS;
}

/**
 * pmap_get_cache_attributes
 */
unsigned int pmap_get_cache_attributes(ppnum_t pn) {
    /* If the pmap subsystem isn't up, just assume writethrough cache. */
    if(!pmap_initialized)
        return (ARM_CACHEBIT_WT_NWA_BUFFERED << 2) | (ARM_L2_4KB_TEX(ARM_L2_TEX_100 | ARM_CACHEBIT_WT_NWA_BUFFERED));

    assert(pn != vm_page_fictitious_addr);
    pv_rooted_entry_t pv_h = pai_to_pvh(pn);
    assert(pv_h);

    unsigned int attr = pv_h->flags;
    unsigned int template = 0;
    
    if (attr & PMAP_OSPTE_TYPE_NOCACHE)
        /* No cache, strongly ordered memory. */
        template |= 0;
    else
        /* Assume writethrough, no write allocate for now. */
        template |= (ARM_CACHEBIT_WT_NWA_BUFFERED << 2) | (ARM_L2_4KB_TEX(ARM_L2_TEX_100 | ARM_CACHEBIT_WT_NWA_BUFFERED));

    return template;
}


/**
 * pmap_cache_attributes
 */
unsigned int pmap_cache_attributes(ppnum_t pn)
{
    if (!pmap_get_cache_attributes(pn) & ARM_L2_C_BIT)
        return (VM_WIMG_IO);
    else
        return (VM_WIMG_COPYBACK);
}

/**
 * pmap_clear_noencrypt
 */
void pmap_clear_noencrypt(ppnum_t pn)
{
    if (!pmap_initialized)
        return;
    phys_attribute_clear(pn, PMAP_OSPTE_TYPE_NOENCRYPT);
}

/**
 * pmap_is_noencrypt
 */
boolean_t pmap_is_noencrypt(ppnum_t pn)
{
    if (!pmap_initialized)
        return FALSE;
    return (phys_attribute_test(pn, PMAP_OSPTE_TYPE_NOENCRYPT));
}

/**
 * pmap_set_noencrypt
 */
void pmap_set_noencrypt(ppnum_t pn)
{
    if (!pmap_initialized)
        return;
    phys_attribute_set(pn, PMAP_OSPTE_TYPE_NOENCRYPT);
}


/*
 * Update cache attributes for all extant managed mappings.
 * Assumes PV for this page is locked, and that the page
 * is managed.
 */

static uint32_t cacheability_mask = ~((ARM_L2_TEX_011 << 2) | ARM_L2_4KB_TEX(ARM_L2_TEX_111));

void
pmap_update_cache_attributes_locked(ppnum_t pn, unsigned attributes) {
    pv_rooted_entry_t pv_h, pv_e;
    pv_hashed_entry_t pvh_e, nexth;
    vm_map_offset_t vaddr;
    pmap_t  pmap;
    pt_entry_t  *ptep;

    pv_h = pai_to_pvh(pn);
    /* 
     * TODO: translate the PHYS_* bits to PTE bits, while they're
     * currently identical, they may not remain so
     * Potential optimization (here and in page_protect),
     * parallel shootdowns, check for redundant
     * attribute modifications.
     */
    
    /*
     * Alter attributes on all mappings
     */
    if (pv_h->pmap != PMAP_NULL) {
        pv_e = pv_h;
        pvh_e = (pv_hashed_entry_t)pv_e;

        do {
            pmap = pv_e->pmap;
            vaddr = pv_e->va;
            ptep = pmap_pte(pmap, vaddr);
        
            if (0 == ptep)
                panic("pmap_update_cache_attributes_locked: Missing PTE, pmap: %p, pn: 0x%x vaddr: 0x%x kernel_pmap: %p", pmap, pn, vaddr, kernel_pmap);

            nexth = (pv_hashed_entry_t)queue_next(&pvh_e->qlink);

            /*
             * Update PTE.
             */
            pt_entry_t* cpte = (pt_entry_t*)ptep;
            *cpte &= cacheability_mask;
            *cpte |= attributes;
            armv7_tlb_flushID_SE(vaddr);

            pvh_e = nexth;
        } while ((pv_e = (pv_rooted_entry_t)nexth) != pv_h);
    }
}


/**
 * pmap_set_cache_attributes
 *
 * Set the specified cache attributes.
 */
void pmap_set_cache_attributes(ppnum_t pn, unsigned int cacheattr)
{
    unsigned int current, template = 0;
    int pai;

    if (cacheattr & VM_MEM_NOT_CACHEABLE) {
        /*
         * Template of 0 is non-cacheable, strongly ordered memory.
         */
        template &= cacheability_mask;
    } else {
        /*
         * Writethrough.
         */
        if(cacheattr == VM_WIMG_WTHRU)
            template |= (ARM_CACHEBIT_WT_NWA_BUFFERED << 2) | (ARM_L2_4KB_TEX(ARM_L2_TEX_100 | ARM_CACHEBIT_WT_NWA_BUFFERED));
        /*
         * Writecombine/copyback = writeback.
         */
        else if(cacheattr == VM_WIMG_WCOMB || cacheattr == VM_WIMG_COPYBACK)
            template |= (ARM_CACHEBIT_WB_WA_BUFFERED << 2) | (ARM_L2_4KB_TEX(ARM_L2_TEX_100 | ARM_CACHEBIT_WB_WA_BUFFERED));
    }

    /*
     * On MP systems, interrupts must be enabled.
     */
    if (processor_avail_count > 1 && !ml_get_interrupts_enabled())
        panic("interrupts must be enabled for pmap_set_cache_attributes");

    assert((pn != vm_page_fictitious_addr) && (pn != vm_page_guard_addr));

    LOCK_PVH(pai);
    pmap_update_cache_attributes_locked(pn, template);

    if(cacheattr & VM_MEM_NOT_CACHEABLE)
        phys_attribute_set(pn, PMAP_OSPTE_TYPE_NOCACHE);
    else 
        phys_attribute_clear(pn, PMAP_OSPTE_TYPE_NOCACHE);

    UNLOCK_PVH(pai);

    return;
}

/**
 * compute_pmap_gc_throttle
 *
 * Unused.
 */
void compute_pmap_gc_throttle(void *arg __unused)
{
    return;
}

/**
 * pmap_change_wiring
 *
 * Specify pageability.
 */
void pmap_change_wiring(pmap_t map, vm_map_offset_t va, boolean_t wired)
{
    pt_entry_t *pte;
    uint32_t pa;

    /*
     * Lock the pmap.
     */
    PMAP_LOCK(map);

    if ((pte = pmap_pte(map, va)) == (pt_entry_t *) 0)
        panic("pmap_change_wiring: pte missing");

    /*
     * Use FVTP to get the physical PPN. This will not work with the old
     * pmap_extract.
     */
    PMAP_UNLOCK(map);
    pa = pmap_extract(map, va);
    PMAP_LOCK(map);
    assert(pa);

    if (wired && phys_attribute_test(pa >> PAGE_SHIFT, PMAP_OSPTE_TYPE_WIRED)) {
        /*
         * We are wiring down the mapping.
         */
        pmap_ledger_credit(map, task_ledgers.wired_mem, PAGE_SIZE);
        OSAddAtomic(+1, &map->pm_stats.wired_count);
        phys_attribute_set(pa >> PAGE_SHIFT, PMAP_OSPTE_TYPE_WIRED);
    } else {
        /*
         * Unwiring the mapping.
         */
        assert(map->pm_stats.wired_count >= 1);
        OSAddAtomic(-1, &map->pm_stats.wired_count);
        phys_attribute_clear(pa >> PAGE_SHIFT, PMAP_OSPTE_TYPE_WIRED);
        pmap_ledger_debit(map, task_ledgers.wired_mem, PAGE_SIZE);
    }

    /*
     * Done, unlock the map.
     */
    PMAP_UNLOCK(map);
    return;
}

/**
 * pmap_tte
 */
vm_offset_t pmap_tte(pmap_t pmap, vm_offset_t virt)
{
    uint32_t tte_offset_begin;
    tte_offset_begin = pmap->pm_l1_virt;
    if ((tte_offset_begin + L1_SIZE) < addr_to_tte(pmap->pm_l1_virt, virt))
        panic("Translation table entry extends past L1 size (base: 0x%08X)", tte_offset_begin);
    return addr_to_tte(pmap->pm_l1_virt, virt);
}

/**
 * pmap_pte
 */
vm_offset_t pmap_pte(pmap_t pmap, vm_offset_t virt)
{
    uint32_t *tte_offset = (uint32_t *) pmap_tte(pmap, virt);
    uint32_t tte, pte, *ptep;

    /*
     * Get the translation-table entry. 
     */
    assert(tte_offset);
    tte = *tte_offset;

    /*
     * If the requested PTE entry is required is indeed the commonpage and
     * we are not the kernel pmap, quit.
     * 
     * This is because the TTBCR is set to 4kB, and all higher page table
     * address accesses will go to the kernel.
     */
    if (pmap != kernel_pmap && virt >= _COMM_PAGE_BASE_ADDRESS)
        return 0;

    /*
     * Verify it's not a section mapping. 
     */
    if ((tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_SECTION) {
        panic("Translation table entry is a section mapping (tte %x ttep %x ttebv %x)!\n", tte, tte_offset, pmap->pm_l1_virt);
    }

    /*
     * Clean the TTE bits off, get the address. 
     */
    pte = L1_PTE_ADDR(tte);
    if (!pte)
        return 0;

    /*
     * Return the virtual mapped PTE. 
     */
    ptep = (uint32_t *) ((phys_to_virt(pte) + pte_offset(virt)));

    return (ptep);
}

void mapping_free_prime(void)
{
    int i;
    pv_hashed_entry_t pvh_e;
    pv_hashed_entry_t pvh_eh;
    pv_hashed_entry_t pvh_et;
    int pv_cnt;

    pv_cnt = 0;
    pvh_eh = pvh_et = PV_HASHED_ENTRY_NULL;
    for (i = 0; i < (5 * PV_HASHED_ALLOC_CHUNK); i++) {
        pvh_e = (pv_hashed_entry_t) zalloc(pv_hashed_list_zone);

        pvh_e->qlink.next = (queue_entry_t) pvh_eh;
        pvh_eh = pvh_e;

        if (pvh_et == PV_HASHED_ENTRY_NULL)
            pvh_et = pvh_e;
        pv_cnt++;
    }
    PV_HASHED_FREE_LIST(pvh_eh, pvh_et, pv_cnt);

    pv_cnt = 0;
    pvh_eh = pvh_et = PV_HASHED_ENTRY_NULL;
    for (i = 0; i < PV_HASHED_KERN_ALLOC_CHUNK; i++) {
        pvh_e = (pv_hashed_entry_t) zalloc(pv_hashed_list_zone);

        pvh_e->qlink.next = (queue_entry_t) pvh_eh;
        pvh_eh = pvh_e;

        if (pvh_et == PV_HASHED_ENTRY_NULL)
            pvh_et = pvh_e;
        pv_cnt++;
    }
    PV_HASHED_KERN_FREE_LIST(pvh_eh, pvh_et, pv_cnt);

}

void mapping_adjust(void)
{
    pv_hashed_entry_t pvh_e;
    pv_hashed_entry_t pvh_eh;
    pv_hashed_entry_t pvh_et;
    int pv_cnt;
    int i;

    if (mapping_adjust_call == NULL) {
        pmap_pagetable_corruption_log_setup();
        thread_call_setup(&mapping_adjust_call_data, (thread_call_func_t) mapping_adjust, (thread_call_param_t) NULL);
        mapping_adjust_call = &mapping_adjust_call_data;
    }

    pv_cnt = 0;
    pvh_eh = pvh_et = PV_HASHED_ENTRY_NULL;
    if (pv_hashed_kern_free_count < PV_HASHED_KERN_LOW_WATER_MARK) {
        for (i = 0; i < PV_HASHED_KERN_ALLOC_CHUNK; i++) {
            pvh_e = (pv_hashed_entry_t) zalloc(pv_hashed_list_zone);

            pvh_e->qlink.next = (queue_entry_t) pvh_eh;
            pvh_eh = pvh_e;

            if (pvh_et == PV_HASHED_ENTRY_NULL)
                pvh_et = pvh_e;
            pv_cnt++;
        }
        PV_HASHED_KERN_FREE_LIST(pvh_eh, pvh_et, pv_cnt);
    }

    pv_cnt = 0;
    pvh_eh = pvh_et = PV_HASHED_ENTRY_NULL;
    if (pv_hashed_free_count < PV_HASHED_LOW_WATER_MARK) {
        for (i = 0; i < PV_HASHED_ALLOC_CHUNK; i++) {
            pvh_e = (pv_hashed_entry_t) zalloc(pv_hashed_list_zone);

            pvh_e->qlink.next = (queue_entry_t) pvh_eh;
            pvh_eh = pvh_e;

            if (pvh_et == PV_HASHED_ENTRY_NULL)
                pvh_et = pvh_e;
            pv_cnt++;
        }
        PV_HASHED_FREE_LIST(pvh_eh, pvh_et, pv_cnt);
    }
    mappingrecurse = 0;
}

/**
 * pmap_map
 *
 * Map specified virtual address range to a physical one.
 */
vm_offset_t pmap_map(vm_offset_t virt, vm_map_offset_t start_addr, vm_map_offset_t end_addr, vm_prot_t prot, unsigned int flags)
{
    int ps;

    ps = PAGE_SIZE;
    while (start_addr < end_addr) {
        pmap_enter(kernel_pmap, (vm_map_offset_t) virt, (start_addr >> PAGE_SHIFT), prot, flags, FALSE, TRUE);
        virt += ps;
        start_addr += ps;
    }
    return (virt);
}

/**
 * pmap_next_page_hi
 *
 * Allocate physical pages.
 */
boolean_t pmap_next_page_hi(ppnum_t * pnum)
{
    return pmap_next_page(pnum);
}

/**
 * pmap_zero_page
 *
 * Zero a physical page.
 */
void pmap_zero_page(ppnum_t p)
{
    assert(p != vm_page_fictitious_addr);

    /*
     * Make sure the page is valid. 
     */
    if (((p << PAGE_SHIFT) < avail_start) || ((p << PAGE_SHIFT) > avail_end))
        panic("pmap_zero_page: zeroing a non-managed page, ppnum %d", p);

    bzero(phys_to_virt(p << PAGE_SHIFT), PAGE_SIZE);
}

/**
 * pmap_clear_refmod
 *
 * Clears the referenced and modified bits as specified by the mask
 * of the specified physical page.
 */
void pmap_clear_refmod(ppnum_t pn, unsigned int mask)
{
    phys_attribute_clear(pn, mask);
}

/**
 * io_map
 *
 * Maps an IO region and returns its virtual address.
 */
vm_offset_t io_map(vm_offset_t phys_addr, vm_size_t size, unsigned int flags)
{
    vm_offset_t start;

    if (kernel_map == VM_MAP_NULL) {
        /*
         * VM is not initialized.  Grab memory.
         */
        start = virt_begin;
        virt_begin += round_page(size);

        (void) pmap_map_bd(start, phys_addr, phys_addr + round_page(size), VM_PROT_READ | VM_PROT_WRITE, flags);
    } else {
        (void) kmem_alloc_pageable(kernel_map, &start, round_page(size));
        (void) pmap_map(start, phys_addr, phys_addr + round_page(size), VM_PROT_READ | VM_PROT_WRITE, flags);
    }

    return (start);
}

vm_offset_t io_map_spec(vm_map_offset_t phys_addr, vm_size_t size, unsigned int flags)
{
    return (io_map(phys_addr, size, flags));
}

/**
 * pmap_next_page
 *
 * Allocate physical pages.
 */
boolean_t pmap_next_page(ppnum_t * addrp)
{
    if (first_avail >= avail_end) {
        kprintf("pmap_next_page: ran out of possible pages, last page was 0x%08x", first_avail);
        return FALSE;
    }

    *addrp = pa_index(first_avail);

    /*
     * We lost a page. 
     */
    first_avail += PAGE_SIZE;
    avail_remaining--;
    return TRUE;
}

/**
 * pmap_virtual_space
 *
 * Get virtual space parameters.
 */
void pmap_virtual_space(vm_offset_t * startp, vm_offset_t * endp)
{
    *startp = virt_begin;
    *endp = virt_end;
    kprintf("pmap_virtual_space: VM region 0x%08x - 0x%08x\n", virt_begin, virt_end);
}

/**
 * pmap_free_pages
 *
 * Return free page count.
 */
unsigned int pmap_free_pages(void)
{
    return avail_remaining;
}

/**
 * pmap_map_bd
 *
 * Enters a physical mapping. (Before the VM subsystem is up.)
 */
boolean_t pmap_map_bd(vm_offset_t virt, vm_map_offset_t start, vm_map_offset_t end, vm_prot_t prot, unsigned int flags)
{
    spl_t spl;

    /*
     * Verify the start and end are page aligned. 
     */
    assert(!(start & PAGE_MASK));
    assert(!(end & PAGE_MASK));

    /*
     * Disable interrupts and start mapping pages 
     */
    SPLVM(spl);

    /*
     * Write the PTEs to memory. 
     */
    uint32_t ptep = (uint32_t) (pmap_pte(kernel_pmap, virt));
    if (!ptep)
        panic("pmap_map_bd: Invalid kernel address");

    /*
     * Map the pages. 
     */
    l2_map_linear_range_no_cache(virt_to_phys(ptep), start, end);

    /*
     * Return. 
     */
    SPLX(spl);

    return TRUE;
}

/**
 * pmap_pageable
 */
void pmap_pageable(__unused pmap_t pmap, __unused vm_map_offset_t start, __unused vm_map_offset_t end, __unused boolean_t pageable)
{
    return;
}

/**
 * pmap_set_modify
 *
 * Set the modify bit on the specified physical page.
 */
void pmap_set_modify(ppnum_t pn)
{
    phys_attribute_set(pn, PMAP_OSPTE_TYPE_MODIFIED);
}

/**
 * pmap_clear_modify
 *
 * Clear the modify bits on the specified physical page.
 */
void pmap_clear_modify(ppnum_t pn)
{
    phys_attribute_clear(pn, PMAP_OSPTE_TYPE_MODIFIED);
}

/**
 * pmap_clear_reference
 *
 * Clear the reference bit on the specified physical page.
 */
void pmap_clear_reference(ppnum_t pn)
{
    phys_attribute_clear(pn, PMAP_OSPTE_TYPE_REFERENCED);
}

/**
 * pmap_set_reference
 *
 * Set the reference bit on the specified physical page.
 */
void pmap_set_reference(ppnum_t pn)
{
    phys_attribute_set(pn, PMAP_OSPTE_TYPE_REFERENCED);
}

/**
 * pmap_valid_page
 *
 * Is the page inside the managed zone?
 */
boolean_t pmap_valid_page(ppnum_t p)
{
    return (((p << PAGE_SHIFT) > avail_start)
            && ((p << PAGE_SHIFT) < avail_end));
}

/**
 * pmap_verify_free
 *
 * Verify that the page has no mappings.
 */
boolean_t pmap_verify_free(vm_offset_t phys)
{
    pv_rooted_entry_t pv_h;
    int pai;
    boolean_t result;

    assert(phys != vm_page_fictitious_addr);
    if (!pmap_initialized)
        return (TRUE);

    if (!pmap_valid_page(phys))
        return (FALSE);

    pv_h = pai_to_pvh(phys);
    result = (pv_h->pmap == PMAP_NULL);

    return (result);
}

/**
 * pmap_sync_page_data_phys
 * 
 * Invalidates all of the instruction cache on a physical page and
 * pushes any dirty data from the data cache for the same physical page
 */
void pmap_sync_page_data_phys(__unused ppnum_t pa)
{
    Debugger("pmap_sync_page_data_phys");
    return;
}

/**
 * pmap_sync_page_attributes_phys(ppnum_t pa)
 * 
 * Write back and invalidate all cachelines on a physical page.
 */
void pmap_sync_page_attributes_phys(ppnum_t pa)
{
    Debugger("pmap_sync_page_attributes_phys");
    return;
}

/*
 * Statistics routines
 */
int pmap_resident_max(pmap_t pmap)
{
    return ((pmap)->pm_stats.resident_max);
}

int pmap_resident_count(pmap_t pmap)
{
    return ((pmap)->pm_stats.resident_count);
}

/**
 * pmap_disable_NX
 *
 * Disable NX on a specified pmap.
 */
void pmap_disable_NX(pmap_t pmap)
{
    panic("pmap_disable_NX not implemented\n");
}

/**
 * pmap_zero_page
 *
 * pmap_copy_page copies the specified (machine independent)
 * page from physical address src to physical address dst.
 */
void pmap_copy_page(ppnum_t src, ppnum_t dst)
{
    ovbcopy(phys_to_virt(src << PAGE_SHIFT), phys_to_virt(dst << PAGE_SHIFT), PAGE_SIZE);
}

/**
 * pmap_copy_part_page
 *
 * Copies the specified (machine independent) pages.
 */
void pmap_copy_part_page(ppnum_t src, vm_offset_t src_offset, ppnum_t dst, vm_offset_t dst_offset, vm_size_t len)
{
    assert((((dst << PAGE_SHIFT) & PAGE_MASK) + dst_offset + len) <= PAGE_SIZE);
    assert((((src << PAGE_SHIFT) & PAGE_MASK) + src_offset + len) <= PAGE_SIZE);

    ovbcopy(phys_to_virt(src << PAGE_SHIFT) + src_offset, phys_to_virt(dst << PAGE_SHIFT) + src_offset, len);
}

/**
 * pmap_common_init
 *
 * Initialize common elements of pmaps.
 */
void pmap_common_init(pmap_t pmap)
{
    usimple_lock_init(&pmap->lock, 0);
    if (pmap->ledger)
        ledger_reference(pmap->ledger);
    pmap->pm_refcnt = 1;
    pmap->pm_nx = 0;
    pmap->pm_shared = FALSE;
    pmap->pm_stats.resident_count = 0;
    pmap->pm_stats.wired_count = 0;
}

/**
 * pmap_static_init
 *
 * Initialize the basic kernel pmap.
 */
void pmap_static_init(void)
{
    kdb_printf("pmap_static_init: Bootstrapping pmap\n");
    kernel_pmap->ledger = NULL;
    kernel_pmap->pm_asid = 0;
    kernel_pmap->pm_l1_size = 0x4000;   /* Cover 4*1024 TTEs */
    pmap_common_init(kernel_pmap);
    return;
}

/**
 * pmap_is_modified
 *
 * Return whether or not the specified physical page is modified
 * by any physical maps.
 */
boolean_t pmap_is_modified(vm_offset_t phys)
{
    return (phys_attribute_test(phys, PMAP_OSPTE_TYPE_MODIFIED));
}

/**
 * pmap_is_referenced
 *
 * Return whether or not the specified physical page is referenced
 * by any physical maps.
 */
boolean_t pmap_is_referenced(vm_offset_t phys)
{
    return (phys_attribute_test(phys, PMAP_OSPTE_TYPE_REFERENCED));
}

/**
 * pmap_list_resident_pages
 */
int pmap_list_resident_pages(pmap_t pmap, vm_offset_t * listp, int space)
{
    return 0;
}

/**
 * pmap_find_phys
 *
 * pmap_find_phys returns the (4K) physical page number containing a
 * given virtual address in a given pmap.
 */
ppnum_t pmap_find_phys(pmap_t pmap, addr64_t va)
{
    spl_t spl;
    uint32_t ptep, pte, ppn;

    /*
     * Raise priority level. 
     */
    disable_preemption();

#if 1
    /*
     * Get the PTE. 
     */
    ptep = (uint32_t) pmap_pte(pmap, (vm_offset_t) va);
    if (!ptep) {
        ppn = 0;
        goto out;
    }
    pte = (*(uint32_t *) (ptep));

    /*
     * Make sure it's a PTE. 
     */
    if (!((pte) & ARM_PTE_DESCRIPTOR_4K)) {
        ppn = 0;
        goto out;
    }

    ppn = pa_index(pte & L2_ADDR_MASK);
#else
    uint32_t virt = (va & L2_ADDR_MASK), par;
    boolean_t is_priv = (pmap == kernel_pmap) ? TRUE : FALSE;

    /*
     * TTBCR split means that commonpage is at 0x40000000, in kernel_pmap.
     */
    if (virt == _COMM_PAGE_BASE_ADDRESS) {
        ppn = 0;
        goto out;
    }

    /*
     * Fast VirtToPhys involves using the virtual address trnalsation
     * register as present in Cortex-A and ARM11 MPCore systems.
     *
     * Privileged reads are only done on the kernel PMAP versus user
     * pmaps getting user read/write state.
     *
     * The entire process should take much shorter compared to the
     * older pmap_extract, which fully walked the page tables. You can
     * still use the current behaviour however, by messing with
     * the MASTER files.
     *
     * I swear, I need more stupid sleep.
     */

    /*
     * Set the PAtoVA register and perform the operation.
     */
    if (is_priv)
        armreg_va2pa_pr_ns_write(virt);
    else
        armreg_va2pa_ur_ns_write(virt);

    /*
     * Wait for the instruction transaction to complete.
     */
    __asm__ __volatile__("isb sy");

    /*
     * See if the translation aborted, log any translation errors.
     */
    par = armreg_par_read();

    /*
     * Successful translation, we're done.
     */
    if (!(par & 1)) {
        uint32_t pa = par & L2_ADDR_MASK;
        ppn = pa_index(pa);
    } else {
        ppn = 0;
    }
#endif
 out:
    /*
     * Return. 
     */
    enable_preemption();
    return ppn;
}

/**
 * pmap_find_phys_fvtp
 *
 * pmap_find_phys returns the (4K) physical page number containing a
 * given virtual address in a given pmap. This is used for KDP purposes
 * only.
 */
ppnum_t pmap_find_phys_fvtp(pmap_t pmap, addr64_t va)
{
    uint32_t ptep, pte, ppn;
    uint32_t virt = (va & L2_ADDR_MASK), par;
    boolean_t is_priv = (pmap == kernel_pmap) ? TRUE : FALSE;

    /*
     * TTBCR split means that commonpage is at 0x40000000, in kernel_pmap.
     */
    if (virt == _COMM_PAGE_BASE_ADDRESS) {
        ppn = 0;
        goto out;
    }

    /*
     * Fast VirtToPhys involves using the virtual address trnalsation
     * register as present in Cortex-A and ARM11 MPCore systems.
     *
     * Privileged reads are only done on the kernel PMAP versus user
     * pmaps getting user read/write state.
     *
     * The entire process should take much shorter compared to the
     * older pmap_extract, which fully walked the page tables. You can
     * still use the current behaviour however, by messing with
     * the MASTER files.
     *
     * I swear, I need more stupid sleep.
     */

    /*
     * Set the PAtoVA register and perform the operation.
     */
    if (is_priv)
        armreg_va2pa_pr_ns_write(virt);
    else
        armreg_va2pa_ur_ns_write(virt);

    /*
     * Wait for the instruction transaction to complete.
     */
    __asm__ __volatile__("isb sy");

    /*
     * See if the translation aborted, log any translation errors.
     */
    par = armreg_par_read();

    /*
     * Successful translation, we're done.
     */
    if (!(par & 1)) {
        uint32_t pa = par & L2_ADDR_MASK;
        ppn = pa_index(pa);
    } else {
        ppn = 0;
    }
 out:
    /*
     * Return. 
     */
    enable_preemption();
    return ppn;
}

/**
 * pmap_switch
 *
 * Switch the current user pmap to a new one.
 */
void pmap_switch(pmap_t new_pmap)
{
    spl_t spl;

    /*
     * Raise priority level. 
     */
    SPLVM(spl);

    /*
     * Make sure it's not the kernel pmap. 
     */
    if (new_pmap == kernel_pmap)
        goto switch_return;

    /*
     * Switch it if needed. 
     */
    if (current_cpu_datap()->user_pmap == new_pmap) {
        goto switch_return;
    } else {
        current_cpu_datap()->user_pmap = new_pmap;
        armv7_set_context_id(new_pmap->pm_asid & 0xFF);
        armv7_context_switch(new_pmap->pm_l1_phys);
        armv7_tlb_flushID_ASID(new_pmap->pm_asid & 0xFF);
    }

    /*
     * Done. 
     */
 switch_return:
    SPLX(spl);
    return;
}

/**
 * pmap_map_block
 *
 * Map a (possibly) autogenned block
 */
void pmap_map_block(pmap_t pmap, addr64_t va, ppnum_t pa, uint32_t size, vm_prot_t prot, int attr, __unused unsigned int flags)
{
    uint32_t page;
    for (page = 0; page < size; page++) {
        pmap_enter(pmap, va, pa, prot, VM_PROT_NONE, attr, TRUE);
        va += PAGE_SIZE;
        pa++;
    }
}

/**
 * pmap_asid_init
 */
static inline void pmap_asid_init(void)
{
    pm_asid_bitmap[0] = (2 << KERNEL_ASID_PID) - 1;
}

/**
 * pmap_asid_alloc_fast
 *
 * Allocate a specified ASID for each proces. Each pmap has their own
 * individual ASID.
 */
#define __arraycount(__x) (sizeof(__x) / sizeof(__x[0]))
static inline void pmap_asid_alloc_fast(pmap_t map)
{
    /*
     * The pmap specified cannot be the kernel map, it already has its
     * own ASID allocated to it.
     */
    assert(map != kernel_pmap);
    assert(map->pm_asid == 0);
    assert(pm_asids_free > 0);
    assert(pm_asid_hint <= pm_asid_max);

    /*
     * Let's see if the hinted ASID is free. If not, search for a new one.
     */
    if (TLBINFO_ASID_INUSE_P(pm_asid_bitmap, pm_asid_hint)) {
        const size_t words = __arraycount(pm_asid_bitmap);
        const size_t nbpw = 8 * sizeof(pm_asid_bitmap[0]);
        for (size_t i = 0; i < pm_asid_hint / nbpw; i++) {
            assert(pm_asid_bitmap[i] == 0);
        }
        for (size_t i = pm_asid_hint / nbpw;; i++) {
            assert(i < words);
            /*
             * ffs wants to find the first bit set while we want
             * to find the first bit cleared.
             */
            u_long bits = ~pm_asid_bitmap[i];
            if (bits) {
                u_int n = 0;
                if ((bits & 0xffffffff) == 0) {
                    bits = (bits >> 31) >> 1;
                    assert(bits);
                    n += 32;
                }
                n += ffs(bits) - 1;
                assert(n < nbpw);
                pm_asid_hint = n + i * nbpw;
                break;
            }
        }
        assert(pm_asid_hint > KERNEL_ASID_PID);
        assert(TLBINFO_ASID_INUSE_P(pm_asid_bitmap, pm_asid_hint - 1));
        assert(!TLBINFO_ASID_INUSE_P(pm_asid_bitmap, pm_asid_hint));
    }

    /*
     * The hint contains our next ASID so take it and advance the hint.
     * Mark it as used and insert the pai into the list of active asids.
     * There is also one less asid free in this TLB.
     */
    map->pm_asid = pm_asid_hint++;
    TLBINFO_ASID_MARK_USED(pm_asid_bitmap, map->pm_asid);
    pm_asids_free--;

#if 1
    kprintf("[pmap_asid_alloc_fast] ASIDs free: %d ASIDs, ASID subsystem allocated id %u for map %p!\n", pm_asids_free, map->pm_asid, map);
#endif

    return;
}

/**
 * pmap_asid_reset
 */
static inline void pmap_asid_reset(pmap_t map)
{
    /*
     * We must have an ASID.
     */
    assert(map->pm_asid > KERNEL_ASID_PID);

    /*
     * Note that we don't mark the ASID as not in use in the TLB's ASID
     * bitmap (thus it can't be allocated until the ASID space is exhausted
     * and therefore reinitialized).  We don't want to flush the TLB for
     * entries belonging to this ASID so we will let natural TLB entry
     * replacement flush them out of the TLB.  Any new entries for this
     * pmap will need a new ASID allocated.
     */
    map->pm_asid = 0;

    return;
}

/**
 * pmap_bootstrap
 *
 * Bootstrap the pmap subsystem.
 */
void pmap_bootstrap(__unused uint64_t msize, vm_offset_t * __first_avail, __unused unsigned int kmapsize)
{
    /*
     * Set the first virtual address we can use. 
     */
    virt_begin = *__first_avail;

    /*
     * Make sure we don't go to the ARM Vector Table.
     */
    virt_end = vm_last_addr = 0xFFFFEFFF;

    /*
     * Set the available page amount. 
     */
    avail_remaining = (avail_end - first_avail) >> PAGE_SHIFT;
    vm_first_phys = first_avail;
    avail_start = first_avail;

    kprintf("pmap_bootstrap: physical region 0x%08x - 0x%08x\n", first_avail, avail_end);

    /*
     * Set NPVhash defaults.
     */
    if (PE_parse_boot_argn("npvhash", &npvhash, sizeof(npvhash))) {
        if (0 != ((npvhash + 1) & npvhash)) {
            kprintf("invalid hash %d, must be ((2^N)-1), using default %d\n", npvhash, NPVHASH);
            npvhash = NPVHASH;
        }
    } else {
        npvhash = NPVHASH;
    }
    printf("npvhash=%d\n", npvhash);

    /*
     * ASID initialization.
     */
    pmap_asid_init();

    /*
     * Initialize kernel pmap.
     */
    pmap_static_init();
}

/**
 * pmap_reference
 *
 * Increment reference count of the specified pmap.
 */
void pmap_reference(pmap_t pmap)
{
    /*
     * Bump the count.
     */
    if (pmap != PMAP_NULL)
        (void) hw_atomic_add(&pmap->pm_refcnt, 1);
}

/**
 * pmap_get_refmod
 *
 * Returns the referenced and modified bits of the specified
 * physical page.
 */
unsigned int pmap_get_refmod(ppnum_t pn)
{
    int refmod;
    unsigned int retval = 0;

    refmod = phys_attribute_test(pn, PMAP_OSPTE_TYPE_MODIFIED | PMAP_OSPTE_TYPE_REFERENCED);

    if (refmod & PMAP_OSPTE_TYPE_MODIFIED)
        retval |= VM_MEM_MODIFIED;
    if (refmod & PMAP_OSPTE_TYPE_REFERENCED)
        retval |= VM_MEM_REFERENCED;

    return (retval);
}

/**
 * pmap_enter
 *
 * Enter pages into a physical map.
 */
void pmap_enter(pmap_t pmap, vm_map_offset_t va, ppnum_t pa, vm_prot_t prot, vm_prot_t fault_type, unsigned int flags, boolean_t wired)
{
    pmap_enter_options(pmap, va, pa, prot, fault_type, flags, wired, 0);
}

/**
 * pmap_grab_page
 *
 * Get a page from the global pmap object.
 */
vm_page_t pmap_grab_page(pmap_t pmap)
{
    vm_page_t page;
    uint32_t ctr;
    assert(pmap_initialized && kernel_map && pmap->pm_obj);

    /*
     * Grab pages from the global VM object.
     */
    while ((page = vm_page_grab()) == VM_PAGE_NULL)
        VM_PAGE_WAIT();

    /*
     * Lock the global object to prevent interruptions.
     */
    vm_object_lock(pmap->pm_obj);
    assert((page->phys_page << PAGE_SHIFT) > gPhysBase);
    ctr = (page->phys_page) - (gPhysBase >> PAGE_SHIFT);
    bzero(phys_to_virt(page->phys_page << PAGE_SHIFT), PAGE_SIZE);
    vm_page_insert(page, pmap->pm_obj, ctr);

    /*
     * Wire our new page.
     */
    vm_page_lockspin_queues();
    vm_page_wire(page);
    vm_page_unlock_queues();

    /*
     * Done.
     */
    vm_object_unlock(pmap->pm_obj);

    /*
     * Set noencrypt bits.
     */
    pmap_set_noencrypt(page->phys_page);

    /*
     * Increment inuse ptepages.
     */
    OSAddAtomic(1, &inuse_ptepages_count);
    OSAddAtomic(1, &alloc_ptepages_count);

    return page;
}

/**
 * pmap_destroy_page
 *
 * Free a page from the internal VM object.
 */
void pmap_destroy_page(ppnum_t pa)
{
    vm_page_t m;

    vm_object_lock(pmap_object);

    m = vm_page_lookup(pmap_object, pa);
    if (m == VM_PAGE_NULL)
        return;

    vm_object_unlock(pmap_object);

    VM_PAGE_FREE(m);
    kprintf("Freed page for PA %x\n", pa << PAGE_SHIFT);

    /*
     * Remove one.
     */
    OSAddAtomic(-1, &inuse_ptepages_count);

    return;
}

/**
 * pmap_create_sharedpage
 *
 * Create the system common page.
 */
void pmap_create_sharedpage(void)
{
    /*
     * Grab a page...
     */
    commpage = pmap_grab_page(kernel_pmap);
    assert(commpage);

    /*
     * And map it.
     */
    pmap_enter(kernel_pmap, (vm_map_offset_t) _COMM_PAGE_BASE_ADDRESS, commpage->phys_page, VM_PROT_READ | VM_PROT_WRITE, 0, FALSE, TRUE);

    /*
     * Memset it.
     */
    memset((void *) _COMM_PAGE_BASE_ADDRESS, 0x00, PAGE_SIZE);
    return;
}

/**
 * pmap_extract
 *
 * Get the physical address for a virtual one.
 */
vm_offset_t pmap_extract(pmap_t pmap, vm_offset_t virt)
{
#if 0
    spl_t spl;
    vm_offset_t ppn = 0;
    uint32_t tte, *ttep = pmap_tte(pmap, virt);

    /*
     * Block off all interruptions. Nothing may interrupt the extraction process
     * as the page tables may be changed by another callee to pmap_enter or such.
     */

    PMAP_LOCK(pmap);
    if (!ttep)
        goto extract_out;

    /*
     * Look at the TTE and see what type of mapping it is.
     */
    tte = *ttep;

    /*
     * Verify it's not a section mapping.
     */
    if ((tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_SECTION) {
        /*
         * Clean the lower bits off.
         */
        ppn = (tte & L1_SECT_ADDR_MASK);

        /*
         * Now add the lower bits back from the VA.
         */
        ppn |= (virt & ~(L1_SECT_ADDR_MASK));

        /*
         * Done. Address extraction successful.
         */
        goto extract_out;
    } else if ((tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_PAGE_TABLE) {
        uint32_t pte, *ptep;

        /*
         * Clean the TTE bits off, get the address of the L1 entry.
         */
        pte = L1_PTE_ADDR(tte);
        if (!pte)
            goto extract_out;

        /*
         * Return the virtually mapped PTE.
         */
        ptep = (uint32_t *) ((phys_to_virt(pte) + pte_offset(virt)));

        /*
         * Make sure it's not a large page. They're not supported yet, but they will 
         * be at some point.
         */
        if (((*ptep & ARM_PAGE_MASK_VALUE) == ARM_PTE_DESCRIPTOR_64K))
            panic("pmap_extract: 64kb pages not supported yet");

        /*
         * Clean the PTE bits off the address.
         */
        ppn = (*ptep) & L2_ADDR_MASK;

        /*
         * Now, add the lower bits back from the VA. 
         */
        ppn |= (virt & ~(L2_ADDR_MASK));

        /*
         * Done. Extraction successful.
         */
        goto extract_out;
    } else {
        kprintf("pmap_extract: invalid tte (ttep %x tte %x)\n", ttep, tte);
    }

 extract_out:

    /*
     * Return.
     */
    PMAP_UNLOCK(pmap);
    return ppn;
#else
    uint32_t va = (virt & L2_ADDR_MASK), par;
    boolean_t is_priv = (pmap == kernel_pmap) ? TRUE : FALSE;

    /*
     * Fast VirtToPhys involves using the virtual address trnalsation
     * register as present in Cortex-A and ARM11 MPCore systems.
     *
     * Privileged reads are only done on the kernel PMAP versus user
     * pmaps getting user read/write state.
     *
     * The entire process should take much shorter compared to the
     * older pmap_extract, which fully walked the page tables. You can
     * still use the current behaviour however, by messing with
     * the MASTER files.
     *
     * I swear, I need more stupid sleep.
     */

    /*
     * Set the PAtoVA register and perform the operation.
     */
    if (is_priv)
        armreg_va2pa_pr_ns_write(va);
    else
        armreg_va2pa_ur_ns_write(va);

    /*
     * Wait for the instruction transaction to complete.
     */
    __asm__ __volatile__("isb sy");

    /*
     * See if the translation aborted, log any translation errors.
     */
    par = armreg_par_read();

    /*
     * Successful translation, we're done.
     */
    if (!(par & 1)) {
        uint32_t pa = par & L2_ADDR_MASK;
        pa |= (virt & ~(L2_ADDR_MASK));
        return pa;
    } else {
        /*
         * Log translation fault.
         */
        kprintf("pmap_extract: fast extraction failed, par 0x%x\n", par);
    }

    return 0;
#endif
}

/** 
 * pmap_expand_ttb
 * 
 * Expand and reorganize the current translation-table as to fit a new size.
 */
void pmap_expand_ttb(pmap_t map, vm_offset_t expansion_size)
{
    /*
     * If the requested expansion size is less than or greater, we have nothing to do.
     */
    if (expansion_size <= map->pm_l1_size)
        return;

    /*
     * Do not expand past maximum size.
     */
    if (expansion_size > 0x4000)
        panic("pmap_expand_ttb: attempting to expand past maximum address of %x, map %p, expansion %x\n", 0x4000, map, expansion_size);

    switch (expansion_size) {
    case 0x1000:
        panic("pmap_expand_ttb: attempting to expand an already-expanded pmap?");
    case 0x2000 ... 0x3000:{
            kern_return_t ret;
            vm_page_t pages;

            /*
             * Allocate a contiguous segment of memory for the new L1 mapping table. (including one guard)
             */
            ret = cpm_allocate(expansion_size, &pages, 0, FALSE, KMA_LOMEM);
            assert(ret == KERN_SUCCESS);

            /*
             * We got the new contiguous block.
             */
            bzero(phys_to_virt(pages->phys_page << PAGE_SHIFT), expansion_size);

            /*
             * Copy the old entries to the new area.
             */
            bcopy((void *) map->pm_l1_virt, (void *) phys_to_virt(pages->phys_page << PAGE_SHIFT), map->pm_l1_size);
#if 1
            kprintf("pmap_expand_ttb: 0x%x => 0x%x\n", map->pm_l1_virt, phys_to_virt(pages->phys_page << PAGE_SHIFT));
#endif

            /*
             * Deallocate the old L1.
             */
            pmap_deallocate_l1(map);

            /*
             * Set the new TTB base.
             */
            map->pm_l1_virt = phys_to_virt(pages->phys_page << PAGE_SHIFT);
            map->pm_l1_phys = pages->phys_page << PAGE_SHIFT;
            map->pm_l1_size = expansion_size;

            OSAddAtomic((expansion_size >> PAGE_SHIFT), &inuse_ptepages_count);
            OSAddAtomic((expansion_size >> PAGE_SHIFT), &alloc_ptepages_count);

            /*
             * Switch into the new TTB if it needs to be used.
             */
            if (map == current_cpu_datap()->user_pmap) {
                armv7_context_switch(map->pm_l1_phys);
                armv7_tlb_flushID_ASID(map->pm_asid & 0xFF);
            }

            return;
        }
    default:
        panic("pmap_expand_ttb: invalid expansion size %x\n", expansion_size);
    }

    return;
}

/**
 * pmap_expand
 *
 * Expand the address space of the current physical map.
 */
void pmap_expand(pmap_t map, vm_offset_t v)
{
    vm_offset_t *tte = (vm_offset_t *) pmap_tte(map, v);
    vm_page_t page = pmap_grab_page(map);
    spl_t spl;

    /*
     * High priority. We do not want any interruptions.
     */
    PMAP_LOCK(map);

    if (map != kernel_pmap) {
        /*
         * First, if we have a size below 0x1000, we can't be sure about expanding. 
         */
        if (map->pm_l1_size < 0x1000) {
            panic("pmap_expand: this pmap has a really weird size: %d bytes", map->pm_l1_size);
        }

        /*
         * See if we can make it grow.
         */
        uint32_t expansion_size = ((tte_offset(v)) & ~(PAGE_SIZE - 1)) + PAGE_SIZE;
        pmap_expand_ttb(map, expansion_size);

        /*
         * Refetch the TTE, since the pmap base may have changed.
         */
        tte = (vm_offset_t *) pmap_tte(map, v);

#if 0
        /*
         * Do not extend past the commpage. 
         */
        if (map->pm_l1_size == 0x1000) {
            if (v >= _COMM_PAGE_BASE_ADDRESS) {
                panic("attempting to expand pmap past maximum address of %x\n", _COMM_PAGE_BASE_ADDRESS);
            }
        }
#endif

        /*
         * L1 section mappings may not be expanded any further.
         */
        if ((*tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_SECTION)
            panic("cannot expand current map into L1 sections");
    }

    /*
     * Overwrite the old L1 mapping in this region with a fresh L1 descriptor.
     */
    *tte = ((page->phys_page << PAGE_SHIFT) & L1_PTE_ADDR_MASK) | L1_TYPE_PTE;

 Out:

    /*
     * Flush the TLBs since we updated the page tables.
     */
    armv7_tlb_flushID_SE(v);
    PMAP_UNLOCK(map);
    return;
}

/**
 * pmap_enter_options
 *
 * Create a translation entry for a PA->VA mappings with additional options.
 * Called from vm_fault.
 */
kern_return_t pmap_enter_options(pmap_t pmap, vm_map_offset_t va, ppnum_t pa, vm_prot_t prot, vm_prot_t fault_type, unsigned int flags, boolean_t wired, unsigned int options)
{
    spl_t spl;
    pt_entry_t pte;
    register pv_rooted_entry_t pv_h;
    pv_hashed_entry_t pvh_e;
    pv_hashed_entry_t pvh_new;
    pv_hashed_entry_t *hashp;
    int pvhash_idx;
    uint32_t pv_cnt;
    boolean_t old_pvh_locked = FALSE;

    /*
     * Verify the address isn't fictitious.
     */
    assert(pa != vm_page_fictitious_addr);

    /*
     * Only low addresses are supported for user pmaps.
     */
    if (va > _COMM_PAGE_BASE_ADDRESS && pmap != kernel_pmap) {
        kprintf("pmap_enter_options: low address 0x%08X is invalid for pmap %p\n", va, pmap);
        return KERN_INVALID_ARGUMENT;
    }

    pvh_new = PV_HASHED_ENTRY_NULL;

 Retry:
    pvh_e = PV_HASHED_ENTRY_NULL;

    /*
     * Set a high priority level. We do not wany any interruptions or any unauthorized
     * page table modification.
     */
    PMAP_LOCK(pmap);

    /*
     * Expand the pmap to include the new PTE if necessary to accomodate the new VA we're
     * entering in.
     */
    while ((pte = pmap_pte(pmap, va)) == NULL) {
        PMAP_UNLOCK(pmap);
        pmap_expand(pmap, va);
        PMAP_LOCK(pmap);
    }

    /*
     * If the old page already has a mapping, the caller might be changing protection flags.
     */
    uint32_t old_pte = (*(uint32_t *) pte);

    /*
     * If it's a managed page, lock the pv entry right now.
     */
    if((old_pte & L2_ADDR_MASK) != 0) {
        uint32_t pai = pa_index(old_pte & L2_ADDR_MASK);
        LOCK_PVH(pai);
        old_pvh_locked = TRUE;
        old_pte = (*(uint32_t *) pte);
        if(0 == old_pte) {
            UNLOCK_PVH(pai);
            old_pvh_locked = FALSE;
        }
    }

    if ((old_pte & L2_ADDR_MASK) == (pa << PAGE_SHIFT)) {
        /*
         * !!! IMPLEMENT 'pmap_vm_prot_to_page_flags' !!!
         * XXX protection is not implemented right now, all pages are 'RWX'.
         */

        uint32_t template_pte = ((pa << PAGE_SHIFT) & L2_ADDR_MASK) | L2_SMALL_PAGE;
        template_pte |= pmap_vm_prot_to_page_flags(pmap, prot, wired, 0);

        if (va == _COMM_PAGE_BASE_ADDRESS)
            template_pte |= L2_ACCESS_USER;

        /*
         * Add cacheability attributes.
         */
        template_pte |= pmap_get_cache_attributes(pa);

        if (wired) {
            if (!phys_attribute_test(pa, PMAP_OSPTE_TYPE_WIRED)) {
                OSAddAtomic(+1, &pmap->pm_stats.wired_count);
                phys_attribute_set(pa, PMAP_OSPTE_TYPE_WIRED);
                pmap_ledger_credit(pmap, task_ledgers.wired_mem, PAGE_SIZE);
            } else {
                assert(pmap->pm_stats.wired_count >= 1);
                OSAddAtomic(-1, &pmap->pm_stats.wired_count);
                pmap_ledger_debit(pmap, task_ledgers.wired_mem, PAGE_SIZE);
            }
        }

        *(uint32_t *) pte = template_pte;

        /*
         * The work here is done, the PTE will now have new permissions. Flush the TLBs for the
         * specific VA and then exit.
         */
        if(old_pvh_locked) {
            UNLOCK_PVH(pa);
            old_pvh_locked = FALSE;
        }

        goto enter_options_done;
    }

    /*
     * This is a new mapping, add it to the pv_head_table if pmap is initialized. This is so
     * we can correctly manage our entries.
     */
    if (pmap_initialized) {
        ppnum_t pai;

        /*
         * If the current PA isn't zero, and if it's non-existent... remove the mapping
         */
        if ((old_pte & L2_ADDR_MASK) != 0) {
            pai = pa_index((old_pte & L2_ADDR_MASK));
            pv_h = pai_to_pvh(pai);

            *(uint32_t *) pte = 0;

            if (!pmap_valid_page(pa))
                goto EnterPte;

            /*
             * Set statistics and credit/debit internal pmap ledgers
             */
            {
                pmap_ledger_debit(pmap, task_ledgers.phys_mem, PAGE_SIZE);
                assert(pmap->pm_stats.resident_count >= 1);
                OSAddAtomic(-1, &pmap->pm_stats.resident_count);
            }

            if (phys_attribute_test(pa, PMAP_OSPTE_TYPE_WIRED)) {
                assert(pmap->pm_stats.wired_count >= 1);
                OSAddAtomic(-1, &pmap->pm_stats.wired_count);
                phys_attribute_clear(pa, PMAP_OSPTE_TYPE_WIRED);
            }

            if (pv_h->pmap == PMAP_NULL) {
                panic("pmap_enter_options: null pv_list\n");
            }
            pvh_e = pmap_pv_remove(pmap, va, (ppnum_t *) & pai, 0);
        }

        pai = pa;
        pv_h = pai_to_pvh(pai);

        if (!pmap_valid_page(pa))
            goto EnterPte;

        if(old_pvh_locked) {
            UNLOCK_PVH(pai);
            old_pvh_locked = FALSE;
        }

#if 0
        /*
         * Check to see if it exists, if it does, then make it null. The code later
         * will treat a null mapping as a new one and will enter it anyway.
         */
        if ((pv_h->pv_pmap == pmap) && (pv_h->pv_address_va == va)) {
            pv_entry_t cur;
            cur = pv_h->pv_next;
            if (cur != (pv_entry_t) 0) {
                *pv_h = *cur;
                pv_e = cur;
            } else {
                pv_h->pv_pmap = PMAP_NULL;
            }
        }
#endif
        /*
         *  Step 2) Enter the mapping in the PV list for this
         *  physical page.
         */
        LOCK_PVH(pai);

        /*
         * This is definitely a new mapping.
         */
        if (pv_h->pmap == PMAP_NULL) {
            pv_h->va = va;
            pv_h->pmap = pmap;
            queue_init(&pv_h->qlink);
            if (wired)
                phys_attribute_set(pa, PMAP_OSPTE_TYPE_WIRED);
        } else {
            /*
             *  Add new pv_hashed_entry after header.
             */
            if ((PV_HASHED_ENTRY_NULL == pvh_e) && pvh_new) {
                pvh_e = pvh_new;
                pvh_new = PV_HASHED_ENTRY_NULL;
            } else if (PV_HASHED_ENTRY_NULL == pvh_e) {
                PV_HASHED_ALLOC(pvh_e);
                if (PV_HASHED_ENTRY_NULL == pvh_e) {
                    /*
                     * the pv list is empty. if we are on
                     * the kernel pmap we'll use one of
                     * the special private kernel pv_e's,
                     * else, we need to unlock
                     * everything, zalloc a pv_e, and
                     * restart bringing in the pv_e with
                     * us.
                     */
                    if (kernel_pmap == pmap) {
                        PV_HASHED_KERN_ALLOC(pvh_e);
                    } else {
                        UNLOCK_PVH(pai);
                        PMAP_UNLOCK(pmap);
                        pvh_new = (pv_hashed_entry_t) zalloc(pv_hashed_list_zone);
                        goto Retry;
                    }
                }
            }

            if (PV_HASHED_ENTRY_NULL == pvh_e)
                panic("Mapping alias chain exhaustion, possibly induced by numerous kernel virtual double mappings");
            pvh_e->va = va;
            pvh_e->pmap = pmap;
            pvh_e->ppn = pa;

            pv_hash_add(pvh_e, pv_h);

            /*
             *  Remember that we used the pvlist entry.
             */
            pvh_e = PV_HASHED_ENTRY_NULL;
        }
#if 0
        kprintf("pmap_enter: pai %d pa %d (%x) va %x pv_h %p pmap %p pv_h->pmap %p pv_h->pv_address_va %x\n", pai, pa, pa << PAGE_SHIFT, va, pv_h, pmap, pv_h->pv_pmap, pv_h->pv_address_va);
#endif
    }
 EnterPte:

    /*
     * Enter and count the mapping.
     */
    pmap->pm_stats.resident_count++;
    pmap_ledger_credit(pmap, task_ledgers.phys_mem, PAGE_SIZE);

    if (wired) {
        pmap->pm_stats.wired_count++;
        pmap_ledger_credit(pmap, task_ledgers.phys_mem, PAGE_SIZE);
    }

    /*
     * Set VM protections
     */
    uint32_t template_pte = ((pa << PAGE_SHIFT) & L2_ADDR_MASK) | L2_SMALL_PAGE;
    template_pte |= pmap_vm_prot_to_page_flags(pmap, prot, wired, 0);

    /*
     * Hack for commpage, how is this to be done?
     */
    if (va == _COMM_PAGE_BASE_ADDRESS)
        template_pte |= L2_ACCESS_USER;

    /*
     * Add cacheability attributes.
     */
    template_pte |= pmap_get_cache_attributes(pa);

    *(uint32_t *) pte = template_pte;

    /* 
     * Unlock the pv. (if it is managed by us)
     */
    if(pmap_initialized && pmap_valid_page(pa)) {
        UNLOCK_PVH(pa);
    }

 enter_options_done:
    /*
     * Done, now invalidate the TLB for a single page.
     */
    armv7_tlb_flushID_SE(va);

    if (pvh_e != PV_HASHED_ENTRY_NULL) {
        PV_HASHED_FREE_LIST(pvh_e, pvh_e, 1);
    }
    if (pvh_new != PV_HASHED_ENTRY_NULL) {
        PV_HASHED_KERN_FREE_LIST(pvh_new, pvh_new, 1);
    }

    /*
     * The operation has completed successfully.
     */
    PMAP_UNLOCK(pmap);

    return KERN_SUCCESS;
}

extern vm_offset_t sdata, edata;
extern vm_offset_t sconstdata, econstdata;
extern boolean_t doconstro_override;

/**
 * pmap_init
 *
 * Stage 2 initialization of the pmap subsystem.
 */
void pmap_init(void)
{
    vm_offset_t pv_root;
    vm_size_t s;
    spl_t spl;
    int i;

    kprintf("pmap_init: %d physical pages in memory, kernel pmap at %p\n", (mem_size / PAGE_SIZE), kernel_pmap);

    /*
     * Allocate the core PV structure. The pv_head_table contains trunk entries
     * for every physical page that exists in the system.
     */
    s = (mem_size / PAGE_SIZE) * sizeof(pv_entry);
    if (kernel_memory_allocate(kernel_map, &pv_root, s, 0, KMA_KOBJECT | KMA_PERMANENT) != KERN_SUCCESS)
        panic("pmap_init: failed to allocate pv table!");

    /*
     * Okay. Zero out the PV head table.
     */
    pv_head_table = (pv_entry_t) pv_root;
    kprintf("pmap_init: pv_head_table at %p\n", pv_head_table);
    bzero((void *) pv_head_table, s);

    /*
     * Initialize the Zones for object allocation. 
     */
    pmap_zone = zinit((sizeof(struct pmap)), 400 * (sizeof(struct pmap)), 4096, "pmap_pmap");

    /*
     * Expandable zone. (pv_entry zone)
     */
    pve_zone = zinit((sizeof(struct __pv_entry__)), 10000 * (sizeof(struct __pv_entry__)), 4096, "pmap_pve");

    /*
     * Allocate memory for the pv_head_hash_table.
     */
    s = (vm_size_t) (sizeof(struct pv_rooted_entry) * (mem_size / PAGE_SIZE)
                     + (sizeof(struct pv_hashed_entry_t *) * (npvhash + 1))
                     + pv_lock_table_size((mem_size / PAGE_SIZE))
                     + pv_hash_lock_table_size((npvhash + 1))
                     + (mem_size / PAGE_SIZE));
    if (kernel_memory_allocate(kernel_map, &pv_root, s, 0, KMA_KOBJECT | KMA_PERMANENT) != KERN_SUCCESS)
        panic("pmap_init: failed to allocate pv hash table!");

    /*
     * Initialize the core objects.
     */
    uint32_t npages = (mem_size / PAGE_SIZE);
    pv_head_hash_table = (pv_rooted_entry_t) pv_root;
    pv_root = (vm_offset_t) (pv_head_table + npages);

    pv_hash_table = (pv_hashed_entry_t *) pv_root;
    pv_root = (vm_offset_t) (pv_hash_table + (npvhash + 1));

    pv_lock_table = (char *) pv_root;
    pv_root = (vm_offset_t) (pv_lock_table + pv_lock_table_size(npages));

    pv_hash_lock_table = (char *) pv_root;
    pv_root = (vm_offset_t) (pv_hash_lock_table + pv_hash_lock_table_size((npvhash + 1)));

    bzero((void *) pv_head_hash_table, s);
    kprintf("pmap_init: pv_head_hash_table at %p\n", pv_head_hash_table);

    /*
     * PVHash Zone
     */
    pv_hashed_list_zone = zinit(sizeof(struct pv_hashed_entry), 10000 * sizeof(struct pv_hashed_entry), 4096, "pv_list");   /* XXX */

    /*
     * Initialize the free list lock. (unused right now.)
     */
    simple_lock_init(&kernel_pmap->lock, 0);
    simple_lock_init(&pv_free_list_lock, 0);
    simple_lock_init(&pv_hashed_free_list_lock, 0);
    simple_lock_init(&pv_hashed_kern_free_list_lock, 0);
    simple_lock_init(&pv_hash_table_lock, 0);

    /*
     * Remap kernel as RO only.
     */
    uint32_t ro_kern = 1;
    if (PE_parse_boot_argn("kernel_read_only", &ro_kern, sizeof(ro_kern))) {
        ro_kern = 0;
    }
    SPLVM(spl);

    /*
     * Rewrite the kernel PTEs.
     */
    if (ro_kern) {
        vm_offset_t kva;
        pt_entry_t *ptep;

        kprintf("Kernel text %x-%x to be write-protected\n", vm_kernel_stext, vm_kernel_etext);

        /*
         * Add APX-bit to reduce protections to R-X.
         */
        for (kva = vm_kernel_stext; kva < vm_kernel_etext; kva += PAGE_SIZE) {
            ptep = pmap_pte(kernel_pmap, (vm_map_offset_t) kva);
            if (ptep)
                *ptep |= L2_ACCESS_APX;
        }
    }

    /*
     * Set const to R-- only too.
     */
    boolean_t doconstro = TRUE;

    (void) PE_parse_boot_argn("dataconstro", &doconstro, sizeof(doconstro));

    if ((sconstdata | econstdata) & PAGE_MASK) {
        kprintf("Const DATA misaligned 0x%lx 0x%lx\n", sconstdata, econstdata);
        if ((sconstdata & PAGE_MASK) || (doconstro_override == FALSE))
            doconstro = FALSE;
    }

    if ((sconstdata > edata) || (sconstdata < sdata)
        || ((econstdata - sconstdata) >= (edata - sdata))) {
        kprintf("Const DATA incorrect size 0x%lx 0x%lx 0x%lx 0x%lx\n", sconstdata, econstdata, sdata, edata);
        doconstro = FALSE;
    }

    if (doconstro)
        kprintf("Marking const DATA read-only\n");

    vm_offset_t dva;
    for (dva = sdata; dva < edata; dva += PAGE_SIZE) {
        pt_entry_t *pte, dpte;
        pte = pmap_pte(kernel_pmap, dva);
        assert(pte);

        /*
         * Make sure the PTE is valid.
         */
        dpte = *pte;
        assert(dpte & ARM_PTE_DESCRIPTOR_4K);
        if (!(dpte & ARM_PTE_DESCRIPTOR_4K)) {
            kprintf("Missing data mapping 0x%x 0x%x 0x%x\n", dva, sdata, edata);
            continue;
        }

        /*
         * Enforce NX and RO as necessary.
         */
        dpte |= L2_NX_BIT;
        if (doconstro && (dva >= sconstdata) && (dva < econstdata)) {
            dpte |= L2_ACCESS_APX;
        }
        *pte = dpte;
    }

    /*
     * Just flush the entire TLB since we messed with quite a lot of mappings.
     */
    armv7_tlb_flushID();

    SPLX(spl);

    /*
     * Set up the core VM object.
     */
    pmap_object = &pmap_object_store;
    _vm_object_allocate(mem_size, &pmap_object_store);
    kernel_pmap->pm_obj = pmap_object;

    /*
     * Done initializing. 
     */
    pmap_initialized = TRUE;

    return;
}

/**
 * pmap_remove_range
 *
 * Remove a range of hardware page-table entries. (This function does not support section mappings.)
 */
void pmap_remove_range(pmap_t pmap, vm_map_offset_t start_vaddr, pt_entry_t * spte, pt_entry_t * epte, boolean_t is_sect)
{
    pt_entry_t *cpte = spte;
    vm_map_offset_t vaddr = start_vaddr;
    vm_size_t our_page_size = (is_sect) ? (_1MB) : PAGE_SIZE;
    int num_removed = 0, num_unwired = 0;
    pv_hashed_entry_t pvh_et = PV_HASHED_ENTRY_NULL;
    pv_hashed_entry_t pvh_eh = PV_HASHED_ENTRY_NULL;
    pv_hashed_entry_t pvh_e;
    int pvh_cnt = 0;
    int pvhash_idx;
    uint32_t pv_cnt;

    /*
     * Make sure the Cpte/Epte are within sane boundaries. (256 entries, one L2 area size.)
     */
    if (((vm_offset_t) epte - (vm_offset_t) cpte) > L2_SIZE)
        panic("pmap_remove_range: attempting to remove more ptes than 256!\n");

    for (cpte = spte, vaddr = start_vaddr; cpte < epte; cpte++, vaddr += our_page_size) {
        /*
         * Start nuking the range. 
         */
        pt_entry_t *p = cpte;

        /*
         * Get the index for the PV table.
         */
        ppnum_t pai = pa_index(*cpte & L2_ADDR_MASK);
        if (pai == 0) {
            continue;
        }

        num_removed++;
        if (phys_attribute_test(pai, PMAP_OSPTE_TYPE_WIRED)) {
            phys_attribute_clear(pai, PMAP_OSPTE_TYPE_WIRED);
            num_removed++;
        }

        /*
         * Nuke the page table entry.
         */
        *cpte = 0;

        /*
         * Continue onwards if pmap isn't up yet.. (keep nuking pages!)
         */
        if (!pmap_initialized)
            continue;

        /*
         * If it isn't a managed page, don't update the pv_table.
         */
        if (!pmap_valid_page(pai))
            continue;

        LOCK_PVH(pai);
        /*
         *  Remove the mapping from the pvlist for
         *  this physical page.
         */
        {
            pvh_e = pmap_pv_remove(pmap, vaddr, (ppnum_t *) & pai, cpte);
            UNLOCK_PVH(pai);
            if (pvh_e != PV_HASHED_ENTRY_NULL) {
                pvh_e->qlink.next = (queue_entry_t) pvh_eh;
                pvh_eh = pvh_e;

                if (pvh_et == PV_HASHED_ENTRY_NULL) {
                    pvh_et = pvh_e;
                }
                pvh_cnt++;
            }
        }                       /* removing mappings for this phy page */
    }

    if (pvh_eh != PV_HASHED_ENTRY_NULL) {
        PV_HASHED_FREE_LIST(pvh_eh, pvh_et, pvh_cnt);
    }

 out:
    /*
     * Invalidate all TLBs.
     */
    armv7_tlb_flushID_RANGE(start_vaddr, vaddr);

    /*
     * Make sure the amount removed isn't... weird.
     */
    if (pmap->pm_stats.resident_count < num_removed)
        panic("pmap_remove_range: resident_count");
    pmap_ledger_debit(pmap, task_ledgers.phys_mem, num_removed * PAGE_SIZE);
    assert(pmap->pm_stats.resident_count >= num_removed);
    OSAddAtomic(-num_removed, &pmap->pm_stats.resident_count);

    if (pmap->pm_stats.wired_count < num_unwired)
        panic("pmap_remove_range: wired_count");
    assert(pmap->pm_stats.wired_count >= num_unwired);
    OSAddAtomic(-num_unwired, &pmap->pm_stats.wired_count);
    pmap_ledger_debit(pmap, task_ledgers.wired_mem, num_unwired * PAGE_SIZE);

    return;
}

/**
 * pmap_remove
 *
 * Remove the given range of addresses from the specified map.
 */
void pmap_remove(pmap_t map, vm_offset_t sva, vm_offset_t eva)
{
    spl_t spl;
    pt_entry_t *tte;
    vm_offset_t *spte, *epte, lva = sva;

    /*
     * Verify the pages are page aligned.
     */
    assert(!(sva & PAGE_MASK));
    assert(!(eva & PAGE_MASK));

    /*
     * High Priority. Nothing may interrupt the removal process.
     */
    PMAP_LOCK(map);

    /*
     * This is broken.
     */
    while (sva < eva) {
        lva = (sva + _1MB) & ~((_1MB) - 1);
        if (lva > eva)
            lva = eva;
        tte = pmap_tte(map, sva);
        assert(tte);
        if (tte && ((*tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_PAGE_TABLE)) {
            pt_entry_t *spte_begin;
            spte_begin = (pt_entry_t *) (phys_to_virt(L1_PTE_ADDR(*tte)));
            spte = (vm_offset_t) spte_begin + (vm_offset_t) pte_offset(sva);
            epte = (vm_offset_t) spte_begin + (vm_offset_t) pte_offset(lva);

            /*
             * If the addresses are more than one 1MB apart, well...
             */
            if ((sva >> L1SHIFT) != (lva >> L1SHIFT)) {
                int mb_off = (lva >> L1SHIFT) - (sva >> L1SHIFT);
                epte = (vm_offset_t) spte_begin + (0x400 * mb_off) + (vm_offset_t) pte_offset(lva);
            }

            assert(epte >= spte);

            /*
             * Make sure the range isn't bogus.
             */
            if (((vm_offset_t) epte - (vm_offset_t) spte) > L2_SIZE) {
                panic("pmap_remove: attempting to remove bogus PTE range");
            }

            pmap_remove_range(map, sva, spte, epte, FALSE);
        }
        sva = lva;
    }

    /*
     * Flush TLBs since we modified page table entries.
     */
    armv7_tlb_flushID_RANGE(sva, eva);

    /*
     * Return. 
     */
    PMAP_UNLOCK(map);
    return;
}

/**
 * pmap_create
 *
 * Create a pmap.
 */
pmap_t pmap_create(ledger_t ledger, vm_map_size_t size, __unused boolean_t is_64bit)
{
    pmap_t our_pmap;
    vm_page_t new_l1;

    /*
     * Some necessary requisites.
     */
    if (!pmap_initialized || size || !kernel_task)
        return PMAP_NULL;

    /*
     * Zalloc a new one.
     */
    our_pmap = (pmap_t) zalloc(pmap_zone);
    if (!our_pmap) {
        panic("pmap_create: allocating the new pmap failed");
    }
    our_pmap->pm_refcnt = 1;
    our_pmap->ledger = ledger;
    our_pmap->pm_asid = 0;
    pmap_common_init(our_pmap);

#ifdef _NOTYET_
    pmap_asid_alloc_fast(our_pmap);
#endif

    /*
     * Create the pmap VM object. 
     */
    if (NULL == (our_pmap->pm_obj = vm_object_allocate((vm_object_size_t) (4096 * PAGE_SIZE))))
        panic("pmap_create: pm_obj null");

    /*
     * Grab a new page and set the new L1 region.
     */
    new_l1 = pmap_grab_page(our_pmap);
    our_pmap->pm_l1_phys = new_l1->phys_page << PAGE_SHIFT;
    our_pmap->pm_l1_virt = phys_to_virt(new_l1->phys_page << PAGE_SHIFT);
    bzero(phys_to_virt(new_l1->phys_page << PAGE_SHIFT), PAGE_SIZE);

    /*
     * New pmaps have 4096 bytes of TTB area.
     */
    our_pmap->pm_l1_size = PAGE_SIZE;

    /*
     * Done.
     */
    return our_pmap;
}

/**
 * pmap_page_protect
 *
 * Lower the protections on a set of mappings.
 */
void pmap_page_protect(ppnum_t pn, vm_prot_t prot)
{
    boolean_t remove;
    spl_t spl;
    pv_hashed_entry_t pvh_eh = PV_HASHED_ENTRY_NULL;
    pv_hashed_entry_t pvh_et = PV_HASHED_ENTRY_NULL;
    pv_hashed_entry_t nexth;
    int pvh_cnt = 0;
    int pvhash_idx;
    pv_rooted_entry_t pv_h;
    pv_rooted_entry_t pv_e;
    pv_hashed_entry_t pvh_e;
    register pmap_t pmap;
    pt_entry_t *pte;

    /*
     * Verify it's not a fictitious page.
     */
    assert(pn != vm_page_fictitious_addr);

    /*
     * Verify said page is managed by us.
     */
    assert(pmap_initialized);
    if (!pmap_valid_page(pn)) {
        return;
    }

    /*
     * Determine the new protection.
     */
    switch (prot) {
    case VM_PROT_READ:
    case VM_PROT_READ | VM_PROT_EXECUTE:
        remove = FALSE;
        break;
    case VM_PROT_ALL:
        return;                 /* nothing to do */
    default:
        remove = TRUE;
        break;
    }

    /*
     * Walk down the PV listings and remove the entries.
     */
    pv_h = pai_to_pvh(pn);
    LOCK_PVH(pn);

    /*
     * Walk down PV list, changing or removing all mappings.
     */
    if (pv_h->pmap != PMAP_NULL) {

        pv_e = pv_h;
        pvh_e = (pv_hashed_entry_t) pv_e;   /* cheat */

        do {
            register vm_map_offset_t vaddr;
            pmap = pv_e->pmap;

            vaddr = pv_e->va;
            pte = pmap_pte(pmap, vaddr);

            if (0 == pte) {
                panic("pmap_page_protect(): null PTE pmap=%p pn=0x%x vaddr=0x%x\n", pmap, pn, vaddr);
            }
            nexth = (pv_hashed_entry_t) queue_next(&pvh_e->qlink);  /* if there is one */

            /*
             * Remove the mapping if new protection is NONE
             * or if write-protecting a kernel mapping.
             */
            if (remove || pmap == kernel_pmap) {
                /*
                 * Remove the mapping, collecting any modify bits.
                 */
                *(pt_entry_t *) pte = 0;
                armv7_tlb_flushID_SE(vaddr);
                phys_attribute_clear(pn, PMAP_OSPTE_TYPE_REFERENCED | PMAP_OSPTE_TYPE_MODIFIED);
                if (pmap->pm_stats.resident_count < 1)
                    panic("pmap_page_protect: resident_count");
                assert(pmap->pm_stats.resident_count >= 1);
                OSAddAtomic(-1, (SInt32 *) & pmap->pm_stats.resident_count);
                pmap_ledger_debit(pmap, task_ledgers.phys_mem, PAGE_SIZE);

                /*
                 * Deal with the pv_rooted_entry.
                 */

                if (pv_e == pv_h) {
                    /*
                     * Fix up head later.
                     */
                    pv_h->pmap = PMAP_NULL;
                } else {
                    /*
                     * Delete this entry.
                     */
                    pv_hash_remove(pvh_e);
                    pvh_e->qlink.next = (queue_entry_t) pvh_eh;
                    pvh_eh = pvh_e;

                    if (pvh_et == PV_HASHED_ENTRY_NULL)
                        pvh_et = pvh_e;
                    pvh_cnt++;
                }
            } else {
                /*
                 * Write-protect.
                 */
                *(pt_entry_t *) pte |= (L2_ACCESS_APX);
                armv7_tlb_flushID_SE(vaddr);
            }

            pvh_e = nexth;
        } while ((pv_e = (pv_rooted_entry_t) nexth) != pv_h);

        /*
         * If pv_head mapping was removed, fix it up.
         */

        if (pv_h->pmap == PMAP_NULL) {
            pvh_e = (pv_hashed_entry_t) queue_next(&pv_h->qlink);

            if (pvh_e != (pv_hashed_entry_t) pv_h) {
                pv_hash_remove(pvh_e);
                pv_h->pmap = pvh_e->pmap;
                pv_h->va = pvh_e->va;
                pvh_e->qlink.next = (queue_entry_t) pvh_eh;
                pvh_eh = pvh_e;

                if (pvh_et == PV_HASHED_ENTRY_NULL)
                    pvh_et = pvh_e;
                pvh_cnt++;
            }
        }
    }
    if (pvh_eh != PV_HASHED_ENTRY_NULL) {
        PV_HASHED_FREE_LIST(pvh_eh, pvh_et, pvh_cnt);
    }
    UNLOCK_PVH(pn);
}

/**
 * pmap_deallocate_l1
 *
 * Deallocate the allocated L1 translation table.
 */
void pmap_deallocate_l1(pmap_t pmap)
{
    uint32_t ttb_base = pmap->pm_l1_phys;
    vm_page_t m;

    /*
     * If the pmap is expanded past 0x1000, we must use cpm_deallocate.
     */
    if (pmap->pm_l1_size > 0x1000) {
        /*
         * xxx todo 
         */
        return;
    }

    /*
     * Lock the VM object. 
     */
    vm_object_lock(pmap->pm_obj);

    /*
     * Look up the page.
     */
    m = vm_page_lookup(pmap->pm_obj, (vm_object_offset_t) ((ttb_base >> PAGE_SHIFT) - (gPhysBase >> PAGE_SHIFT)));
    assert(m);

    /*
     * Got it, now free it.
     */
    VM_PAGE_FREE(m);

    /*
     * Done.
     */
    vm_object_unlock(pmap->pm_obj);

    /*
     * Remove one.
     */
    OSAddAtomic(-1, &inuse_ptepages_count);

    /*
     * Invalidation of the entire pmap should be done.
     */
    return;
}

/**
 * pmap_destroy
 *
 * Destroy the current physical map.
 */
void pmap_destroy(pmap_t pmap)
{
    spl_t spl;
    int refcnt, i;

    /*
     * Some necessary prerequisites.
     */
    assert(pmap_initialized);

    /*
     * NEVER EVER EVER DESTROY THE KERNEL PMAP 
     */
    if (pmap == kernel_pmap)
        panic("pmap_destroy: attempting to destroy kernel_pmap");

    PMAP_LOCK(pmap);

    /*
     * Okay, decrease the reference count.
     */
    refcnt = --pmap->pm_refcnt;
    if (refcnt == 0) {
        /*
         * We might be using this pmap, invalidate all TLBs.
         */
        armv7_tlb_flushID();
    }

    /*
     * Unlock the pmap system.
     */
    PMAP_UNLOCK(pmap);

    /*
     * If the pmap still has a reference count, we don't kill it.
     */
    if (refcnt != 0) {
        return;
    }

    /*
     * Free the associated objects with the pmap first.
     */
    pmap_deallocate_l1(pmap);
    ledger_dereference(pmap->ledger);

    /*
     * Free the 'expanded' pages.
     */
    OSAddAtomic(-pmap->pm_obj->resident_page_count, &inuse_ptepages_count);
    PMAP_ZINFO_PFREE(pmap, pmap->pm_obj->resident_page_count * PAGE_SIZE);
    vm_object_deallocate(pmap->pm_obj);

    /*
     * Free the actual pmap.
     */
    zfree(pmap_zone, pmap);

    /*
     * Done.
     */
    return;
}

/**
 * pmap_protect
 *t
 * Lower the specified protections on a certain map from sva to eva using prot prot.
 */
void pmap_protect(pmap_t map, vm_map_offset_t sva, vm_map_offset_t eva, vm_prot_t prot)
{
    register pt_entry_t *tte;
    register pt_entry_t *spte, *epte;
    vm_map_offset_t lva;
    vm_map_offset_t orig_sva;
    boolean_t set_NX;
    int num_found = 0;

    /*
     * Verify the start and end are page aligned. 
     */
    assert(!(sva & PAGE_MASK));
    assert(!(eva & PAGE_MASK));

    /*
     * Remove PTEs if they're set to VM_PROT_NONE.
     */
    if (map == PMAP_NULL)
        return;

    if (prot == VM_PROT_NONE) {
        pmap_remove(map, sva, eva);
        return;
    }

    /*
     * Enforce NX if necessary.
     */
    if ((prot & VM_PROT_EXECUTE) || !nx_enabled)
        set_NX = FALSE;
    else
        set_NX = TRUE;

    /*
     * Lock the pmap and set the protections on the PTEs. 
     */
    PMAP_LOCK(map);

    /*
     * This is broken.
     */
    orig_sva = sva;
    while (sva < eva) {
        lva = (sva + _1MB) & ~((_1MB) - 1);
        if (lva > eva)
            lva = eva;
        tte = pmap_tte(map, sva);
        assert(tte);
        if (tte && ((*tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_PAGE_TABLE)) {
            pt_entry_t *spte_begin;
            spte_begin = (pt_entry_t *) (phys_to_virt(L1_PTE_ADDR(*tte)));
            spte = (vm_offset_t) spte_begin + (vm_offset_t) pte_offset(sva);
            epte = (vm_offset_t) spte_begin + (vm_offset_t) pte_offset(lva);

            /*
             * If the addresses are more than one 1MB apart, well...
             */
            if ((sva >> L1SHIFT) != (lva >> L1SHIFT)) {
                int mb_off = (lva >> L1SHIFT) - (sva >> L1SHIFT);
                epte = (vm_offset_t) spte_begin + (0x400 * mb_off) + (vm_offset_t) pte_offset(lva);
            }

            assert(epte >= spte);

            /*
             * Make sure the range isn't bogus.
             */
            if (((vm_offset_t) epte - (vm_offset_t) spte) > L2_SIZE)
                panic("pmap_protect: attempting to protect bogus PTE range");;

            while (spte < epte) {
                if (*spte & ARM_PTE_DESCRIPTOR_4K) {
                    assert(*spte & ARM_PTE_DESCRIPTOR_4K);

                    /*
                     * Make the PTE RO if necessary.
                     */
                    if (prot & VM_PROT_WRITE)
                        *spte &= ~(L2_ACCESS_APX);
                    else
                        *spte |= L2_ACCESS_APX;

                    /*
                     * Enforce NX bit.
                     */
                    if (set_NX)
                        *spte |= L2_NX_BIT;
                    else
                        *spte &= ~(L2_NX_BIT);
                    num_found++;
                }
                spte++;
            }
        }
        sva = lva;
    }

    /*
     * We're done with that, bye.
     */
    armv7_tlb_flushID_RANGE(sva, eva);
    PMAP_UNLOCK(map);

    return;
}

/**
 * pmap_nest
 *
 * Nest a pmap with new mappings into a master pmap.
 */
kern_return_t pmap_nest(pmap_t grand, pmap_t subord, addr64_t va_start, addr64_t nstart, uint64_t size)
{
    int copied;
    unsigned int i;
    vm_offset_t *tte, *ntte;
    vm_map_offset_t nvaddr, vaddr;

    /*
     * Anounce ourselves. We are nesting one pmap inside another.
     */
    kprintf("pmap_nest: %p[0x%08llx] => %p[0x%08llx], %d tte entries\n", subord, va_start, grand, nstart, size >> L1SHIFT);

    /*
     * Sanity checks.
     */
    if (size == 0) {
        panic("pmap_nest: size is invalid - %016llX\n", size);
    }

    if (va_start != nstart)
        panic("pmap_nest: va_start(0x%llx) != nstart(0x%llx)\n", va_start, nstart);

    /*
     * Start the copy operations.
     */
    PMAP_LOCK(subord);

    /*
     * Mark the surbodinate pmap as shared.
     */
    uint32_t num_sect = size >> L1SHIFT;
    subord->pm_shared = TRUE;
    nvaddr = (vm_map_offset_t) nstart;

    /*
     * Expand the subordinate pmap to fit.
     */
    for (i = 0; i < num_sect; i++) {
        /*
         * Fetch the TTE and expand the pmap if there is not one. 
         */
        ntte = pmap_tte(subord, nvaddr);

        while (ntte == 0 || ((*ntte & ARM_PAGE_MASK_VALUE) != ARM_PAGE_PAGE_TABLE)) {
            PMAP_UNLOCK(subord);
            pmap_expand(subord, nvaddr);
            PMAP_LOCK(subord);
            ntte = pmap_tte(subord, nvaddr);
        }

        /*
         * Increase virtual address by granularity of one TTE entry.
         */
        nvaddr += (_1MB);
    }
    PMAP_UNLOCK(subord);

    /*
     * Initial expansion of the Subordinate pmap is done, copy the new entries to the
     * master Grand pmap.
     */
    PMAP_LOCK(grand);
    vaddr = (vm_map_offset_t) va_start;
    for (i = 0; i < num_sect; i++) {
        pt_entry_t target;

        /*
         * Get the initial TTE from the subordinate map and verify it.
         */
        ntte = pmap_tte(subord, vaddr);
        if (ntte == 0)
            panic("pmap_nest: no ntte, subord %p nstart 0x%x", subord, nstart);
        target = *ntte;

        nstart += (_1MB);

        /*
         * Now, get the TTE address from the Grand map.
         */
        tte = pmap_tte(grand, vaddr);
        if (tte == 0)
            panic("pmap_nest: no tte, grand %p vaddr 0x%x", grand, vaddr);

        /*
         * Store the TTE.
         */
        *tte = target;
        vaddr += (_1MB);
    }
    PMAP_UNLOCK(grand);

    /*
     * Out. Flush all TLBs.
     */
    armv7_tlb_flushID_RANGE(va_start, va_start + size);

    return KERN_SUCCESS;
}

/**
 * pmap_unnest
 *
 * Remove a nested pmap.
 */
kern_return_t pmap_unnest(pmap_t grand, addr64_t vaddr, uint64_t size)
{
    vm_offset_t *tte;
    unsigned int i, num_sect;
    addr64_t vstart, vend;
    spl_t spl;

    /*
     * Verify the sizes aren't unaligned.
     */
    if ((size & (pmap_nesting_size_min - 1)) || (vaddr & (pmap_nesting_size_min - 1))) {
        panic("pmap_unnest(%p,0x%x,0x%x): unaligned addresses\n", grand, vaddr, size);
    }

    /*
     * Align everything to a 1MB boundary. (TTE granularity)
     */
    vstart = vaddr & ~((_1MB) - 1);
    vend = (vaddr + size + (_1MB) - 1) & ~((_1MB) - 1);
    size = (vend - vstart);

    /*
     * Lock the pmaps to prevent use.
     */
    PMAP_LOCK(grand);

    num_sect = size >> L1SHIFT;
    vaddr = vstart;
    for (i = 0; i < num_sect; i++) {
        tte = pmap_tte(grand, (vm_map_offset_t) vaddr);
        if (tte == 0)
            panic("pmap_unnest: no tte, grand %p vaddr 0x%x\n", grand, vaddr);
        *tte = 0;
        vaddr += (_1MB);
    }

    /*
     * The operation has now completed.
     */
    armv7_tlb_flushID_RANGE(vaddr, vaddr + size);

    PMAP_UNLOCK(grand);

    return KERN_SUCCESS;
}

/**
 * pmap_disconnect
 *
 * Remove a page and return the referenced bits.
 */
unsigned int pmap_disconnect(ppnum_t pa)
{
    /*
     * Disconnect the page.
     */
    pmap_page_protect(pa, 0);
    return pmap_get_refmod(pa);
}

/*
 * kern_return_t
 * pmap_add_physical_memory(vm_offset_t spa, vm_offset_t epa,
 *                          boolean_t available, unsigned int attr)
 *
 *  THIS IS NOT SUPPORTED
 */
kern_return_t pmap_add_physical_memory(__unused vm_offset_t spa, __unused vm_offset_t epa, __unused boolean_t available, __unused unsigned int attr)
{
    panic("Forget it! You can't map no more memory, you greedy puke!\n");
    return KERN_SUCCESS;
}

/**
 * pmap_zero_part_page
 *
 * Zeroes the specified (machine independent) pages.
 */
void pmap_zero_part_page(ppnum_t src, vm_offset_t src_offset, vm_offset_t len)
{
    assert(src != vm_page_fictitious_addr);
    assert((((src << PAGE_SHIFT) & PAGE_MASK) + src_offset + len) <= PAGE_SIZE);
    bzero(phys_to_virt(src << PAGE_SHIFT) + src_offset, len);
}

/**
 * pmap_copy_part_lpage
 * 
 * Copy part of a virtually addressed page 
 * to a physically addressed page.
 */
void pmap_copy_part_lpage(vm_offset_t src, vm_offset_t dst, vm_offset_t dst_offset, vm_size_t len)
{
    panic("pmap_copy_part_lpage");
}

/**
 * pmap_copy_part_rpage
 *
 * Copy part of a physically addressed page 
 * to a virtually addressed page.
 */
void pmap_copy_part_rpage(vm_offset_t src, vm_offset_t src_offset, vm_offset_t dst, vm_size_t len)
{
    panic("pmap_copy_part_rpage");
}

/**
 * pmap_copy
 *
 * Unused.
 */
void pmap_copy(pmap_t dst, pmap_t src, vm_offset_t dst_addr, vm_size_t len, vm_offset_t src_addr)
{
    return;
}

/*
 * These functions are used for bookkeeping.
 */
void pt_fake_zone_init(int zone_index)
{
    pt_fake_zone_index = zone_index;
}

void pt_fake_zone_info(int *count, vm_size_t * cur_size, vm_size_t * max_size, vm_size_t * elem_size, vm_size_t * alloc_size, uint64_t * sum_size, int *collectable, int *exhaustable, int *caller_acct)
{
    *count = inuse_ptepages_count;
    *cur_size = PAGE_SIZE * inuse_ptepages_count;
    *max_size = PAGE_SIZE * (inuse_ptepages_count + vm_page_inactive_count + vm_page_active_count + vm_page_free_count);
    *elem_size = PAGE_SIZE;
    *alloc_size = PAGE_SIZE;
    *sum_size = alloc_ptepages_count * PAGE_SIZE;

    *collectable = 1;
    *exhaustable = 0;
    *caller_acct = 1;
}
