// stolen from ppc

#ifndef _ARM_PMAP_H_
#define _ARM_PMAP_H_

#include <vm/pmap.h>
#include <mach/kern_return.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <mach/machine/vm_param.h>
#include <kern/kern_types.h>
#include <kern/thread.h>
#include <kern/lock.h>

#ifdef MACH_KERNEL_PRIVATE
#include <pexpert/arm/boot.h>

#include <kern/queue.h>
#include <vm/vm_page.h>

#define mmu_texcb_small(x) ((((x) & 0x1c) << 4) | (((x) & 3) << 2))

#define MMU_TEXCB_ORDERED 0
#define MMU_TEXCB_SH_DEVICE 1
#define MMU_TEXCB_WT_NWA 2
#define MMU_TEXCB_WB_NWA 3
#define MMU_TEXCB_NCA 4
#define MMU_TEXCB_WB_WA 7
#define MMU_TEXCB_NSH_DEVICE 8
// cacheable memory -[outer]_[inner]
#define MMU_TEXCB_CA 0x10
#define MMU_TEXCB_CA_NCA 0
#define MMU_TEXCB_CA_WB_WA 1
#define MMU_TEXCB_CA_WT_NWA 2
#define MMU_TEXCB_CA_WB_NWA 3

#define MMU_CODE (MMU_TEXCB_CA | MMU_TEXCB_CA_WT_NWA | (MMU_TEXCB_CA_WT_NWA<<2))
#define MMU_DATA (MMU_TEXCB_CA | MMU_TEXCB_CA_WB_WA | (MMU_TEXCB_CA_WB_WA<<2))
#define MMU_DMA (MMU_TEXCB_CA | MMU_TEXCB_CA_WT_NWA | (MMU_TEXCB_CA_WT_NWA<<2))
#define MMU_DEVICE_SHARED MMU_TEXCB_SH_DEVICE
#define MMU_DEVICE_NSHARED MMU_TEXCB_NSH_DEVICE

/*
 * pmap locking
 */

#define PMAP_LOCK(pmap) {		     \
    simple_lock(&(pmap)->lock);	    \
}

#define PMAP_UNLOCK(pmap) {			      \
    simple_unlock(&(pmap)->lock);		  \
}

#define l2_size(size) ((uint32_t)((size >> 20) << 10))

void arm_vm_init(uint32_t mem_limit, boot_args *args);

typedef uint32_t	pd_entry_t;	/* L1 table entry */
typedef uint32_t	pt_entry_t;	/* L2 table entry */

#pragma pack(4)							/* Make sure the structure stays as we defined it */
struct pmap {
	decl_simple_lock_data(,lock)	/* lock on map */
	uint32_t        ttb;
    uint32_t        ttb_phys;
    vm_offset_t     l2_cache;
    int             ref_count;
    ledger_t        ledger;
    boolean_t       pm_shared;
    task_map_t      pm_task_map;
    int             nx_enabled;
    struct pmap_statistics  stats;
};
#pragma pack()

#define	PMAP_SWITCH_USER(th, map, my_cpu) th->map = map;

#define pmap_kernel_va(VA)	\
	(((VA) >= VM_MIN_KERNEL_ADDRESS) && ((VA) <= vm_last_addr))

#define SUPERPAGE_NBASEPAGES 0

#define PMAP_DEFAULT_CACHE	0
#define PMAP_INHIBIT_CACHE	1
#define PMAP_GUARDED_CACHE	2
#define PMAP_ACTIVATE_CACHE	4
#define PMAP_NO_GUARD_CACHE	8

/* corresponds to cached, coherent, not writethru, not guarded */
#define VM_WIMG_DEFAULT		(VM_MEM_COHERENT)
#define	VM_WIMG_COPYBACK	(VM_MEM_COHERENT)
#define VM_WIMG_IO		(VM_MEM_COHERENT | 	\
				VM_MEM_NOT_CACHEABLE | VM_MEM_GUARDED)
#define VM_WIMG_WTHRU		(VM_MEM_WRITE_THROUGH | VM_MEM_COHERENT | VM_MEM_GUARDED)
/* write combining mode, aka store gather */
#define VM_WIMG_WCOMB		(VM_MEM_NOT_CACHEABLE | VM_MEM_COHERENT) 
#define	VM_WIMG_INNERWBACK	VM_MEM_COHERENT


/* 
 * prototypes.
 */

/*
 * "these two macros are your friends"
 */

#define align_up(p, s) align_down((uintptr_t)(p)+s-1, s)
#define align_down(p, s) ((uintptr_t)(p)&~(s-1))

#define virt_to_phys(p) ((unsigned int)((((unsigned long)(p)) - gVirtBase) + gPhysBase))
#define phys_to_virt(p) ((unsigned int)((((unsigned long)(p)) - gPhysBase) + gVirtBase))

#define L1_SIZE 0x4000 /* 16kb: covers 2048*2 1MB sections */
#define L2_SIZE 0x400 /* 1kb: covers 256 4kb sections */

#define tte_offset(addr) (((addr >> 0x14) & 0xfff) << 2)
#define pte_offset(addr) (((addr & ~(L1_SECT_ADDR_MASK)) >> PAGE_SHIFT) << 2)
#define addr_to_tte(base, addr) (base + tte_offset(addr))

