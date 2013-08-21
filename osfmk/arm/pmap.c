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
 * This is painful.
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
 * Kernel's physical memory map.
 */

vm_offset_t free_l1_tables;

typedef enum {
    ATTR_NONE   = 0x0,
    ATTR_READ   = 0x1,
    ATTR_WRITE  = 0x2,
    ATTR_WIRED  = 0x4,
} attr_bits_t;

#define PV_ENTRY_NULL   ((pv_entry_t) 0)

#define pa_index(pa)    (atop(pa - gPhysBase))
#define pai_to_pvh(pai)        (&pv_head_table[pai])

unsigned int	inuse_ptepages_count = 0;
unsigned int 	alloc_ptepages_count = 0;

struct pmap    kernel_pmap_store;
pmap_t kernel_pmap = &kernel_pmap_store;

struct zone    *pmap_zone;  /* zone of pmap structures */
int         free_pmap_count;
pmap_t      free_pmap_list;

static struct vm_object pmap_object_store;
vm_object_t pmap_object;

extern uint32_t first_avail, avail_end;

extern uint32_t identityBaseVA, identityCachePA;
extern uint32_t managedBaseVA, managedCachePA;
extern uint32_t exceptionVectVA, exceptionCachePA;
extern uint32_t sectionOffset;

uint32_t virt_begin, virt_end, ram_begin;

typedef struct pv_entry {
    struct pv_entry *next;              /* next pv_entry */
    pmap_t          pmap;               /* pmap where mapping lies */
    vm_offset_t     va;                 /* virtual address for mapping */
    uint32_t        attr;               /* protection bits for a page */
} pv_entry, *pv_entry_t;

pv_entry_t    pv_head_table;        /* array of entries, one per page */

boolean_t pmap_initialized = FALSE;

/*
 * We raise the interrupt level to splvm, to block interprocessor
 * interrupts during pmap operations.  We must take the CPU out of
 * the cpus_active set while interrupts are blocked.
 */
#define SPLVM(spl)    { \
    spl = splhigh(); \
}

#define SPLX(spl)    { \
    splx(spl); \
}

/*
 * Lock on pmap system
 */
lock_t pmap_system_lock;

#define PMAP_READ_LOCK(pmap, spl) {    \
    SPLVM(spl);            \
    lock_read(&pmap_system_lock);    \
    simple_lock(&(pmap)->lock);    \
}

#define PMAP_WRITE_LOCK(spl) {        \
    SPLVM(spl);            \
    lock_write(&pmap_system_lock);    \
}

#define PMAP_READ_UNLOCK(pmap, spl) {        \
    simple_unlock(&(pmap)->lock);        \
    lock_read_done(&pmap_system_lock);    \
    SPLX(spl);                \
}

#define PMAP_WRITE_UNLOCK(spl) {        \
    lock_write_done(&pmap_system_lock);    \
    SPLX(spl);                \
}

#define PMAP_WRITE_TO_READ_LOCK(pmap) {        \
    simple_lock(&(pmap)->lock);        \
    lock_write_to_read(&pmap_system_lock);    \
}


/**
 * pmap_common_init
 *
 * Initialize common elements of pmaps.
 */
