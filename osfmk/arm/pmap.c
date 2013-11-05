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

#if 0
#define PMAP_LOCK(pmap) {       \
    simple_lock(&(pmap)->lock); \
}

#define PMAP_UNLOCK(pmap) {         \
    simple_unlock(&(pmap)->lock);       \
}
#endif

#define PMAP_LOCK(pmap)
#define PMAP_UNLOCK(pmap)

/** Useful Macros */
#define pa_index(pa)        (atop(pa))
#define pai_to_pvh(pai)     (&pv_head_table[pai - atop(gPhysBase)])

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

/** Template PTEs */

/*
 * Protection flags for various requested VM definitions, all of them are in here.
 * These are per ARMv6/ARM11JZF-S defintions.
 */
arm_l2_t arm_pte_prot_templates[] = {
    /*
     * Mode         Flags   Mode    Flags 
     */
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

/** Functions */

/**
 * pmap_vm_prot_to_page_flags
 */
uint32_t pmap_vm_prot_to_page_flags(pmap_t pmap, vm_prot_t prot, int wired,
                                    int nx)
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
    pv_entry_t pv_h;

    assert(pn != vm_page_fictitious_addr);

    pv_h = pai_to_pvh(pn);
    pv_h->pv_flags &= ~bits;

    return;
}

int phys_attribute_test(ppnum_t pn, int bits)
{
    int pai;
    pv_entry_t pv_h;

    assert(pn != vm_page_fictitious_addr);

    pv_h = pai_to_pvh(pn);
    if ((pv_h->pv_flags & bits) == bits)
        return bits;

    return (pv_h->pv_flags & bits);
}

void phys_attribute_set(ppnum_t pn, int bits)
{
    int pai;
    pv_entry_t pv_h;

    assert(pn != vm_page_fictitious_addr);

    pv_h = pai_to_pvh(pn);
    pv_h->pv_flags |= bits;

    return;
}

/**
 * pmap_adjust_unnest_parameters
 *
 * Invoked by the Mach VM to determine the platform specific unnest region.
 */
boolean_t pmap_adjust_unnest_parameters(pmap_t p, vm_map_offset_t * s,
                                        vm_map_offset_t * e)
{
    return FALSE;
}

/**
 * pmap_attributes
 *
 * Set/Get special memory attributes; Set/Get is not implemented.
 */
kern_return_t pmap_attribute(pmap_t pmap, vm_offset_t address, vm_size_t size,
                             vm_machine_attribute_t atte,
                             vm_machine_attribute_val_t * attrp)
{
    return KERN_INVALID_ARGUMENT;
}

/**
 * pmap_attribute_cache_sync
 *
 * Flush appropriate cache based on page number sent.
 */
kern_return_t pmap_attribute_cache_sync(ppnum_t pn, vm_size_t size,
                                        vm_machine_attribute_t attr,
                                        vm_machine_attribute_val_t * attrp)
{
    return KERN_SUCCESS;
}

/**
 * pmap_cache_attributes
 */
unsigned int pmap_cache_attributes(ppnum_t pn)
{
    panic("pmap_cache_attributes");
}

/**
 * pmap_clear_noencrypt
 */
void pmap_clear_noencrypt(ppnum_t pn)
{
    return;
}

/**
 * pmap_is_noencrypt
 */
boolean_t pmap_is_noencrypt(ppnum_t pn)
{
    return FALSE;
}

/**
 * pmap_set_noencrypt
 */
void pmap_set_noencrypt(ppnum_t pn)
{
    return;
}

/**
 * pmap_set_cache_attributes
 *
 * Set the specified cache attributes.
 */
void pmap_set_cache_attributes(ppnum_t pn, unsigned int cacheattr)
{
    return;
}