#define L1_PTE_ADDR_MASK 0xfffffc00 /* Bits [31:10] */
#define L1_SECT_ADDR_MASK 0xfff00000 /* Bits [31:20] */

#define L1_PTE_ADDR(tte) (tte & L1_PTE_ADDR_MASK)

#define L1_TYPE_MASK 3 /* two least bits */

#define L1_TYPE_FAULT 0
#define L1_TYPE_PTE 1
#define L1_TYPE_SECT 2
#define L1_TYPE_RESERVED 3

#define L2_ADDR_MASK 0xfffff000
#define L2_ADDR(pte) (pte & L2_ADDR_MASK)

#define L2_ACCESS_NONE 0x0
#define L2_ACCESS_PRW 0x10
#define L2_ACCESS_PRO 0x210

#define L2_ACCESS_USER      (1 << 5)

#define tte_is_page_table(tte) ((tte & L1_TYPE_MASK) == L1_TYPE_PTE)

#define L2_SMALL_PAGE 0x2
#define L2_NX_BIT 0x1 /* XN bit */
#define L2_C_BIT 0x8 /* C bit */
#define L2_B_BIT 0x4 /* B bit */
#define L2_S_BIT 0x400 /* S bit */
#define L2_NG_BIT 0x800 /* nG bit */

extern addr64_t	   	kvtophys(vm_offset_t va);				/* Get physical address from kernel virtual */
extern vm_map_offset_t kvtophys64(vm_map_offset_t va);				/* Get 64-bit physical address from kernel virtual */
extern vm_offset_t	pmap_map(
                             vm_offset_t        virt,
                             vm_map_offset_t        start,
                             vm_map_offset_t        end,
                             vm_prot_t	prot,
                             unsigned int	flags);

extern boolean_t	pmap_map_bd(
                             vm_offset_t        virt,
                             vm_map_offset_t        start,
                             vm_map_offset_t        end,
                             vm_prot_t	prot,
                             unsigned int	flags);

extern kern_return_t    pmap_add_physical_memory(vm_offset_t spa,
						 vm_offset_t epa,
						 boolean_t available,
						 unsigned int attr);
extern void		pmap_bootstrap(uint64_t msize,
				       vm_offset_t *first_avail,
				       unsigned int kmapsize);

extern    vm_offset_t
pmap_get_phys(pmap_t pmap, void* virt);

extern vm_offset_t pmap_boot_map(vm_size_t size);

extern void sync_cache64(addr64_t pa, unsigned length);
extern void sync_ppage(ppnum_t pa);
extern void	sync_cache_virtual(vm_offset_t va, unsigned length);
extern void flush_dcache(vm_offset_t va, unsigned length, boolean_t phys);
extern void flush_dcache64(addr64_t va, unsigned length, boolean_t phys);
extern void invalidate_dcache(vm_offset_t va, unsigned length, boolean_t phys);
extern void invalidate_dcache64(addr64_t va, unsigned length, boolean_t phys);
extern void invalidate_icache(vm_offset_t va, unsigned length, boolean_t phys);
extern void invalidate_icache64(addr64_t va, unsigned length, boolean_t phys);
extern void pmap_map_block(pmap_t pmap, addr64_t va, ppnum_t pa, uint32_t size, vm_prot_t prot, int attr, unsigned int flags);
extern int pmap_map_block_rc(pmap_t pmap, addr64_t va, ppnum_t pa, uint32_t size, vm_prot_t prot, int attr, unsigned int flags);

extern ppnum_t pmap_find_phys(pmap_t pmap, addr64_t va);
extern void MapUserMemoryWindowInit(void);
extern addr64_t MapUserMemoryWindow(vm_map_t map, addr64_t va);
extern boolean_t pmap_eligible_for_execute(ppnum_t pa);
extern int pmap_list_resident_pages(struct pmap *pmap, vm_offset_t *listp, int space);
extern void pmap_init_sharedpage(vm_offset_t cpg);
extern void pmap_disable_NX(pmap_t pmap);

extern boolean_t pmap_valid_page(ppnum_t pn);

extern void pt_fake_zone_init(int);
extern void pt_fake_zone_info(int *, vm_size_t *, vm_size_t *, vm_size_t *, vm_size_t *, 
			      uint64_t *, int *, int *, int *);

extern void pmap_create_sharedpage(void);

/* Not required for arm: */
static inline void pmap_set_4GB_pagezero(__unused pmap_t pmap) {}
static inline void pmap_clear_4GB_pagezero(__unused pmap_t pmap) {}
#endif

typedef struct mem_region {
	vm_offset_t start;                  /* Address of base of region */
	struct phys_entry *phys_table;      /* base of region's table */
	unsigned int end;                   /* End address+1 */
} mem_region_t;

typedef uint32_t  pmap_paddr_t;

#define PMAP_MEM_REGION_MAX 26
extern mem_region_t pmap_mem_regions[PMAP_MEM_REGION_MAX];
extern int          pmap_mem_regions_count;

void pmap_common_init(pmap_t pmap);
void pmap_static_init(void);

void l2_map_linear_range(uint32_t pa_cache_start, uint32_t phys_start, uint32_t phys_end);
void l2_cache_to_range(uint32_t pa_cache_start, uint32_t va, uint32_t tteb, uint32_t size, int zero);


#endif