void pmap_common_init(pmap_t pmap)
{
    usimple_lock_init(&pmap->lock, 0);
    ledger_reference(pmap->ledger);
    pmap->ref_count = 1;
    pmap->nx_enabled = 0;
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
    kprintf("pmap_virtual_space: region 0x%08x-0x%08x\n", virt_begin, virt_end);
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
    vm_offset_t    addr;
    vm_map_offset_t vaddr;
    vm_size_t s;
    ppnum_t page_number;
    
    kdb_printf("pmap_bootstrap: Kicking off pmap!\n");
    
    assert(__first_avail != NULL);
    
    virt_begin = (*__first_avail);
    virt_end = 0xFFFEFFFF;  // Don't go to ARM VT. :(

    vm_last_addr = virt_end;
    
    ram_begin = first_avail;
    kprintf("pmap_bootstrap: physical region 0x%08x-0x%08x\n", ram_begin, avail_end);

    /*
     * Initialize kernel pmap.
     */
    pmap_common_init(kernel_pmap);
    kernel_pmap->stats.resident_count = 0;
    kernel_pmap->stats.wired_count = 0;
    
    /*
     * More needs to be done.
     */
    s = sizeof(struct pv_entry) * (mem_size / PAGE_SIZE);
    addr = phys_to_virt(first_avail);
    first_avail += s;
    pv_head_table = (pv_entry_t)addr;
    bzero(pv_head_table, s);
    
    /*
     * now fill out the addresses
     */
    vaddr = (vm_map_offset_t)VM_MIN_KERNEL_ADDRESS;
    for(page_number = 0; page_number <= pa_index(ram_begin); page_number++) {
        pv_entry_t  entry;
        
        entry = pai_to_pvh(page_number);
        entry->va = vaddr;
        vaddr += PAGE_SIZE;
        entry->pmap = kernel_pmap;
        entry->next = PV_ENTRY_NULL;
        entry->attr = ATTR_WIRED;   /* All initial pages are wired */
    }

    lock_init(&pmap_system_lock, FALSE, 0, 0);

    return;
}

/**
 * pmap_static_init
 *
 * Initialize the basic kernel pmap.
 */
void pmap_static_init(void)
{
    kdb_printf("pmap_static_init: Bootstrapping pmap\n");

    return;
}

/**
 * pmap_map_bd
 *
 * Enters a physical mapping.
 */
boolean_t pmap_map_bd(vm_offset_t virt,
                      vm_map_offset_t start,
                      vm_map_offset_t end,
                      vm_prot_t prot,
                      unsigned int flags)
{
    uint32_t* tte_ptr = (uint32_t*)addr_to_tte(kernel_pmap->ttb, virt);
    uint32_t tte = *tte_ptr;
    uint32_t pte;

    if (!tte_is_page_table(tte)) {
        /* Not cached */
        return FALSE;
    }

    pte = L1_PTE_ADDR(tte); /* l2 base */
    pte += pte_offset(virt);
    
    /*
     * This should always work when mapping into contigious
     * cache ranges. If the range is not contigious, shit will
     * happen.
     */
    
    l2_map_linear_range((pte),
                        start,
                        end);
    
    /* Flush TLB after creating entries */
    flush_mmu_tlb();
    
    return TRUE;
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
    
    *addrp = first_avail;
    first_avail += PAGE_SIZE;
    
    return TRUE;
}


/**
 * pmap_get_phys (old)
 *
 * Get a physical address for the virtual one.
 */
vm_offset_t
pmap_get_phys(pmap_t pmap, void* virt)
{
    uint32_t* tte_ptr = (uint32_t*)addr_to_tte(pmap->ttb, (uint32_t)virt);
    uint32_t tte = *tte_ptr;
    uint32_t pte, *pte_ptr;
    uint32_t pa;
    
    if (!tte_is_page_table(tte)) {
        /* Not cached */
        return 0;
    }

    pte = L1_PTE_ADDR(tte); /* l2 base */
    pte += pte_offset((uint32_t)virt);
    if(!pte)
        return 0;
    pte_ptr = phys_to_virt((uint32_t*)pte);
    
    pa = *pte_ptr & L2_ADDR_MASK;   // Knock off the last two bits.
    
    kprintf("pmap_get_phys: va 0x%08x -> pa 0x%08x\n", virt, pa);
    
    return pa;
}

vm_offset_t
pmap_get_phys_tte(uint32_t tte_va, void* virt)
{
    uint32_t* tte_ptr = (uint32_t*)addr_to_tte(tte_va, (uint32_t)virt);
    uint32_t tte = *tte_ptr;
    uint32_t pte, *pte_ptr;
    uint32_t pa;
    
    if (!tte_is_page_table(tte)) {
        /* Not cached */
        return 0;
    }

    pte = L1_PTE_ADDR(tte); /* l2 base */
    pte += pte_offset((uint32_t)virt);
    if(!pte)
        return 0;
    pte_ptr = phys_to_virt((uint32_t*)pte);
    
    pa = *pte_ptr & L2_ADDR_MASK;   // Knock off the last two bits.
    
    kprintf("pmap_get_phys_tte: va 0x%08x -> pa 0x%08x\n", virt, pa);
    
    return pa;
}