/**
 * compute_pmap_gc_throttle
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
void pmap_change_wiring(pmap_t pmap, vm_map_offset_t va, boolean_t wired)
{
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
        panic("Translation table entry extends past L1 size (base: 0x%08X)",
              tte_offset_begin);
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
     * Verify it's not a section mapping. 
     */
    if ((tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_SECTION)
        panic("Translation table entry is a section mapping (tte %x ttep %x)",
              tte, tte_offset);

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

/**
 * mapping_adjust
 */
void mapping_adjust(void)
{
    return;
}

/**
 * mapping_free_prime
 */
void mapping_free_prime(void)
{
    return;
}

/**
 * pmap_map
 *
 * Map specified virtual address range to a physical one.
 */
vm_offset_t pmap_map(vm_offset_t virt, vm_map_offset_t start_addr,
                     vm_map_offset_t end_addr, vm_prot_t prot,
                     unsigned int flags)
{
    int ps;

    ps = PAGE_SIZE;
    while (start_addr < end_addr) {
        pmap_enter(kernel_pmap, (vm_map_offset_t) virt, (start_addr), prot,
                   flags, FALSE, TRUE);
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

        (void) pmap_map_bd(start, phys_addr, phys_addr + round_page(size),
                           VM_PROT_READ | VM_PROT_WRITE, flags);
    } else {
        (void) kmem_alloc_pageable(kernel_map, &start, round_page(size));
        (void) pmap_map(start, phys_addr, phys_addr + round_page(size),
                        VM_PROT_READ | VM_PROT_WRITE, flags);
    }

    return (start);
}

vm_offset_t io_map_spec(vm_map_offset_t phys_addr, vm_size_t size,
                        unsigned int flags)
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
        kprintf
            ("pmap_next_page: ran out of possible pages, last page was 0x%08x",
             first_avail);
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
    kprintf("pmap_virtual_space: VM region 0x%08x - 0x%08x\n", virt_begin,
            virt_end);
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
boolean_t pmap_map_bd(vm_offset_t virt, vm_map_offset_t start,
                      vm_map_offset_t end, vm_prot_t prot, unsigned int flags)
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
void pmap_pageable(__unused pmap_t pmap, __unused vm_map_offset_t start,
                   __unused vm_map_offset_t end, __unused boolean_t pageable)
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
    phys_attribute_set(pn, VM_MEM_MODIFIED);
}

/**
 * pmap_clear_modify
 *
 * Clear the modify bits on the specified physical page.
 */
void pmap_clear_modify(ppnum_t pn)
{
    phys_attribute_clear(pn, VM_MEM_MODIFIED);
}

/**
 * pmap_clear_reference
 *
 * Clear the reference bit on the specified physical page.
 */
void pmap_clear_reference(ppnum_t pn)
{
    phys_attribute_clear(pn, VM_MEM_REFERENCED);
}

/**
 * pmap_set_reference
 *
 * Set the reference bit on the specified physical page.
 */
void pmap_set_reference(ppnum_t pn)
{
    phys_attribute_set(pn, VM_MEM_REFERENCED);
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
    pv_entry_t pv_h;
    int pai;
    spl_t spl;
    boolean_t result;

    assert(phys != vm_page_fictitious_addr);
    if (!pmap_initialized)
        return (TRUE);

    if (!pmap_valid_page(phys))
        return (FALSE);

    SPLVM(spl);

    pv_h = pai_to_pvh(phys);
    result = (pv_h->pv_pmap == PMAP_NULL);
    if (result == FALSE)
        kdb_printf("pmap_verify_free: ppn %p pa %x va %x map %p\n",
                   phys, phys << PAGE_SHIFT, pv_h->pv_address_va,
                   pv_h->pv_pmap);
    SPLX(spl);

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
    return;
}

/**
 * pmap_sync_page_attributes_phys(ppnum_t pa)
 * 
 * Write back and invalidate all cachelines on a physical page.
 */
void pmap_sync_page_attributes_phys(ppnum_t pa)
{
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
    ovbcopy(phys_to_virt(src << PAGE_SHIFT), phys_to_virt(dst << PAGE_SHIFT),
            PAGE_SIZE);
}

/**
 * pmap_copy_part_page
 *
 * Copies the specified (machine independent) pages.
 */
void pmap_copy_part_page(ppnum_t src, vm_offset_t src_offset, ppnum_t dst,
                         vm_offset_t dst_offset, vm_size_t len)
{
    assert((((dst << PAGE_SHIFT) & PAGE_MASK) + dst_offset + len) <= PAGE_SIZE);
    assert((((src << PAGE_SHIFT) & PAGE_MASK) + src_offset + len) <= PAGE_SIZE);

    ovbcopy(phys_to_virt(src << PAGE_SHIFT) + src_offset,
            phys_to_virt(dst << PAGE_SHIFT) + src_offset, len);
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
    return (phys_attribute_test(phys, VM_MEM_MODIFIED));
}

