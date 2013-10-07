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
 * Version 1.2b1, 'The Rewrite'.
 *
 * I'm sorry. This pmap sucks, but it sucks 'less' than the previous one did.
 *
 * Todo: fix pmap_nest, pmap_copy, pmap_remove, pmap_destroy, pmap_protect, pmap_page_protect, pmap_disconnect,
 *           pmap_enter_options.
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
    struct __pv_entry__     *pv_next;        /* Next PV entry. */
    pmap_t                  pv_pmap;         /* Where does our mapping lie? */
    vm_offset_t             pv_address_va;   /* Virtual Address for the mapping. */
    uint32_t                pv_flags;        /* Pmap Flags */
} pv_entry, *pv_entry_t;

typedef enum {
    ARM_PAGE_TRANSLATION_FAULT   = 0x00,          /* 0b00 */
    ARM_PAGE_PAGE_TABLE          = 0x01,          /* 0b01 */
    ARM_PAGE_SECTION             = 0x02,          /* 0b10 */
    ARM_PAGE_MASK_VALUE          = 0x03,          /* 0b11 */
} pmap_arm_l1_page_types_t;

typedef enum {
    ARM_PTE_DESCRIPTOR_64K      = 0x01,              /* 0b01 */
    ARM_PTE_DESCRIPTOR_4K       = 0x02,              /* 0b1X */
} pmap_arm_l2_page_types_t;

/** Global variables */
boolean_t                   pmap_initialized    = FALSE;    /* Is the pmap system initialized? */
static struct vm_object     pmap_object_store;              /* Storage object for the actual VM thing. */
vm_object_t                 pmap_object;                    /* The real VM object. */
extern uint32_t             first_avail, avail_end;         /* End/begin of Managed RAM space. */
struct zone                 *pmap_zone;                     /* Zone of pmap structures */
struct zone                 *pve_zone;                      /* Pmap Virtual Entry zone. */
pv_entry_t                  pv_head_table;                  /* Start of PV entries. */
static pmap_paddr_t         avail_remaining;                /* Remaining avaialable pages. */
uint32_t                    virt_begin, virt_end;           /* Virtual Address Space. */
uint32_t                    avail_start, vm_first_phys;

uint64_t                    pmap_nesting_size_min = 0x8000000;
uint64_t                    pmap_nesting_size_max = 0x8000000;

/* THE kernel pmap. */
struct pmap                 kernel_pmap_store;
pmap_t                      kernel_pmap =       &kernel_pmap_store;

/** Locking Primitives */
lock_t                      pmap_system_lock;
#define SPLVM(spl)          spl = splhigh();
#define SPLX(spl)           splx(spl);

/** Useful Macros */
#define pa_index(pa)        (atop(pa))
#define pai_to_pvh(pai)     (&pv_head_table[pai - atop(gPhysBase)])

/** The Free List. */
pv_entry_t      pv_free_list;       /* The free list should be populated when the pmaps are not locked. */
decl_simple_lock_data(,pv_free_list_lock);

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

/** Functions */

/**
 * phys_attribute_clear and friends. These suck.
 */
void
phys_attribute_clear(ppnum_t pn, int bits)
{
    int pai;
    pv_entry_t pv_h;

    assert(pn != vm_page_fictitious_addr);

    pv_h = pai_to_pvh(pn);
    pv_h->pv_flags &= ~bits;

    return;
}

int
phys_attribute_test(ppnum_t pn, int bits)
{
    int pai;
    pv_entry_t pv_h;

    assert(pn != vm_page_fictitious_addr);

    pv_h = pai_to_pvh(pn);
    if((pv_h->pv_flags & bits) == bits)
        return bits;

    return (pv_h->pv_flags & bits);
}

void
phys_attribute_set(ppnum_t pn, int bits)
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
boolean_t pmap_adjust_unnest_parameters(pmap_t p, vm_map_offset_t *s, vm_map_offset_t *e)
{
    return FALSE;
}

/**
 * pmap_attributes
 *
 * Set/Get special memory attributes; Set/Get is not implemented.
 */
kern_return_t
pmap_attribute(pmap_t pmap, vm_offset_t address, vm_size_t size, vm_machine_attribute_t atte, vm_machine_attribute_val_t* attrp)
{
    return KERN_INVALID_ARGUMENT;
}

/**
 * pmap_attribute_cache_sync
 *
 * Flush appropriate cache based on page number sent.
 */
kern_return_t
pmap_attribute_cache_sync(ppnum_t pn, vm_size_t size, vm_machine_attribute_t attr, vm_machine_attribute_val_t *attrp)
{
    return KERN_SUCCESS;
}

/**
 * pmap_cache_attributes
 */
unsigned int 
pmap_cache_attributes(ppnum_t pn)
{
    panic("pmap_cache_attributes");
}

/**
 * pmap_clear_noencrypt
 */
void
pmap_clear_noencrypt(ppnum_t pn)
{
    return;
}

/**
 * pmap_is_noencrypt
 */
boolean_t
pmap_is_noencrypt(ppnum_t pn)
{
    return FALSE;
}

/**
 * pmap_set_noencrypt
 */
void 
pmap_set_noencrypt(ppnum_t pn)
{
    return;
}