vm_offset_t pmap_extract(pmap_t pmap, vm_offset_t virt) {
    uint32_t* tte_ptr = (uint32_t*)addr_to_tte(pmap->ttb, (uint32_t)virt);
    uint32_t tte = *tte_ptr;
    uint32_t pte, *pte_ptr;
    uint32_t pa;
    
    pte = L1_PTE_ADDR(tte); /* l2 base */
    pte += pte_offset((uint32_t)virt);
    if(!pte)
        return 0;
    pte_ptr = phys_to_virt((uint32_t*)pte);
    
    pa = (*pte_ptr & L2_ADDR_MASK);   //  PTE_SHIFT
    
    return pa;
}

/**
 * pmap_find_phys (old)
 *
 * Find the physical frame number of a virtual address.
 */
ppnum_t pmap_find_phys(pmap_t pmap, addr64_t virt)
{
    return 0;
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
        (void)hw_atomic_add(&pmap->ref_count, 1); /* Bump the count */
}

/**
 * pmap_verify_free
 *
 * Look at the page and verify that it is free.
 */
boolean_t pmap_verify_free(vm_offset_t pa) {
    pv_entry_t  pv_h;
    int         pai;
    boolean_t   result;
    
    assert(pa != vm_page_fictitious_addr);
    
    if(!pmap_initialized)
        return TRUE;
    
    if(!pmap_valid_page(pa))
        return TRUE;

    pai = pa_index(pa);
    pv_h = pai_to_pvh(pai);

    result = (pv_h->pmap == PMAP_NULL);
    
    return TRUE;    /* result, since pmap_remove is not done yet */
}

/**
 * pmap_tte
 */
vm_offset_t pmap_tte(pmap_t pmap, addr64_t virt)
{
    uint32_t size_max = L1_SIZE;
    uint32_t size_begin;
    
    assert(pmap != NULL);
    
    size_begin = pmap->ttb;

    if((size_begin + size_max) < addr_to_tte(pmap->ttb, virt))
        panic("size extends past tte length");
    
    return addr_to_tte(pmap->ttb, virt);
}

/**
 * pmap_find_phys
 *
 * Find the physical frame number of a virtual address.
 */
pt_entry_t pmap_pte(pmap_t pmap, addr64_t virt)
{
    uint32_t* tte_ptr;
    uint32_t tte;
    uint32_t pte, *pte_ptr;
    
    if(!pmap)
        return NULL;
    
    tte_ptr = (uint32_t*)addr_to_tte(pmap->ttb, (uint32_t)virt);
    tte = *tte_ptr;
    
    pte = L1_PTE_ADDR(tte); /* l2 base */
    if(pte == 0)
        return NULL;
    
    pte += pte_offset((uint32_t)virt);
    return (pt_entry_t)pte;
}

vm_page_t pmap_alloc_l2(pmap_t map)
{
    vm_page_t m;
    uint32_t ctr;

    /* Verify pmap is up */
    assert(map != NULL);
    assert(pmap_initialized);
    
    /* Grab pages */
    while((m = vm_page_grab()) == VM_PAGE_NULL)
        VM_PAGE_WAIT();
    
    /* Lock the global object */
    vm_object_lock(pmap_object);
    ctr = (m->phys_page >> PAGE_SHIFT) - (gPhysBase >> PAGE_SHIFT);
    vm_page_insert(m, pmap_object, ctr);

    /* Now wire it and inser it */
    vm_page_lockspin_queues();
    vm_page_wire(m);
    vm_page_unlock_queues();

    /* Done */
    vm_object_unlock(pmap_object);

    /* Increment counts */
    OSAddAtomic(1, &inuse_ptepages_count);
    OSAddAtomic64(1, &alloc_ptepages_count);
    
    /* Zero page */
    bzero(phys_to_virt(m->phys_page), PAGE_SIZE);

    kprintf("pmap_alloc_l2: L2 page at 0x%08x/0x%08x\n",
            m->phys_page, phys_to_virt(m->phys_page));
    return m;
}