/**
 * pmap_is_referenced
 *
 * Return whether or not the specified physical page is referenced
 * by any physical maps.
 */
boolean_t pmap_is_referenced(vm_offset_t phys)
{
    return (phys_attribute_test(phys, VM_MEM_REFERENCED));
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
    SPLVM(spl);

    /*
     * Get the PTE. 
     */
    ptep = (uint32_t) pmap_pte(pmap, (vm_offset_t) va);
    if (!ptep) {
        ppn = 0;
        goto out;
    }
    pte = (*(uint32_t *) (ptep)) & L1_PTE_ADDR_MASK;

    /*
     * Make sure it's a PTE. 
     */
    if (!((pte) & ARM_PTE_DESCRIPTOR_4K)) {
        ppn = 0;
        goto out;
    }

    ppn = pa_index(pte);
 out:
    /*
     * Return. 
     */
    SPLX(spl);
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
        set_mmu_ttb(new_pmap->pm_l1_phys);
        flush_mmu_tlb();
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
void pmap_map_block(pmap_t pmap, addr64_t va, ppnum_t pa, uint32_t size,
                    vm_prot_t prot, int attr, __unused unsigned int flags)
{
    uint32_t page;
    for (page = 0; page < size; page++) {
        pmap_enter(pmap, va, pa, prot, VM_PROT_NONE, attr, TRUE);
        va += PAGE_SIZE;
        pa++;
    }
}

/**
 * pmap_bootstrap
 *
 * Bootstrap the pmap subsystem.
 */
void pmap_bootstrap(__unused uint64_t msize, vm_offset_t * __first_avail,
                    __unused unsigned int kmapsize)
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

    kprintf("pmap_bootstrap: physical region 0x%08x - 0x%08x\n", first_avail,
            avail_end);

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

    refmod = phys_attribute_test(pn, VM_MEM_MODIFIED | VM_MEM_REFERENCED);

    if (refmod & VM_MEM_MODIFIED)
        retval |= VM_MEM_MODIFIED;
    if (refmod & VM_MEM_REFERENCED)
        retval |= VM_MEM_REFERENCED;

    return (retval);
}

/**
 * pmap_enter
 *
 * Enter pages into a physical map.
 */
void pmap_enter(pmap_t pmap, vm_map_offset_t va, ppnum_t pa, vm_prot_t prot,
                vm_prot_t fault_type, unsigned int flags, boolean_t wired)
{
    pmap_enter_options(pmap, va, pa, prot, fault_type, flags, wired, 0);
}

/**
 * pmap_grab_page
 *
 * Get a page from the global pmap object.
 */
vm_page_t pmap_grab_page(void)
{
    vm_page_t page;
    uint32_t ctr;
    assert(pmap_initialized && kernel_map);

    /*
     * Grab pages from the global VM object.
     */
    while ((page = vm_page_grab()) == VM_PAGE_NULL)
        VM_PAGE_WAIT();

    /*
     * Lock the global object to prevent interruptions.
     */
    vm_object_lock(pmap_object);
    assert((page->phys_page << PAGE_SHIFT) > gPhysBase);
    ctr = (page->phys_page) - (gPhysBase >> PAGE_SHIFT);
    bzero(phys_to_virt(page->phys_page << PAGE_SHIFT), PAGE_SIZE);
    vm_page_insert(page, pmap_object, ctr);

    /*
     * Wire our new page.
     */
    vm_page_lockspin_queues();
    vm_page_wire(page);
    vm_page_unlock_queues();

    /*
     * Done.
     */
    vm_object_unlock(pmap_object);
    return page;
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
    commpage = pmap_grab_page();
    assert(commpage);

    /*
     * And map it.
     */
    pmap_enter(kernel_pmap, (vm_map_offset_t) _COMM_PAGE_BASE_ADDRESS,
               commpage->phys_page, VM_PROT_READ | VM_PROT_WRITE, 0, FALSE,
               TRUE);

    /*
     * Memset it.
     */
    memset((void *) _COMM_PAGE_BASE_ADDRESS, 0x77, PAGE_SIZE);
    return;
}

