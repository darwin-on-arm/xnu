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

//#ifndef DEBUG_PMAP
#define kprintf(args...)
//#endif

/*
 * Kernel's physical memory map.
 */

static pmap_paddr_t avail_remaining;
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

extern uint64_t first_avail, avail_end;
extern uint64_t ram_begin;

vm_offset_t virt_begin, virt_end;

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
    pmap->pm_refcnt = 1;
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
    avail_remaining--;
    
    return TRUE;
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
 * pmap_verify_free
 *
 * Look at the page and verify that it is free.
 */
boolean_t pmap_verify_free(ppnum_t pa) {
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
 * pmap_free_pages
 *
 * Free pages. Is bad.
 */
unsigned int pmap_free_pages(void)
{
    return avail_remaining;
}

/**
 * pmap_valid_page
 */
boolean_t
pmap_valid_page(ppnum_t x)
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
    refcount = --(pmap->pm_refcnt);
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
    return (pmap)->pm_stats.resident_count;
}

/**
 * pmap_zero_page
 *
 * Zero a physical page.
 */
void pmap_zero_page(ppnum_t p)
{
    assert(p != vm_page_fictitious_addr);
    assert(pmap_valid_page(p));
    bzero(phys_to_virt(p), PAGE_SIZE);
}

/**
 * ohai.
 */
void pmap_clear_refmod(ppnum_t pn, unsigned int mask)
{
    return;
}

uint64_t pmap_nesting_size_min = 0x8000000;
uint64_t pmap_nesting_size_max = 0x8000000;

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
	ppnum_t src,
	ppnum_t dst)
{
	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);

	memcpy((void *)phystokv(dst), (void *)phystokv(src), PAGE_SIZE);
}

int pmap_list_resident_pages(pmap_t pmap, vm_offset_t *listp, int space)
{
    return 0;
}