void pmap_expand(pmap_t map, vm_offset_t v)
{
    vm_offset_t pa;
    vm_page_t l2_page;
    uint32_t* tte_ptr;

    /* Should actually verify if TTB is good, meh */
    l2_page = pmap_alloc_l2(map);

    tte_ptr = pmap_tte(map, v);
    
    assert(l2_page);
    *tte_ptr = ((l2_page->phys_page) & L1_PTE_ADDR_MASK) | L1_TYPE_PTE | (1 << 4);
    
    return;
}

#define valid_page(x) (pmap_initialized && pmap_valid_page(x))
/**
 * pmap_enter
 *
 * Create a translation for the virtual address (virt) to the physical
 * address (phys) in the pmap.
 */
void
pmap_enter(pmap_t pmap, vm_map_offset_t va, ppnum_t pa, vm_prot_t prot,
           vm_prot_t fault_type, unsigned int flags, boolean_t wired)
{
    pmap_enter_options(pmap, va, pa, prot, fault_type, flags, wired, 0);
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
    pt_entry_t  pte;
    uint32_t    old_addr;
    int         pai;
    uint32_t    template_pte;
    pv_entry_t  pv_h;
    pv_entry_t  pv_e;
    
    /* Verify address */
    assert(pa != vm_page_fictitious_addr);
    
    /* leave if software only pmap */
    if (pmap == PMAP_NULL)
        return KERN_INVALID_ARGUMENT;
    if (pa == vm_page_guard_addr)
        return KERN_INVALID_ARGUMENT;

    /*
     * Lock the pmap.
     */
    PMAP_LOCK(pmap);
    
    /*
     * Expand the pmap to include this PTE if necessary.
     */
    while((pte = pmap_pte(pmap, va)) == NULL) {
        PMAP_UNLOCK(pmap);
        pmap_expand(pmap, va);
        kprintf("pmap_expand: expanded pmap, va 0x%08x -> 0x%08x\n", va, pa);
        PMAP_LOCK(pmap);
    }
#if 0
    kprintf("pmap_enter: 0x%08x -> 0x%08x (pmap: 0x%08x, pte: 0x%08x, ttb: 0x%08x, ttb_phys: 0x%08x)\n",
            va, pa, pmap, pte, pmap->ttb, pmap->ttb_phys);
#endif
    /*
     * See if it has an old PA.
     */
    old_addr = (*(uint32_t*)phys_to_virt(pte)) & L2_ADDR_MASK;
    if(old_addr == pa) {
        /*
         * Could be changing protections, implement this.
         */
        template_pte = (pa & L2_ADDR_MASK) | L2_SMALL_PAGE | L2_ACCESS_PRW;
        if(!wired)
            template_pte |= L2_ACCESS_USER;
        *(uint32_t*)phys_to_virt(pte) = template_pte;
        
        goto done;
    }
    
    /*
     * New mapping. So make one and flush the tlb if necessary.
     */
    if(valid_page(pa)) {
        pai = pa_index(pa);
        pv_h = pai_to_pvh(pai);
        
        if (pv_h->pmap == PMAP_NULL) {
            /*
             * No mappings yet.
             */
            pv_h->va = va;
            pv_h->pmap = pmap;
            pv_h->next = PV_ENTRY_NULL;
            if(wired)
                pv_h->attr |= ATTR_WIRED;
            
        } else {
            /*
             * Handle this case.
             */
        }
    }

    /*
     * Step 3) Enter and count the mapping.
     */
    
    pmap->stats.resident_count++;
    
    /*
     * Enter it in the pmap.
     */
    template_pte = (pa & L2_ADDR_MASK) | L2_SMALL_PAGE | L2_ACCESS_PRW;
    if(!wired)
        template_pte |= L2_ACCESS_USER;
    
    /*
     * Add caching flags.
     */
    if(flags & VM_MEM_NOT_CACHEABLE) {
        template_pte |= mmu_texcb_small(MMU_DMA);
    } else if(flags & VM_MEM_COHERENT) {
        template_pte |= mmu_texcb_small(MMU_CODE);
    } else {
        template_pte |= mmu_texcb_small(MMU_DMA);
    }

    *(uint32_t*)phys_to_virt(pte) = template_pte;
    
    /*
     * Update counts
     */
    if(wired)
        pmap->stats.wired_count++;
    
done:
    PMAP_UNLOCK(pmap);
    
    /*
     * Flush TLB.
     */
    flush_mmu_single(pa);
    
    return KERN_SUCCESS;
}