/**
 * pmap_extract
 *
 * Get the physical address for a virtual one.
 */
vm_offset_t pmap_extract(pmap_t pmap, vm_offset_t virt)
{
    spl_t spl;
    vm_offset_t ppn = 0;
    uint32_t tte, *ttep = pmap_tte(pmap, virt);

    /*
     * Block off all interruptions. Nothing may interrupt the extraction process
     * as the page tables may be changed by another callee to pmap_enter or such.
     */
    SPLVM(spl);
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
    SPLX(spl);
    PMAP_UNLOCK(pmap);
    return ppn;
}

/**
 * pmap_expand
 *
 * Expand the address space of the current physical map.
 */
void pmap_expand(pmap_t map, vm_offset_t v)
{
    vm_offset_t *tte = (vm_offset_t *) pmap_tte(map, v);
    vm_page_t page = pmap_grab_page();
    spl_t spl;

    /*
     * High priority. We do not want any interruptions.
     */
    SPLVM(spl);
    PMAP_LOCK(map);

    /*
     * Do not extend past the commpage. 
     */
    if (v > _COMM_PAGE_BASE_ADDRESS)
        panic("attempting to expand pmap past maximum address of %x\n",
              _COMM_PAGE_BASE_ADDRESS);

    /*
     * L1 section mappings may not be expanded any further.
     */
    if ((*tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_SECTION)
        panic("cannot expand current map into L1 sections");

    /*
     * Overwrite the old L1 mapping in this region with a fresh L2 descriptor.
     */
    *tte =
        ((page->phys_page << PAGE_SHIFT) & L1_PTE_ADDR_MASK) | L1_TYPE_PTE | (1
                                                                              <<
                                                                              4);

 Out:

    /*
     * Flush the TLBs since we updated the page tables.
     */
    flush_mmu_tlb();
    SPLX(spl);
    PMAP_UNLOCK(map);
    return;
}

/**
 * pmap_enter_options
 *
 * Create a translation entry for a PA->VA mappings with additional options.
 * Called from vm_fault.
 */
kern_return_t pmap_enter_options(pmap_t pmap, vm_map_offset_t va, ppnum_t pa,
                                 vm_prot_t prot, vm_prot_t fault_type,
                                 unsigned int flags, boolean_t wired,
                                 unsigned int options)
{
    spl_t spl;
    pt_entry_t pte;
    pv_entry_t pv_e, pv_h;

    pv_e = (pv_entry_t) 0;

    /*
     * Verify the address isn't fictitious.
     */
    assert(pa != vm_page_fictitious_addr);

    /*
     * Only low addresses are supported for user pmaps.
     */
    if (va > _COMM_PAGE_BASE_ADDRESS && pmap != kernel_pmap)
        panic("pmap_enter_options: low address 0x%08X is invalid for pmap %p\n",
              va, pmap);

 Retry:
    /*
     * Set a high priority level. We do not wany any interruptions or any unauthorized
     * page table modification.
     */
    SPLVM(spl);
    PMAP_LOCK(pmap);

    /*
     * Expand the pmap to include the new PTE if necessary to accomodate the new VA we're
     * entering in.
     */
    PMAP_UNLOCK(pmap);
    while ((pte = pmap_pte(pmap, va)) == NULL) {
        SPLX(spl);
        pmap_expand(pmap, va);
        SPLVM(spl);
    }
    PMAP_LOCK(pmap);

    /*
     * If the old page already has a mapping, the caller might be changing protection flags.
     */
    uint32_t old_pte = (*(uint32_t *) pte);

    if ((old_pte & L2_ADDR_MASK) == (pa << PAGE_SHIFT)) {
        /*
         * !!! IMPLEMENT 'pmap_vm_prot_to_page_flags' !!!
         * XXX protection is not implemented right now, all pages are 'RWX'.
         */

        uint32_t template_pte =
            ((pa << PAGE_SHIFT) & L2_ADDR_MASK) | L2_SMALL_PAGE;
        template_pte |= pmap_vm_prot_to_page_flags(pmap, prot, wired, 0);

        if (va == _COMM_PAGE_BASE_ADDRESS)
            template_pte |= L2_ACCESS_USER;

        *(uint32_t *) pte = template_pte;

        /*
         * The work here is done, the PTE will now have new permissions. Flush the TLBs for the
         * specific VA and then exit.
         */
        goto enter_options_done;
    }

    /*
     * This is a new mapping, add it to the pv_head_table if pmap is initialized. This is so
     * we can correctly manage our entries.
     */
    if (pmap_initialized) {
        ppnum_t pai;

        /*
         * Get the pv_h for the specified page index.
         */
        pai = (pa);
        pv_h = pai_to_pvh(pai);

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

        /*
         * This is definitely a new mapping.
         */
        if (pv_h->pv_pmap == PMAP_NULL) {
            pv_h->pv_address_va = va;
            pv_h->pv_pmap = pmap;
            pv_h->pv_next = (pv_entry_t) 0;
            if (wired)
                pv_h->pv_flags |= VM_MEM_WIRED;
        } else {
            /*
             * Make sure there's no aliased mapping. There should never be another
             * aliased one with an identical pmap and VA.
             */
            pv_entry_t e = pv_h;
            while (e != (pv_entry_t) 0) {
                if (e->pv_pmap == pmap && e->pv_address_va == va)
                    panic("pmap_enter_options: duplicate pv_h %p", pv_h);
                e = e->pv_next;
            }

            /*
             * Add a new pv_e. This is a leaf mapping. See the beginning of the file
             * as to the purpose of the pv_e/pv_h paradigm, it may help a bit.
             */
            if (pv_e == (pv_entry_t) 0) {
                /*
                 * No new pv_e, grab a new one from the Zone.
                 */
                pv_e = (pv_entry_t) zalloc(pve_zone);
                if (!pv_e) {
                    panic("pmap_enter_options: failed to grab a leaf node\n");
                }
#if 0
                kprintf
                    ("pmap_enter_options: PV_GRAB_LEAF(), leaf node %p, head node %p\n",
                     pv_e, pv_h);
#endif
                goto Retry;
            }

            pv_e->pv_address_va = va;
            pv_e->pv_pmap = pmap;
            pv_e->pv_next = pv_h->pv_next;
            if (wired)
                pv_e->pv_flags |= VM_MEM_WIRED;
            pv_h->pv_next = pv_e;
            pv_e = (pv_entry_t) 0;
        }
#if 0
        kprintf
            ("pmap_enter: pai %d pa %d (%x) va %x pv_h %p pmap %p pv_h->pmap %p pv_h->pv_address_va %x\n",
             pai, pa, pa << PAGE_SHIFT, va, pv_h, pmap, pv_h->pv_pmap,
             pv_h->pv_address_va);
#endif
    }

    /*
     * Enter and count the mapping.
     */
    pmap->pm_stats.resident_count++;
    if (wired)
        pmap->pm_stats.wired_count++;

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
     * XXX add cacheability flags 
     */
    if (flags & VM_MEM_NOT_CACHEABLE) {
        /*
         * xxx arm 
         */
        template_pte |= mmu_texcb_small(MMU_DMA);
    } else if (flags & VM_MEM_COHERENT) {
        /*
         * Writethrough cache by default. 
         */
        template_pte |= mmu_texcb_small(MMU_CODE);
    } else {
        /*
         * Writethrough cache by default. 
         */
        template_pte |= mmu_texcb_small(MMU_DMA);
    }
    *(uint32_t *) pte = template_pte;

 enter_options_done:
    /*
     * Done, now invalidate the TLB for a single page.
     */
    flush_mmu_single(va);

    /*
     * The operation has completed successfully.
     */
    SPLX(spl);
    PMAP_UNLOCK(pmap);

    return KERN_SUCCESS;
}

/**
 * pmap_init
 *
 * Stage 2 initialization of the pmap subsystem.
 */
void pmap_init(void)
{
    vm_offset_t pv_root;
    vm_size_t s;
    int i;

    kprintf("pmap_init: %d physical pages in memory, kernel pmap at %p\n",
            (mem_size / PAGE_SIZE), kernel_pmap);

    /*
     * Allocate the core PV structure. The pv_head_table contains trunk entries
     * for every physical page that exists in the system.
     */
    s = (mem_size / PAGE_SIZE) * sizeof(pv_entry);
    if (kernel_memory_allocate
        (kernel_map, &pv_root, s, 0,
         KMA_KOBJECT | KMA_PERMANENT) != KERN_SUCCESS)
        panic("pmap_init(): failed to allocate pv table!");

    /*
     * Okay. Zero out the PV head table.
     */
    pv_head_table = (pv_entry_t) pv_root;
    kprintf("pmap_init: pv_head_table at %p\n", pv_head_table);
    bzero((void *) pv_head_table, s);

    /*
     * Initialize the Zones for object allocation. 
     */
    pmap_zone =
        zinit((sizeof(struct pmap)), 400 * (sizeof(struct pmap)), 4096,
              "pmap_pmap");

    /*
     * Expandable zone. (pv_entry zone)
     */
    pve_zone =
        zinit((sizeof(struct __pv_entry__)),
              10000 * (sizeof(struct __pv_entry__)), 4096, "pmap_pve");

    /*
     * Initialize the free list lock. (unused right now.)
     */
    simple_lock_init(&pv_free_list_lock, 0);

    /*
     * Set up the core VM object.
     */
    pmap_object = &pmap_object_store;
    _vm_object_allocate(mem_size, &pmap_object_store);

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
void pmap_remove_range(pmap_t pmap, vm_map_offset_t start_vaddr,
                       pt_entry_t * spte, pt_entry_t * epte, boolean_t is_sect)
{
    pt_entry_t *cpte = spte;
    vm_map_offset_t vaddr;
    vm_size_t our_page_size = (is_sect) ? (1 * 1024 * 1024) : PAGE_SIZE;
    int num_removed = 0, num_unwired = 0;

    /*
     * xxx PLEASE FIX THIS FIX FIX FIX FIX 
     */

    /*
     * Make sure the Cpte/Epte are within sane boundaries. (256 entries, one L2 area size.)
     */
    if (((vm_offset_t) epte - (vm_offset_t) cpte) > L2_SIZE)
        kprintf
            ("pmap_remove_range: attempting to remove more ptes than 256!\n");

    for (cpte = spte, vaddr = start_vaddr; cpte < epte;
         cpte++, vaddr += our_page_size) {
        /*
         * Start nuking the range. 
         */
        pt_entry_t *p = cpte;

        /*
         * Get the index for the PV table.
         */
        ppnum_t pai = pa_index(*cpte & L2_ADDR_MASK);
        num_removed++;

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
         * Remove the entry from the PV table.
         */
        {
            pv_entry_t pv_h, prev, cur;
            pv_h = pai_to_pvh(pai);

            /*
             * It's possible that there were pages that were initialized before 'pmap' went up. 
             * This hack needs to DIE.
             */

            if (pv_h->pv_pmap == PMAP_NULL) {
                /*
                 * kprintf("pmap_remove_range: null pv_h->pmap (pmap %p, pv_h %p, pai %d, vaddr %x, cpte %x)\n", pmap, pv_h, pai, vaddr, cpte); 
                 */
                continue;
            }

            /*
             * XXX ARM 
             */
            if (pv_h->pv_address_va == vaddr && pv_h->pv_pmap == pmap) {
                cur = pv_h->pv_next;
                if (cur != (pv_entry_t) 0) {
                    *pv_h = *cur;
                    zfree(pve_zone, cur);
                } else {
                    pv_h->pv_pmap = PMAP_NULL;
                    pv_h->pv_address_va = 0;
                }
            } else {
#if 0                           /* hack */
                cur = pv_h;
                while (cur->pv_address_va != vaddr || cur->pv_pmap != pmap) {
                    prev = cur;
                    if ((cur = prev->pv_next) == (pv_entry_t) 0) {
                        panic("pmap_remove_ranges: mapping not in pv_list!\n");
                    }
                }
                prev->pv_next = cur->pv_next;
                zfree(pve_zone, cur);
#endif
            }
        }
    }
 out:
    /*
     * Invalidate all TLBs.
     */
    flush_mmu_tlb();

    /*
     * Make sure the amount removed isn't... weird.
     */
#if 0                           /* hack */
    assert(pmap->pm_stats.resident_count >= num_removed);
    OSAddAtomic(-num_removed, &pmap->pm_stats.resident_count);
    assert(pmap->pm_stats.wired_count >= num_unwired);
    OSAddAtomic(-num_unwired, &pmap->pm_stats.wired_count);
#endif

    return;
}

/**
 * pmap_remove
 *
 * Remove the given range of addresses from the specified map.
 */
void pmap_remove(pmap_t map, vm_offset_t s, vm_offset_t e)
{
    spl_t spl;
    vm_offset_t tte;
    vm_offset_t *spte, *epte, l = s;

    /*
     * Verify the pages are page aligned.
     */
    assert(!(s & PAGE_MASK));
    assert(!(e & PAGE_MASK));

    /*
     * High Priority. Nothing may interrupt the removal process.
     */
    PMAP_LOCK(map);
    SPLVM(spl);

    /*
     * Grab the TTE.
     */
    tte = pmap_tte(map, l);

    while (s < e) {
        l = (s + (1 * 1024 * 1024)) & ~((1 * 1024 * 1024) - 1);
        if (l > e)
            l = e;
        if (tte && ((*(vm_offset_t *) tte & ARM_PAGE_MASK_VALUE) != 0)) {
            spte = (pt_entry_t *)
                phys_to_virt((*(vm_offset_t *) tte & L1_PTE_ADDR_MASK));
            spte = &spte[((s >> PAGE_SHIFT) & 0x3ff)];
            epte = &spte[((l - s) >> PAGE_SHIFT)];
            pmap_remove_range(map, s, spte, epte, FALSE);
        }
        s = l;
        tte++;
    }

    /*
     * Flush TLBs since we modified page table entries.
     */
    flush_mmu_tlb();

    /*
     * Return. 
     */
    SPLX(spl);
    PMAP_UNLOCK(map);
    return;
}

/**
 * pmap_create
 *
 * Create a pmap.
 */
pmap_t pmap_create(ledger_t ledger, vm_map_size_t size,
                   __unused boolean_t is_64bit)
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
    pmap_common_init(our_pmap);

    /*
     * Grab a new page and set the new L1 region.
     */
    new_l1 = pmap_grab_page();
    our_pmap->pm_l1_phys = new_l1->phys_page << PAGE_SHIFT;
    our_pmap->pm_l1_virt = phys_to_virt(new_l1->phys_page << PAGE_SHIFT);
    bzero(phys_to_virt(new_l1->phys_page << PAGE_SHIFT), PAGE_SIZE);

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
    pv_entry_t pv_h, prev, pv_e;

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
     * Set a high priority level. No interruptions allowed.
     */
    SPLVM(spl);

    /*
     * Walk down the PV listings and remove the entries.
     */
    pv_h = pai_to_pvh(pn);

    if (pv_h->pv_pmap != PMAP_NULL) {
        prev = pv_e = pv_h;
        do {
            vm_map_offset_t vaddr;
            pt_entry_t pte;
            pmap_t pmap;

            pmap = pv_e->pv_pmap;
            vaddr = pv_e->pv_address_va;
            pte = pmap_pte(pmap, vaddr);
            if (!pte) {
                kprintf("pmap_page_protect pmap 0x%x pn 0x%x vaddr 0x%llx\n",
                        pmap, pn, vaddr);
                panic("pmap_page_protect");
            }

            /*
             * Remove the mapping if a new protection is NONE or if write-protecting a kernel mapping.
             */
            if (remove || pmap == kernel_pmap) {
                /*
                 * Remove the mapping by nuking it and then flushing the TLBs.
                 */
                *(pt_entry_t *) pte = 0;
                flush_mmu_single(vaddr);
                pv_h->pv_flags &= (VM_MEM_MODIFIED | VM_MEM_REFERENCED);

                /*
                 * Decrement the resident count.
                 */
                OSAddAtomic(-1, &pmap->pm_stats.resident_count);

                /*
                 * Remove the PV entry.
                 */
                if (pv_e == pv_h) {
                    /*
                     * Fix up head later. 
                     */
                    pv_h->pv_pmap = PMAP_NULL;
                } else {
                    /*
                     * Free the leaf node.
                     */
                    prev->pv_next = pv_e->pv_next;
                    zfree(pve_zone, pv_e);
                }
            } else {
                /*
                 * Write protect the mapping and flush the TLBs for it.
                 */
                Debugger("pmap_page_protect: Write protect mapping");
#if 0
                *(pt_entry_t) |= (1 << 11); /* XXX FIX */
#endif
                flush_mmu_single(vaddr);
                prev = pv_e;
            }
        } while ((pv_e = prev->pv_next) != (pv_entry_t) 0);

        /*
         * If the pv_head mapping was removed, fix it up. 
         */
        if (pv_h->pv_pmap == PMAP_NULL) {
            pv_e = pv_h->pv_next;
            if (pv_e != (pv_entry_t) 0) {
                *pv_e = *pv_h;
                zfree(pve_zone, pv_e);
            }
        }
    }

    /*
     * Return. 
     */
    SPLX(spl);
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
     * Lock the VM object. 
     */
    vm_object_lock(pmap_object);

    /*
     * Look up the page.
     */
    m = vm_page_lookup(pmap_object,
                       (vm_object_offset_t) ((ttb_base >> PAGE_SHIFT) -
                                             (gPhysBase >> PAGE_SHIFT)));
    assert(m);

    /*
     * Got it, now free it.
     */
    VM_PAGE_FREE(m);

    /*
     * Done.
     */
    vm_object_unlock(pmap_object);

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
    int refcnt;

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
    SPLVM(spl);

    /*
     * Okay, decrease the reference count.
     */
    refcnt = --pmap->pm_refcnt;
    if (refcnt == 0) {
        /*
         * We might be using this pmap, invalidate all TLBs.
         */
        flush_mmu_tlb();
    }

    /*
     * Unlock the pmap system.
     */
    SPLX(spl);
    PMAP_UNLOCK(pmap);

    /*
     * If the pmap still has a reference count, we don't kill it.
     */
    if (refcnt != 0) {
        return;
    }

    /*
     * xxx we need to free pages from an expanded pmap.
     */

    /*
     * Free the associated objects with the pmap first.
     */
    pmap_deallocate_l1(pmap);
    ledger_dereference(pmap->ledger);

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
 *
 * Lower the specified protections on a certain map from sva to eva using prot prot.
 */
void pmap_protect(pmap_t map, vm_map_offset_t sva, vm_map_offset_t eva,
                  vm_prot_t prot)
{
    /*
     * xxx finish 
     */
    return;
}

/**
 * pmap_nest
 *
 * Nest a pmap with new mappings into a master pmap.
 */
kern_return_t pmap_nest(pmap_t subord, pmap_t grand, addr64_t va_start,
                        addr64_t nstart, uint64_t size)
{
    int copied;

    /*
     * Anounce ourselves. We are nesting one pmap inside another.
     */
    kprintf("pmap_nest: %p[0x%08llx] => %p[0x%08llx], %d tte entries\n", grand,
            va_start, subord, nstart, size >> 20);

    /*
     * Sanity checks.
     */
    if (size == 0) {
        panic("pmap_nest: size is invalid - %016llX\n", size);
    }

    if (va_start != nstart)
        panic("pmap_nest: va_start(0x%llx) != nstart(0x%llx)\n", va_start,
              nstart);

    /*
     * Start the copy operations.
     */
    spl_t spl;
    SPLVM(spl);
    PMAP_LOCK(grand);
    PMAP_LOCK(subord);

    /*
     * Mark the surbodinate pmap as shared.
     */
    subord->pm_shared = TRUE;
    bcopy(grand->pm_l1_virt + tte_offset(va_start),
          subord->pm_l1_virt + tte_offset(nstart),
          (size >> 20) * sizeof(uint32_t));

    /*
     * Out. Flush all TLBs.
     */
    flush_mmu_tlb();
    SPLX(spl);
    PMAP_UNLOCK(grand);
    PMAP_UNLOCK(subord);

    return KERN_SUCCESS;
}

/**
 * pmap_unnest
 *
 * Remove a nested pmap.
 */
kern_return_t pmap_unnest(pmap_t grand, addr64_t vaddr, uint64_t size)
{
    panic("pmap_unnest");
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