/**
 * pmap_set_cache_attributes
 *
 * Set the specified cache attributes.
 */
void
pmap_set_cache_attributes(ppnum_t pn, unsigned int cacheattr)
{
    return;
}

/**
 * compute_pmap_gc_throttle
 */
void
compute_pmap_gc_throttle(void *arg __unused)
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
vm_offset_t
pmap_tte(pmap_t pmap, vm_offset_t virt)
{
    uint32_t tte_offset_begin;
    tte_offset_begin = pmap->pm_l1_virt;
    if((tte_offset_begin + L1_SIZE) < addr_to_tte(pmap->pm_l1_virt, virt))
        panic("Translation table entry extends past L1 size (base: 0x%08X)", tte_offset_begin);
    return addr_to_tte(pmap->pm_l1_virt, virt);
}

/**
 * pmap_pte
 */
vm_offset_t
pmap_pte(pmap_t pmap, vm_offset_t virt)
{
    uint32_t *tte_offset = (uint32_t*)pmap_tte(pmap, virt);
    uint32_t tte, pte, *ptep;

    /* Get the translation-table entry. */
    assert(tte_offset);
    tte = *tte_offset;

    /* Verify it's not a section mapping. */
    if((tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_SECTION)
        panic("Translation table entry is a section mapping");

    /* Clean the TTE bits off, get the address. */
    pte = L1_PTE_ADDR(tte);
    if(!pte)
        return 0;

    /* Return the virtual mapped PTE. */
    ptep = (uint32_t*)((phys_to_virt(pte) + pte_offset(virt)));

    /* Only 4kB pages are supported. */
    if(*ptep & ARM_PTE_DESCRIPTOR_64K)
        return 0;

    return (ptep);
}

/**
 * mapping_adjust
 */ 
void
mapping_adjust(void)
{
    return;
}

/**
 * mapping_free_prime
 */
void
mapping_free_prime(void)
{
    return;
}

/**
 * pmap_map
 *
 * Map specified virtual address range to a physical one.
 */
vm_offset_t
pmap_map(vm_offset_t virt,
         vm_map_offset_t start_addr,
         vm_map_offset_t end_addr,
         vm_prot_t prot,
         unsigned int flags)
{
    int        ps;
    
    ps = PAGE_SIZE;
    while (start_addr < end_addr) {
        pmap_enter(kernel_pmap, (vm_map_offset_t)virt, (start_addr), prot, flags, FALSE, TRUE);
        virt += ps;
        start_addr += ps;
    }
    return(virt);
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

    /* Make sure the page is valid. */
    if(((p << PAGE_SHIFT) < avail_start) || ((p << PAGE_SHIFT) > avail_end))
        panic("pmap_zero_page: zeroing a non-managed page, ppnum %d", p);

    bzero(phys_to_virt(p << PAGE_SHIFT), PAGE_SIZE);
}

/**
 * pmap_clear_refmod
 *
 * Clears the referenced and modified bits as specified by the mask
 * of the specified physical page.
 */
void
pmap_clear_refmod(ppnum_t pn, unsigned int mask) {
    phys_attribute_clear(pn, mask);
}

/**
 * io_map
 *
 * Maps an IO region and returns its virtual address.
 */
vm_offset_t
io_map(vm_offset_t phys_addr, vm_size_t size, unsigned int flags)
{
    vm_offset_t start;

    if (kernel_map == VM_MAP_NULL) {
        /*
         * VM is not initialized.  Grab memory.
         */
        start = virt_begin;
        virt_begin += round_page(size);

        (void) pmap_map_bd(start, phys_addr, phys_addr + round_page(size),
                   VM_PROT_READ|VM_PROT_WRITE,
                   flags);
    }
    else {
        (void) kmem_alloc_pageable(kernel_map, &start, round_page(size));
        (void) pmap_map(start, phys_addr, phys_addr + round_page(size),
                VM_PROT_READ|VM_PROT_WRITE,
                flags);
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
boolean_t pmap_next_page(ppnum_t *addrp)
{
    if(first_avail >= avail_end) {
        kprintf("pmap_next_page: ran out of possible pages, last page was 0x%08x", first_avail);
        return FALSE;
    }
    
    *addrp = pa_index(first_avail);    

    /* We lost a page. */
    first_avail += PAGE_SIZE;
    avail_remaining--;
    return TRUE;
}

/**
 * pmap_virtual_space
 *
 * Get virtual space parameters.
 */
void
pmap_virtual_space(vm_offset_t *startp,
                   vm_offset_t *endp)
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
boolean_t pmap_map_bd(vm_offset_t virt,
                      vm_map_offset_t start,
                      vm_map_offset_t end,
                      vm_prot_t prot,
                      unsigned int flags)
{
    spl_t   spl;

    /* Verify the start and end are page aligned. */
    assert(!(start & PAGE_MASK));
    assert(!(end & PAGE_MASK));

    /* Disable interrupts and start mapping pages */
    SPLVM(spl);

    /* Write the PTEs to memory. */
    uint32_t ptep = (uint32_t)(pmap_pte(kernel_pmap, virt));
    if(!ptep)
        panic("pmap_map_bd: Invalid kernel address");

    /* Map the pages. */
    l2_map_linear_range_no_cache(virt_to_phys(ptep), start, end);

    /* Return. */
    SPLX(spl);

    return TRUE;
}

/**
 * pmap_pageable
 */
void
pmap_pageable(__unused pmap_t pmap,
              __unused vm_map_offset_t start,
              __unused vm_map_offset_t end,
              __unused boolean_t pageable)
{
    return;
}

/**
 * pmap_set_modify
 *
 * Set the modify bit on the specified physical page.
 */
void
pmap_set_modify(ppnum_t pn)
{
    phys_attribute_set(pn, VM_MEM_MODIFIED);
}

/**
 * pmap_clear_modify
 *
 * Clear the modify bits on the specified physical page.
 */
void
pmap_clear_modify(ppnum_t pn)
{
    phys_attribute_clear(pn, VM_MEM_MODIFIED);
}

/**
 * pmap_clear_reference
 *
 * Clear the reference bit on the specified physical page.
 */
void
pmap_clear_reference(ppnum_t pn)
{
    phys_attribute_clear(pn, VM_MEM_REFERENCED);
}

/**
 * pmap_set_reference
 *
 * Set the reference bit on the specified physical page.
 */
void
pmap_set_reference(ppnum_t pn)
{
    phys_attribute_set(pn, VM_MEM_REFERENCED);
}

/**
 * pmap_valid_page
 *
 * Is the page inside the managed zone?
 */
boolean_t
pmap_valid_page(ppnum_t p)
{
    return (((p << PAGE_SHIFT) > avail_start) && ((p << PAGE_SHIFT) < avail_end));
}

/**
 * pmap_verify_free
 *
 * Verify that the page has no mappings.
 */
boolean_t
pmap_verify_free(vm_offset_t phys)
{
    pv_entry_t  pv_h;
    int     pai;
    spl_t       spl;
    boolean_t   result;

    assert(phys != vm_page_fictitious_addr);
    if (!pmap_initialized)
        return(TRUE);

    if (!pmap_valid_page(phys))
        return(FALSE);

    SPLVM(spl);

    pv_h = pai_to_pvh(phys);
    result = (pv_h->pv_pmap == PMAP_NULL);
    SPLX(spl);

    return(result);
}

/**
 * pmap_sync_page_data_phys
 * 
 * Invalidates all of the instruction cache on a physical page and
 * pushes any dirty data from the data cache for the same physical page
 */
void
pmap_sync_page_data_phys(__unused ppnum_t pa)
{
    return;
}

/**
 * pmap_sync_page_attributes_phys(ppnum_t pa)
 * 
 * Write back and invalidate all cachelines on a physical page.
 */
void
pmap_sync_page_attributes_phys(ppnum_t pa)
{
    return;
}

/*
 * Statistics routines
 */
int
pmap_resident_max(pmap_t pmap)
{
    return ((pmap)->pm_stats.resident_max);
}

int
pmap_resident_count(pmap_t pmap)
{
    return ((pmap)->pm_stats.resident_count);
}

/**
 * pmap_disable_NX
 *
 * Disable NX on a specified pmap.
 */
void
pmap_disable_NX(pmap_t pmap)
{
    panic("pmap_disable_NX not implemented\n");
}

/**
 * pmap_zero_page
 *
 * pmap_copy_page copies the specified (machine independent)
 * page from physical address src to physical address dst.
 */
void
pmap_copy_page(ppnum_t src, ppnum_t dst)
{
    ovbcopy(phys_to_virt(src << PAGE_SHIFT), phys_to_virt(dst << PAGE_SHIFT), PAGE_SIZE);
}

/**
 * pmap_copy_part_page
 *
 * Copies the specified (machine independent) pages.
 */
void
pmap_copy_part_page(
    ppnum_t src,
    vm_offset_t src_offset,
    ppnum_t dst,
    vm_offset_t dst_offset,
    vm_size_t   len
    )
{
    assert((((dst << PAGE_SHIFT) & PAGE_MASK) + dst_offset + len) <= PAGE_SIZE);
    assert((((src << PAGE_SHIFT) & PAGE_MASK) + src_offset + len) <= PAGE_SIZE);

    ovbcopy(phys_to_virt(src << PAGE_SHIFT) + src_offset, 
            phys_to_virt(dst << PAGE_SHIFT) + src_offset, 
            len);
}

/**
 * pmap_common_init
 *
 * Initialize common elements of pmaps.
 */
void pmap_common_init(pmap_t pmap)
{
    usimple_lock_init(&pmap->lock, 0);
    if(pmap->ledger)
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
boolean_t
pmap_is_modified(vm_offset_t phys)
{
    return (phys_attribute_test(phys, VM_MEM_MODIFIED));
}

/**
 * pmap_is_referenced
 *
 * Return whether or not the specified physical page is referenced
 * by any physical maps.
 */
boolean_t
pmap_is_referenced(vm_offset_t phys)
{
    return (phys_attribute_test(phys, VM_MEM_REFERENCED));
}

/**
 * pmap_list_resident_pages
 */
int
pmap_list_resident_pages(pmap_t pmap, vm_offset_t *listp, int space)
{
    return 0;
}


/**
 * pmap_find_phys
 *
 * pmap_find_phys returns the (4K) physical page number containing a
 * given virtual address in a given pmap.
 */
ppnum_t
pmap_find_phys(pmap_t pmap, addr64_t va)
{
    spl_t spl;
    uint32_t ptep, pte, ppn;

    /* Raise priority level. */
    SPLVM(spl);

    /* Get the PTE. */
    ptep = (uint32_t)pmap_pte(pmap, (vm_offset_t)va);
    if(!ptep) {
        ppn = 0;
        goto out;
    }
    pte = (*(uint32_t*)(ptep)) & L1_PTE_ADDR_MASK;

    /* Make sure it's a PTE. */
    if(!((pte) & ARM_PTE_DESCRIPTOR_4K)) {
        ppn = 0;
        goto out;
    }

    ppn = pa_index(pte);
out:
    /* Return. */
    SPLX(spl);
    return ppn;
}

/**
 * pmap_switch
 *
 * Switch the current user pmap to a new one.
 */
void
pmap_switch(pmap_t new_pmap)
{
    spl_t spl;

    /* Raise priority level. */
    SPLVM(spl);

    /* Make sure it's not the kernel pmap. */
    if(new_pmap == kernel_pmap)
        goto switch_return;

    /* Switch it if needed. */
    if(current_cpu_datap()->user_pmap == new_pmap) {
        goto switch_return;
    } else {
        current_cpu_datap()->user_pmap = new_pmap;
        set_mmu_ttb(new_pmap->pm_l1_phys);
        flush_mmu_tlb();
    }

    /* Done. */
switch_return:
    SPLX(spl);
    return;
}


/**
 * pmap_map_block
 *
 * Map a (possibly) autogenned block
 */
void
pmap_map_block(
    pmap_t      pmap, 
    addr64_t    va,
    ppnum_t     pa,
    uint32_t    size,
    vm_prot_t   prot,
    int     attr,
    __unused unsigned int   flags)
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
void pmap_bootstrap(__unused uint64_t msize,
                    vm_offset_t* __first_avail,
                    __unused unsigned int kmapsize)
{
    /* Set the first virtual address we can use. */
    virt_begin = *__first_avail;

    /* Make sure we don't go to the ARM Vector Table. */
    virt_end = vm_last_addr = 0xFFFFEFFF;

    /* Set the available page amount. */
    avail_remaining = (avail_end - first_avail) >> PAGE_SHIFT;
    vm_first_phys = gPhysBase;
    avail_start = gPhysBase;

    kprintf("pmap_bootstrap: physical region 0x%08x - 0x%08x\n", first_avail, avail_end);

    /* Initialize kernel pmap. */
    pmap_static_init();
}

/**
 * pmap_reference
 *
 * Increment reference count of the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{
    if (pmap != PMAP_NULL)
        (void)hw_atomic_add(&pmap->pm_refcnt, 1); /* Bump the count */
}

/**
 * pmap_get_refmod
 *
 * Returns the referenced and modified bits of the specified
 * physical page.
 */
unsigned int
pmap_get_refmod(ppnum_t pn)
{
    int                 refmod;
    unsigned int        retval = 0;

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
void
pmap_enter(pmap_t pmap, vm_map_offset_t va, ppnum_t pa, vm_prot_t prot,
           vm_prot_t fault_type, unsigned int flags, boolean_t wired)
{
    pmap_enter_options(pmap, va, pa, prot, fault_type, flags, wired, 0);
}

/**
 * pmap_grab_page
 *
 * Get a page from the global pmap object.
 */
vm_page_t
pmap_grab_page(void)
{
    vm_page_t page;
    uint32_t ctr;
    assert(pmap_initialized && kernel_map);

    /* Grab pages. */
    while((page = vm_page_grab()) == VM_PAGE_NULL)
        VM_PAGE_WAIT();

    /* Lock the global object. */
    vm_object_lock(pmap_object);
    assert((page->phys_page << PAGE_SHIFT) > gPhysBase);
    ctr = (page->phys_page) - (gPhysBase >> PAGE_SHIFT);
    bzero(phys_to_virt(page->phys_page << PAGE_SHIFT), PAGE_SIZE);
    vm_page_insert(page, pmap_object, ctr);

    /* Wire it. */
    vm_page_lockspin_queues();
    vm_page_wire(page);
    vm_page_unlock_queues();

    /* Done */
    vm_object_unlock(pmap_object);
    return page;
}

/**
 * pmap_create_sharedpage
 *
 * Create the system common page.
 */
void
pmap_create_sharedpage(void)
{
    vm_page_t page = pmap_grab_page();
    assert(page);
    /* Map it. */
    pmap_enter(kernel_pmap, (vm_map_offset_t)_COMM_PAGE_BASE_ADDRESS, page->phys_page, 0, 0,
               FALSE, FALSE);
    return;
}

/**
 * pmap_extract
 *
 * Get the physical address for a virtual one.
 */
vm_offset_t
pmap_extract(pmap_t pmap, vm_offset_t virt) {
    spl_t spl;
    vm_offset_t ppn = 0;
    uint32_t tte, *ttep = pmap_tte(pmap, virt);

    /* Block off all interruptions. */
    SPLVM(spl);
    if(!ttep)
        goto extract_out;

    /* Look at the TTE and see what type of mapping it is. */
    tte = *ttep;

    /* Verify it's not a section mapping. */
    if((tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_SECTION) {
        /* Clean the lower bits off. */
        ppn = (tte & L1_SECT_ADDR_MASK);

        /* Now add the lower bits back from the VA. */
        ppn |= (virt & ~(L1_SECT_ADDR_MASK));

        /* Done. */
        goto extract_out;
    } else if((tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_PAGE_TABLE) {
        uint32_t pte, *ptep;

        /* Clean the TTE bits off, get the address. */
        pte = L1_PTE_ADDR(tte);
        if(!pte)
            goto extract_out;

        /* Return the virtual mapped PTE. */
        ptep = (uint32_t*)((phys_to_virt(pte) + pte_offset(virt)));

        /* Make sure it's not a large page. */
        if(((*ptep) & ARM_PTE_DESCRIPTOR_64K))
            panic("pmap_extract: 64kb pages not supported yet");

        /* Clean the PTE bits off. */
        ppn = (*ptep) & L2_ADDR_MASK;

        /* Now, add the lower bits back from the VA. */
        ppn |= (virt & ~(L2_ADDR_MASK));

        /* Done. */
        goto extract_out;
    } else {
        kprintf("pmap_extract: invalid TTE!\n");
    }

extract_out:
    /* Return. */
    SPLX(spl);
    return ppn;
}

/**
 * pmap_expand
 *
 * Expand the address space of the current physical map.
 */
void
pmap_expand(pmap_t map, vm_offset_t v)
{
    vm_offset_t* tte = (vm_offset_t*)pmap_tte(map, v);
    vm_page_t page = pmap_grab_page();

    /* xxx we need to free pages from an expanded pmap */

    assert(tte);

    if((*tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_SECTION)
        panic("cannot expand current map into L1 sections");

    /* Overwrite the old mapping in this region. */
    *tte = ((page->phys_page << PAGE_SHIFT) & L1_PTE_ADDR_MASK) | L1_TYPE_PTE | (1 << 4);

    /* Flush the TLBs */
    flush_mmu_tlb();

    return;
}

/**
 * pmap_enter_options
 *
 * Create a translation entry for a PA->VA mappings with additional options.
 * Called from vm_fault.
 */
kern_return_t
pmap_enter_options(
    pmap_t pmap,
    vm_map_offset_t va,
    ppnum_t pa,
    vm_prot_t prot,
    vm_prot_t fault_type,
    unsigned int flags,
    boolean_t wired,
    unsigned int options)
{
    spl_t       spl;
    pt_entry_t  pte;
    pv_entry_t  pv_e, pv_h;

    pv_e = (pv_entry_t)0;

    /* Verify the address. */
    assert(pa != vm_page_fictitious_addr);

    /* Only low addresses are supported for user pmaps. */
    if(va > _COMM_PAGE_BASE_ADDRESS && pmap != kernel_pmap)
        panic("pmap_enter_options: low address 0x%08X is invalid for pmap %p\n", va, pmap);
Retry:
    /* Set a high priority level. */
    SPLVM(spl);

    /* Expand the pmap to include the new PTE if necessary. */
    while((pte = pmap_pte(pmap, va)) == NULL) {
        SPLX(spl);
        pmap_expand(pmap, va);
        SPLVM(spl);
    }

    /* Make sure said PA is a valid one. */
    if((avail_start >> PAGE_SHIFT) > pa || (avail_end >> PAGE_SHIFT) < pa)
        return KERN_INVALID_ARGUMENT;

    /* If the old page already has a mapping, the caller might be changing protection flags. */
    uint32_t old_pte = (*(uint32_t*)pte);
    
    if((old_pte & L2_ADDR_MASK) == (pa << PAGE_SHIFT)) {
        /* !!! IMPLEMENT 'pmap_vm_prot_to_page_flags' !!! */
    
        /* XXX protection is not implemented right now, all pages are 'RWX'. */
        uint32_t template_pte = ((pa << PAGE_SHIFT) & L2_ADDR_MASK) | L2_SMALL_PAGE | L2_ACCESS_PRW;
        if(!wired)
            template_pte |= L2_ACCESS_USER;
        *(uint32_t*)pte = template_pte;

        /* The work here is done, the PTE will now have new permissions. */
        goto enter_options_done;
    }

    /* This is a new mapping, add it to the pv_head_table if pmap is initialized. */
    if(pmap_initialized) {
        ppnum_t pai;

        /* Get the offset... */
        pai = (pa);
        pv_h = pai_to_pvh(pai);

        kprintf("pai %d pa %d (%x) va %x pv_h %p pmap %p pv_h->pmap %p pv_h->pv_address_va %x\n",
                pai, pa, pa << PAGE_SHIFT, va, pv_h, pmap, pv_h->pv_pmap, pv_h->pv_address_va);

        /* Check to see if it exists, if it does, then make it null. */
        if((pv_h->pv_pmap == pmap) && (pv_h->pv_address_va == va)) {
            pv_entry_t cur;
            cur = pv_h->pv_next;
            if(cur != (pv_entry_t)0) {
                *pv_h = *cur;
                pv_e = cur;
            } else {
                pv_h->pv_pmap = PMAP_NULL;
            }
        }

        /* New mapping? */
        if(pv_h->pv_pmap == PMAP_NULL) {
            pv_h->pv_address_va = va;
            pv_h->pv_pmap = pmap;
            pv_h->pv_next = (pv_entry_t)0;
            if(wired)
                pv_h->pv_flags |= VM_MEM_WIRED;
        } else {
            /* Make sure there's no aliased mapping. */
            pv_entry_t e = pv_h;
            while(e != (pv_entry_t)0) {
                if(e->pv_pmap == pmap && e->pv_address_va == va)
                    panic("pmap_enter_options: duplicate pv_h %p", pv_h);
                e = e->pv_next;
            }

            /* Add a new pv_e. This is a leaf mapping. */
            if(pv_e == (pv_entry_t)0) {
                /* No new pv_e, grab a new one from the Zone. */
                pv_e = (pv_entry_t)zalloc(pve_zone);
                if(!pv_e) {
                    panic("pmap_enter_options: failed to grab a leaf node\n");
                }
                kprintf("pmap_enter_options: PV_GRAB_LEAF(), leaf node %p, head node %p\n", pv_e, pv_h);
                goto Retry;
            }

            pv_e->pv_address_va = va;
            pv_e->pv_pmap = pmap;
            pv_e->pv_next = pv_h->pv_next;
            if(wired)
                pv_e->pv_flags |= VM_MEM_WIRED;
            pv_h->pv_next = pv_e;
            pv_e = (pv_entry_t)0;
        }
    }

    /* Enter and count the mapping. */
    pmap->pm_stats.resident_count++;
    if(wired)
        pmap->pm_stats.wired_count++;

    /* !!! IMPLEMENT 'pmap_vm_prot_to_page_flags' !!! */
    
    /* XXX protection is not implemented right now, all pages are 'RWX'. */
    uint32_t template_pte = ((pa << PAGE_SHIFT) & L2_ADDR_MASK) | L2_SMALL_PAGE | L2_ACCESS_PRW;
    if(!wired)
        template_pte |= L2_ACCESS_USER;

    /* XXX add cacheability flags */
    if(flags & VM_MEM_NOT_CACHEABLE) {
        template_pte |= mmu_texcb_small(MMU_DMA);
    } else if(flags & VM_MEM_COHERENT) {
        template_pte |= mmu_texcb_small(MMU_CODE);
    } else {
        template_pte |= mmu_texcb_small(MMU_DATA);
    }
    *(uint32_t*)pte = template_pte;

enter_options_done:
    /* Done, now invalidate the TLB for a single page. */
    flush_mmu_single(va);

    /* Bye. */
    SPLX(spl);
    return KERN_SUCCESS;
}

/**
 * pmap_init
 *
 * Stage 2 initialization of the pmap subsystem.
 */
void
pmap_init(void)
{
    vm_offset_t pv_root;
    vm_size_t s;
    int i;

    kprintf("pmap_init: %d physical pages in memory, kernel pmap at %p\n", (mem_size / PAGE_SIZE), kernel_pmap);

    /* Allocate the core PV structure. */
    s = (mem_size / PAGE_SIZE) * sizeof(pv_entry);
    if(kernel_memory_allocate(kernel_map, &pv_root, s, 0, KMA_KOBJECT | KMA_PERMANENT) != KERN_SUCCESS)
        panic("pmap_init(): failed to allocate pv table!");

    /* Okay. Zero out the PV head table. */
    pv_head_table = (pv_entry_t)pv_root;
    kprintf("pmap_init: pv_head_table at %p\n", pv_head_table);
    bzero((void*)pv_head_table, s);

    /* Initialize the Zones. */
    pmap_zone = zinit((sizeof(struct pmap)), 400 * (sizeof(struct pmap)), 4096, "pmap_pmap");

    /* Expandable zone. */
    pve_zone = zinit((sizeof(struct __pv_entry__)), 10000 * (sizeof(struct __pv_entry__)), 4096, "pmap_pve");

    /* Initialize the free list lock. */
    simple_lock_init(&pv_free_list_lock, 0);

    /* Set up the core VM object. */
    pmap_object = &pmap_object_store;
    _vm_object_allocate(mem_size, &pmap_object_store);

    /* Done initializing. */
    pmap_initialized = TRUE;

    return;
}

/**
 * pmap_remove_range
 *
 * Remove a range of hardware page-table entries.
 */
void
pmap_remove_range(pmap_t pmap, vm_map_offset_t start_vaddr, pt_entry_t* spte, pt_entry_t *epte, boolean_t is_sect)
{
    pt_entry_t*     cpte;
    vm_map_offset_t vaddr;
    vm_size_t       our_page_size = (is_sect) ? (1 * 1024 * 1024) : PAGE_SIZE;
    int             num_removed = 0, num_unwired = 0;

    for(cpte = spte, vaddr = start_vaddr; cpte < epte; cpte++, vaddr += our_page_size) {
        /* Start nuking the range. */
        pt_entry_t *p = cpte;
        if(!*cpte & L2_ADDR_MASK)
            continue;

        num_removed++;

        /* If the pmap subsystem isn't initialized, just go on. (Remove mappings?) */
        if(!pmap_initialized) {
            continue;
        }

        /* Get the index for the PV table. */
        ppnum_t pai = pa_index(*cpte & L2_ADDR_MASK);

        kprintf("pmap_remove_range: cpte %x, *cpte: %x, epte %x\n", cpte, *cpte, epte);
        kprintf("pmap_remove_range: pmap %p pai %d %x vaddr %x\n", pmap, pai, pai << PAGE_SHIFT, start_vaddr);

        /* Remove the entry from the PV table. */
        {
            pv_entry_t  pv_h, prev, cur;
            pv_h = pai_to_pvh(pai);

            /*
             * It's possible that there were pages that were initialized before 'pmap' went up. 
             * This hack needs to DIE.
             */

            if(pv_h->pv_pmap == PMAP_NULL) {
                kprintf("pmap_remove_range: null pv_h->pmap (pmap %p, pv_h %p, pai %d)\n", pmap, pv_h, pai);
                goto out;
            }

            /* XXX ARM */
            if(pv_h->pv_address_va == vaddr && pv_h->pv_pmap == pmap) {
                cur = pv_h->pv_next;
                if(cur != (pv_entry_t)0) {
                    *pv_h = *cur;
                    zfree(pve_zone, cur);
                } else {
                    pv_h->pv_pmap = PMAP_NULL;
                }
            } else {
                cur = pv_h;
                while(cur->pv_address_va != vaddr || cur->pv_pmap != pmap) {
                    prev = cur;
                    if((cur = prev->pv_next) == (pv_entry_t)0) {
                        panic("pmap_remove_ranges: mapping not in pv_list!\n");
                    }
                }
                prev->pv_next = cur->pv_next;
                zfree(pve_zone, cur);
            }
        }
    }
out:
    /* Invalidate all TLBs. */
    flush_mmu_tlb();


    /* Make sure the amount removed isn't... weird. */
    assert(pmap->pm_stats.resident_count >= num_removed);
    OSAddAtomic(-num_removed, &pmap->pm_stats.resident_count);
    assert(pmap->pm_stats.wired_count >= num_unwired);
    OSAddAtomic(-num_unwired, &pmap->pm_stats.wired_count);

    return;
}

/**
 * pmap_remove
 *
 * Remove the given range of addresses from the specified map.
 */
void
pmap_remove(
    pmap_t      map,
    vm_offset_t s,
    vm_offset_t e)
{
    spl_t spl;
    vm_offset_t tte;
    vm_offset_t *spte, *epte, l = s;

    /* Verify the pages are page aligned. */
    assert(!(s & PAGE_MASK));
    assert(!(e & PAGE_MASK));

    /* XXX if the address to remove is greater than 1MB this is broken */
#if 0
    printf("pmap_remove(%p,%x,%x)\n", map, s, e);
#endif

    /* High Priority. */
    SPLVM(spl);

    while(s < e) {
        /* Grab the TTE. */
        tte = pmap_tte(map, l);

        /* 1MB section size. eeeeee */
        l = (s + (1 * 1024 * 1024));

        /* Make sure the TTE isn't a fault one. */
        if(tte && ((*(vm_offset_t*)tte & ARM_PAGE_MASK_VALUE) != 0)) {
            boolean_t   is_sect = TRUE;

            /* 1MB Section */
            spte = (vm_offset_t*)tte;
            epte = (vm_offset_t*)tte + 1;

            /* Special treatment for a page table. */ 
            if((*(vm_offset_t*)tte & ARM_PAGE_MASK_VALUE) == ARM_PAGE_PAGE_TABLE) {
                spte = (vm_offset_t*)pmap_pte(map, s);
                epte = (vm_offset_t*)pmap_pte(map, e);
                is_sect = FALSE;
            }

            /* Remove the range */
            pmap_remove_range(map, s, spte, epte, is_sect);
        } 
        s = l;
    }

    /* Flush TLBs */
    flush_mmu_tlb();

    /* Return. */
    SPLX(spl);
    return;
}

/**
 * pmap_create
 *
 * Create a pmap.
 */
pmap_t
pmap_create(ledger_t ledger, vm_map_size_t size, __unused boolean_t is_64bit)
{
    pmap_t      our_pmap;
    vm_page_t   new_l1;
    
    /* Some necessary requisites. */
    if(!pmap_initialized || size || !kernel_task)
        return PMAP_NULL;

    /* Zalloc a new one. */
    our_pmap = (pmap_t)zalloc(pmap_zone);
    our_pmap->pm_refcnt = 1;
    pmap_common_init(our_pmap);

    /* Grab a new page and set the new L1 region. */
    new_l1 = pmap_grab_page();
    our_pmap->pm_l1_phys = new_l1->phys_page << PAGE_SHIFT;
    our_pmap->pm_l1_virt = phys_to_virt(new_l1->phys_page << PAGE_SHIFT);
    bzero(phys_to_virt(new_l1->phys_page << PAGE_SHIFT), PAGE_SIZE);

    /* Done. */
    return our_pmap;
}

/**
 * pmap_page_protect
 *
 * Lower the protections on a set of mappings.
 */
void
pmap_page_protect(ppnum_t pn, vm_prot_t prot)
{
    boolean_t remove;
    spl_t spl;
    pv_entry_t pv_h, prev, pv_e;

    /* Verify it's not a fictitious page. */
    assert(pn != vm_page_fictitious_addr);

    /* Verify said page is managed by us. */
    assert(pmap_initialized);
    if(!pmap_valid_page(pn)) {
        return;
    }

    /* Determine the new protection. */
    switch (prot) {
        case VM_PROT_READ:
        case VM_PROT_READ|VM_PROT_EXECUTE:
            remove = FALSE;
            break;
        case VM_PROT_ALL:
            return; /* nothing to do */
        default:
            remove = TRUE;
            break;
    }

    /* Set a high priority level. */
    SPLVM(spl);

    /* Walk down the PV listings and remove the entries. */
    pv_h = pai_to_pvh(pn);

    if(pv_h->pv_pmap != PMAP_NULL) {
        prev = pv_e = pv_h;
        do {
            vm_map_offset_t vaddr;
            pt_entry_t pte;
            pmap_t pmap;

            pmap = pv_e->pv_pmap;
            vaddr = pv_e->pv_address_va;
            pte = pmap_pte(pmap, vaddr);
            if(!pte) {
                kprintf("pmap_page_protect pmap 0x%x pn 0x%x vaddr 0x%llx\n", pmap, pn, vaddr);
                panic("pmap_page_protect");
            }

            /* Remove the mapping if a new protection is NONE or if write-protecting a kernel mapping. */
            if(remove || pmap == kernel_pmap) {
                /* Remove the mapping. */
                *(pt_entry_t*)pte = 0;
                flush_mmu_single(vaddr);
                pv_h->pv_flags &= (VM_MEM_MODIFIED | VM_MEM_REFERENCED);

                /* Decrement the resident count. */
                OSAddAtomic(-1, &pmap->pm_stats.resident_count);

                /* Remove the PV entry. */
                if(pv_e == pv_h) {
                    /* Fix up head later. */
                    pv_h->pv_pmap = PMAP_NULL;
                } else {
                    /* Free the leaf node. */
                    prev->pv_next = pv_e->pv_next;
                    zfree(pve_zone, pv_e);
                }
            } else {
                /* Write protect. */
                Debugger("pmap_page_protect: Write protect mapping");
#if 0
                *(pt_entry_t) |= (1 << 11);     /* XXX FIX */
#endif
                flush_mmu_single(vaddr);
                prev = pv_e;
            }
        } while ((pv_e = prev->pv_next) != (pv_entry_t)0);

        /* If the pv_head mapping was removed, fix it up. */
        if(pv_h->pv_pmap == PMAP_NULL) {
            pv_e = pv_h->pv_next;
            if(pv_e != (pv_entry_t)0) {
                *pv_e = *pv_h;
                zfree(pve_zone, pv_e);
            }
        }
    }

    /* Return. */
    SPLX(spl);
}

/**
 * pmap_deallocate_l1
 *
 * Deallocate the allocated L1 translation table.
 */
void
pmap_deallocate_l1(pmap_t pmap)
{
    uint32_t    ttb_base = pmap->pm_l1_phys;
    vm_page_t   m;

    /* Lock the VM object. */
    vm_object_lock(pmap_object);

    /* Look up the page. */
    m = vm_page_lookup(pmap_object, (vm_object_offset_t)((ttb_base >> PAGE_SHIFT) - (gPhysBase >> PAGE_SHIFT)));
    assert(m);

    /* Got it, now free it. */
    VM_PAGE_FREE(m);

    /* Done. */
    vm_object_unlock(pmap_object);

    /* Invalidation of the entire pmap should be done. */
    return;
}

/**
 * pmap_destroy
 *
 * Destroy the current physical map.
 */
void
pmap_destroy(pmap_t pmap)
{
    spl_t spl;
    int refcnt;

    /* Some necessary prerequisites. */
    assert(pmap_initialized);

    /* NEVER EVER EVER DESTROY THE KERNEL PMAP */
    if(pmap == kernel_pmap) 
        panic("pmap_destroy: attempting to destroy kernel_pmap");

    SPLVM(spl);

    /* Okay, decrease the reference count. */
    refcnt = --pmap->pm_refcnt;
    if(refcnt == 0) {
        /* We might be using this pmap, invalidate all TLBs. */
        flush_mmu_tlb();
    }

    /* Unlock the pmap system. */
    SPLX(spl);

    /* If the pmap still has a reference count, we don't kill it. */
    if(refcnt != 0) {
        return;
    }

    /* xxx we need to free pages from an expanded pmap */

    /* Free the associated objects with the pmap first. */
    pmap_deallocate_l1(pmap);
    ledger_dereference(pmap->ledger);

    /* Free the actual pmap. */
    zfree(pmap_zone, pmap);

    /* Done. */
    return;
}

/**
 * pmap_protect
 *
 * Lower the specified protections on a certain map from sva to eva using prot prot.
 */
void
pmap_protect(pmap_t map, vm_map_offset_t sva, vm_map_offset_t eva, vm_prot_t prot)
{
    /* xxx finish */
    return;
}