/**
 * pmap_free_pages
 *
 * Free pages. Is bad.
 */
unsigned int pmap_free_pages(void)
{
    return (avail_end - first_avail) >> PAGE_SHIFT;
}

/**
 * pmap_valid_page
 */
boolean_t
pmap_valid_page(vm_offset_t x)
{
    return ((ram_begin <= x) && (x < avail_end));
}

/**
 * pmap_is_noencrypt/pmap_clear_noencrypt/whatever.
 *
 * Not my problem right now.
 */
boolean_t
pmap_is_noencrypt(ppnum_t pn)
{
    return (FALSE);
}

void
pmap_clear_noencrypt(ppnum_t pn)
{
    return;
}

void
pmap_set_noencrypt(ppnum_t pn)
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
 * io_map
 *
 * Maps an IO region and returns its virtual address.
 */
vm_offset_t
io_map(vm_offset_t phys_addr, vm_size_t size, unsigned int flags)
{
	vm_offset_t	start;

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

void
compute_pmap_gc_throttle(void *arg __unused)
{

}

/**
 * pmap_init
 *
 * Stage 2 initialization of the pmap subsystem.
 */

void
pmap_init(void)
{
    kprintf("pmap_init: I am hell. Love me.\n");
    
    free_pmap_list = NULL;                    /* Set that there are no free pmaps */
    free_pmap_count = 0;
    
    pmap_zone = zinit((sizeof(struct pmap)), 400 * (sizeof(struct pmap)), 4096, "pmap");

    pmap_object = &pmap_object_store;
    _vm_object_allocate(mem_size, &pmap_object_store);

    pmap_initialized = TRUE;
    
    return;
}


/**
 * pmap_page_protect
 *
 * Lower the permission for all mappings to a given page.
 */
void pmap_page_protect(ppnum_t pn, vm_prot_t prot) {
    pv_entry_t pv_h, prev;
    pv_entry_t pv_e;
    pt_entry_t pte;
    boolean_t remove;
    pmap_t pmap;
    spl_t spl;
    int pai;
    
    assert(pn != vm_page_fictitious_addr);
    
    if (!valid_page(pn)) {
        /*
         * Not a managed page.
         */
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
            return;    /* nothing to do */
        default:
            remove = TRUE;
            break;
    }
    
    /*
     * Lock the pmap system first, since we will be changing
     * several pmaps.
     */
    
    PMAP_WRITE_LOCK(spl);
    
    pai = pa_index(pn);
    pv_h = pai_to_pvh(pai);
    
    /*
     * Walk down PV list, changing or removing all mappings.
     * We do not have to lock the pv_list because we have
     * the entire pmap system locked.
     */
    if (pv_h->pmap != PMAP_NULL) {
        prev = pv_e = pv_h;
        do {
            register vm_offset_t va;
            pmap = pv_e->pmap;
            /*
             * Lock the pmap to block pmap_extract and similar routines.
             */
            simple_lock(&pmap->lock);
            {
                va = pv_e->va;
                pte = pmap_pte(pmap, va);
            }
            if (remove || pmap == kernel_pmap) {
                /*
                 * Remove the mapping.
                 */
                uint32_t* pte_ptr = (uint32_t*)phys_to_virt(pte);
                *pte_ptr = 0x0;
                assert(pmap->stats.resident_count >= 1);
                pmap->stats.resident_count--;
                /*
                 * Remove the pv_entry.
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
                    prev->next = pv_e->next;
                }
            } else {
                /*
                 * Write protect.
                 */
                {
                    uint32_t *pte_ptr = (uint32_t*)phys_to_virt(pte);
                    *pte_ptr &= ~(L2_ACCESS_PRW);
                    *pte_ptr |= (L2_ACCESS_PRO);
                    flush_mmu_single(pn);
                }
                /*
                 * Advance prev.
                 */
                prev = pv_e;
            }
            
            simple_unlock(&pmap->lock);
            
        } while ((pv_e = prev->next) != PV_ENTRY_NULL);
        
        /*
         * If pv_head mapping was removed, fix it up.
         */
        if (pv_h->pmap == PMAP_NULL) {
            pv_e = pv_h->next;
            if (pv_e != PV_ENTRY_NULL) {
                *pv_h = *pv_e;
            }
        }
    }
    
    PMAP_WRITE_UNLOCK(spl);
}

/*
 * Grrr.
 */

unsigned int pmap_disconnect(ppnum_t pa)
{
    pmap_page_protect(pa, 0);
    return 0;
}

void
pmap_pageable(__unused pmap_t pmap,
              __unused vm_map_offset_t start,
              __unused vm_map_offset_t end,
              __unused boolean_t pageable)
{
    return;
}

void mapping_free_prime(void)
{
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
    pmap_t  our_pmap;
    
    if(!pmap_initialized)
        return PMAP_NULL;
    
    if(size)
        return PMAP_NULL;
    
    if(!kernel_task)
        return PMAP_NULL;
    
    /*
     * Just zalloc a new one. Eventually get one out of the free list.
     */
    
    our_pmap = (pmap_t)zalloc(pmap_zone);
    kprintf("pmap_create: %p = new_pmap\n", our_pmap);
    
    our_pmap->ref_count = 1;
    pmap_common_init(our_pmap);
    
    /*
     * Awesome. Now allocate a L1/L2 for it. 
     */

    /*
     * alloc aligned object, should *really* use the pmap object.
     */
    vm_page_t m;
    uint32_t ctr;
    vm_offset_t address = NULL;
    
    /* Grab pages */
    while((m = vm_page_grab()) == VM_PAGE_NULL)
        VM_PAGE_WAIT();
    
    /* Lock the global object */
    vm_object_lock(pmap_object);
    ctr = (m->phys_page >> PAGE_SHIFT) - (gPhysBase >> PAGE_SHIFT);
    vm_page_insert(m, pmap_object, ctr);

    /* Now wire it and inser it */
    vm_page_lockspin_queues();
    vm_page_wire(m);
    vm_page_unlock_queues();
    vm_object_unlock(pmap_object);
    
    address = m->phys_page;
    
    our_pmap->ttb = phys_to_virt(address);
    our_pmap->ttb_phys = address;
    bzero(phys_to_virt(address), PAGE_SIZE);
    
    kprintf("pmap_create: new ttb 0x%08x, 0x%08x\n", our_pmap->ttb, our_pmap->ttb_phys);
    
    return our_pmap;
}

/*
 * wat.
 */
void mapping_adjust(void) {
    return;
}

void
pmap_remove(pmap_t map,
            vm_map_offset_t s,
            vm_map_offset_t e)
{
    return;
}

/**
 * pmap_change_wiring
 *
 * Change physical attributes of the specified page.
 * tbd: change bits in page map, put in pmap_page_attributes
 */
void
pmap_change_wiring(pmap_t map,
                   vm_map_offset_t vaddr,
                   boolean_t wired)
{
    pt_entry_t pte;
    
    PMAP_LOCK(map);
    
    if ((pte = pmap_pte(map, vaddr)) == 0)
        panic("pmap_change_wiring: pte missing");
    
    if (wired) {
        /*
         * wiring down mapping
         */
        OSAddAtomic(+1,  &map->stats.wired_count);
    } else if (!wired) {
        /*
         * unwiring mapping
         */
        assert(map->stats.wired_count >= 1);
        OSAddAtomic(-1,  &map->stats.wired_count);
    }
    
    PMAP_UNLOCK(map);
}

void pmap_switch(pmap_t tpmap)
{
    spl_t s;

	s = splhigh();
    if(current_cpu_datap()->user_pmap == tpmap) {
        goto out;
    } else {
        current_cpu_datap()->user_pmap = tpmap;
        set_mmu_ttb(tpmap->ttb_phys);
        flush_mmu_tlb();
    }
out:
	splx(s);
}

/**
 * pmap_destroy
 *
 * Destroy the current pmap and all mappings inside of it.
 */
void
pmap_destroy(pmap_t pmap)
{
    int refcount;

    assert(pmap != NULL);
    
    PMAP_LOCK(pmap);
    refcount = --(pmap->ref_count);
    PMAP_UNLOCK(pmap);
    
    if(refcount) {
        /* Still in use! */
        return;
    }
    
    ledger_dereference(pmap->ledger);
    zfree(pmap_zone, pmap);
    return;
}

/** 
 * pmap_resident_count
 *
 * Return the number of resident pages in a specified pmap.
 */
int pmap_resident_count(pmap_t pmap)
{
    assert(pmap);
    return (pmap)->stats.resident_count;
}

/**
 * pmap_zero_page
 *
 * Zero a physical page.
 */
void pmap_zero_page(vm_offset_t p)
{
    assert(p != vm_page_fictitious_addr);
    assert(pmap_valid_page(p));
    bzero(phys_to_virt(p), 4096);
}

/**
 * ohai.
 */
void pmap_clear_refmod(ppnum_t pn, unsigned int mask)
{
    return;
}

/*
 *	kern_return_t pmap_nest(grand, subord, va_start, size)
 *
 *	grand  = the pmap that we will nest subord into
 *	subord = the pmap that goes into the grand
 *	va_start  = start of range in pmap to be inserted
 *	nstart  = start of range in pmap nested pmap
 *	size   = Size of nest area
 *
 *	Inserts a pmap into another.  This is used to implement shared segments.
 */
uint64_t pmap_nesting_size_min = 0x8000000;
uint64_t pmap_nesting_size_max = 0x8000000;

kern_return_t pmap_nest(pmap_t grand, pmap_t subord, addr64_t va_start, addr64_t nstart, uint64_t size) {
    uint32_t section_counter;
    uint32_t ttb_grand;
    uint32_t ttb_subord;
    
    assert(grand && subord);
    
    kprintf("pmap_nest: grand %p[0x%llx] -> subord: %p[0x%llx], size: 0x%llx\n",
            grand, va_start, subord, nstart, size);
    
    /* Copy the pmap mappings for one to another. */
    assert(size != 0);

	if (va_start != nstart)
		panic("pmap_nest: va_start(0x%llx) != nstart(0x%llx)\n", va_start, nstart);

    PMAP_LOCK(subord);
    subord->pm_shared = TRUE;
    PMAP_UNLOCK(subord);
    
    kprintf("ttb of subordinate is 0x%08x, ttb grand: 0x%08x\n",
            grand->ttb, subord->ttb);
    
    panic("not yet\n");
}

void
pmap_protect(
	pmap_t		map,
	vm_map_offset_t	sva,
	vm_map_offset_t	eva,
	vm_prot_t	prot)
{
	return;
}

/**
 * pmap_copy_page
 *
 * Copy a specified page to another page.
 */
void
pmap_copy_page(
	vm_offset_t src,
	vm_offset_t dst)
{
	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);

	memcpy((void *)phystokv(dst), (void *)phystokv(src), PAGE_SIZE);
}

/**
 * commpage
 */
void pmap_create_sharedpage(void)
{
    vm_page_t m;
    uint32_t ctr;
    pmap_t map;
    
    /* could really put everything into one large thing */
    map = kernel_pmap;

    /* Verify pmap is up */
    assert(map != NULL);
    assert(pmap_initialized);
    
    /* Grab pages */
    while((m = vm_page_grab()) == VM_PAGE_NULL)
        VM_PAGE_WAIT();
    
    /* Lock the global object */
    vm_object_lock(pmap_object);
    ctr = (m->phys_page >> PAGE_SHIFT) - (gPhysBase >> PAGE_SHIFT);
    bzero(phys_to_virt(m->phys_page), PAGE_SIZE);
    vm_page_insert(m, pmap_object, ctr);

    /* Now wire it and inser it */
    vm_page_lockspin_queues();
    vm_page_wire(m);
    vm_page_unlock_queues();

    /* Done */
    vm_object_unlock(pmap_object);

    /* Map page */
    pmap_enter(map, (vm_map_offset_t)_COMM_PAGE_BASE_ADDRESS, (m->phys_page), 0, 0, FALSE, FALSE);
    return;
}

int pmap_list_resident_pages(pmap_t pmap, vm_offset_t *listp, int space)
{
    return 0;
}
