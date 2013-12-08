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
 * l2_map_linear_range/l2_cache_to_range is not my code.
 */

/*
 * ARM VM initialization
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
#include <console/serial_protos.h>
#include <mach-o/loader.h>
#include <libkern/kernel_mach_header.h>
#include <arm/pmap.h>
#include <arm/misc_protos.h>
#include <arm/low_globals.h>

extern void *version;
extern void *kmod;
extern void *kdp_trans_off;
extern void *kdp_read_io;
extern void *osversion;
extern void *flag_kdp_trigger_reboot;
extern void *manual_pkt;

#define align_down(p, s)        ((uintptr_t)(p)&~(s-1))
#define align_up(p, s)          align_down((uintptr_t)(p)+s-1, s)

/*
 * cpu_ttb contains the current TTB (translation-table
 * base) of the processor. This is a physical address.
 */
uint32_t cpu_ttb;

/*
 * The section offset is set by the bootloader.
 */
uint32_t sectionOffset = 0x1000;

/*
 * sane_size, max_mem and mem_size are controlled by arm_vm_init
 *
 * At the moment, sane_size is forced to the size of memory
 * the booter passes to the kernel.
 */
uint64_t sane_size = 0;
uint32_t mem_size = 0;

/*
 * The physical and virtual base of the kernel. These are used in 
 */
unsigned long gPhysBase = 0x0, gVirtBase = 0x0;
unsigned long gTopOfKernel;

addr64_t vm_last_addr = VM_MAX_KERNEL_ADDRESS;

/*
 * Break down the KASLR. (pronunced like "castle" but with a twangy accent).
 */

/*
 * These variables are initialized during vm init.
 */
ppnum_t vm_kernel_base_page;
vm_offset_t vm_kernel_base;
vm_offset_t vm_kernel_top;
vm_offset_t vm_kernel_stext;
vm_offset_t vm_kernel_etext;
vm_offset_t vm_kernel_slide;

/*
 * These both are initialized to the same value.
 */
uint64_t max_mem;
uint64_t mem_actual;

/*
 * The sectXxx stuff contains the current kernel Mach-O section information.
 */
vm_offset_t sectTEXTB;
unsigned long sectSizeTEXT;
vm_offset_t sectDATAB;
unsigned long sectSizeDATA;
vm_offset_t sectLINKB;
unsigned long sectSizeLINK;
vm_offset_t sectKLDB;
unsigned long sectSizeKLD;
vm_offset_t sectPRELINKB;
unsigned long sectSizePRELINK;
vm_offset_t sectHIBB;
unsigned long sectSizeHIB;

vm_offset_t segHIBB;
unsigned long segSizeHIB;
vm_offset_t segPRELINKB;
unsigned long segSizePRELINK;


vm_offset_t sectCONSTB;
unsigned long sectSizeConst;
boolean_t doconstro_override = FALSE;
static kernel_section_t *sectDCONST, *segDATA;

vm_offset_t end, etext, sdata, edata, sconstdata, econstdata;

extern void *ExceptionVectorsBase;

#define LOWGLO_BASE     0xFFFF0040
#define VECTORS_BASE    0xFFFF0000
#define MANAGED_BASE    0xC0000000  /* Can also be 0xA0000000, but iPhone OS 5 uses this address. */

/*
 * VBARNS support will break kdp for now.
 */
lowglo* lowGlo = (lowglo*)VECTORS_BASE;

/*
 * These both represent the first physical page we can use in the system,
 * and the end of the physical memory region.
 */
uint32_t first_avail;
uint32_t avail_end;

/*
 * The identity base represents a mapping that maps the entire kernel region
 * to the first section of memory.
 */
uint32_t identityBaseVA, identityCachePA;

/*
 * The managed page sections are dynamically mapped to free pages.
 */
uint32_t managedBaseVA, managedCachePA;

/*
 * The exception vectors are always mapped high on ARM.
 */
uint32_t exceptionVectVA, exceptionCachePA;
uint32_t sectionOffset;

/**
 * l2_map_linear_range
 *
 * Maps a linear range of physical pages starting at 'phys_start' and ending
 * at 'phys_end' into an L2 cache region starting at PA 'pa_cache_start'.
 */
void l2_map_linear_range(uint32_t pa_cache_start, uint32_t phys_start,
                         uint32_t phys_end)
{
    uint32_t pte_iter;
    uint32_t page_iter;
    uint32_t phys_iter;

    pte_iter = phys_to_virt(pa_cache_start);
    page_iter = (phys_end - phys_start) >> PAGE_SHIFT;
    phys_iter = phys_start;

    for (unsigned int i = 0; i < page_iter; i++) {
        unsigned int *ptv = (unsigned int *) pte_iter;

        if (phys_iter & ~L2_ADDR_MASK) {
            panic("l2_map_linear_range: Misaligned physical page!\n");
        }

        *ptv = phys_iter;

        *ptv |= L2_SMALL_PAGE;
        *ptv |= L2_ACCESS_PRW;

        /* Writethrough, no write allocate. */
        *ptv |= (MMU_TEXCB_CA_WT_NWA << 2);

        pte_iter += sizeof(unsigned int);
        phys_iter += PAGE_SIZE;
    }
}

void l2_map_linear_range_no_cache(uint32_t pa_cache_start, uint32_t phys_start,
                                  uint32_t phys_end)
{
    uint32_t pte_iter;
    uint32_t page_iter;
    uint32_t phys_iter;

    pte_iter = phys_to_virt(pa_cache_start);
    page_iter = (phys_end - phys_start) >> PAGE_SHIFT;
    phys_iter = phys_start;

    for (unsigned int i = 0; i < page_iter; i++) {
        unsigned int *ptv = (unsigned int *) pte_iter;

        if (phys_iter & ~L2_ADDR_MASK) {
            panic("l2_map_linear_range: Misaligned physical page!\n");
        }

        *ptv = phys_iter;

        *ptv |= L2_SMALL_PAGE;
        *ptv |= L2_ACCESS_PRW;

        pte_iter += sizeof(unsigned int);
        phys_iter += PAGE_SIZE;
    }
}

/**
 * l2_cache_to_range
 *
 * Binds a set of L2 entries starting at PA 'cache_start' to a translation table
 * 'tte' starting at virtual address 'va' and of size 'size'.
 */
void l2_cache_to_range(uint32_t pa_cache_start, uint32_t va, uint32_t tteb,
                       uint32_t size, int zero)
{
    uint32_t pte_iter = pa_cache_start;
    uint32_t tte_pbase;
    uint32_t tte_psize;

    tte_pbase = addr_to_tte(tteb, va);  /* Base of the L1 region for virtBase */
    tte_psize = ((size >> 20) << 2);    /* Size of the L1 region */

    /*
     * We must make sure that the managed mapping is
     * cleared, as this region may have been used by the
     * bootloader, leaving some stuff in it. We do not
     * do this for the identity mapping as every single page
     * of it is mapped anyway.
     *
     * thing size = (page count * 4)
     */
    if (zero) {
        bzero((void *) phys_to_virt(pte_iter), ((size >> PAGE_SHIFT) << 2));
    }

    /*
     * Create an L2 for every section in the given region 
     */
    for (unsigned int tte = tte_pbase; tte < (tte_pbase + tte_psize); tte += 4) {
        unsigned int *ttv = (unsigned int *) tte;

        if (pte_iter & ~L1_PTE_ADDR_MASK) {
            panic("l2_cache_to_range: Misaligned L2 table %x!\n", pte_iter);
        }

        *ttv = pte_iter;
        *ttv |= L1_TYPE_PTE;

        pte_iter += L2_SIZE;
    }
}

/**
 * verify_lowGlo
 *
 * Verify that the ARM exception vectors are mapped in the right place. Our string
 * is "Scolecit" for right now. If they aren't, panic and halt the system.
 */
static void verify_lowGlo(void)
{
    char *lowGloString = (char *) LOWGLO_BASE;

    if (strncmp(lowGloString, "Scolecit", 8) != 0) {
        panic
            ("Invalid signature for lowGlo in vectors, got %s, was expecting %s\n",
             lowGloString, "Scolecit");
    }

    kprintf("lowGlo verification string: %s\n", lowGloString);
}

/**
 * arm_vm_init
 *
 * Initialize basic MMU mappings (L1/L2) for identity, managed and exception
 * vector page tables. Additionally, kick off necessary subsystems such as
 * kprintf so that we can get out of semihosting debug output (if enabled)
 * and onto an actual serial port or something. Whatever.
 */
void arm_vm_init(uint32_t mem_limit, boot_args * args)
{
    uint32_t gMemSize;

    /*
     * ARM vm init starting up. 
     */
    kdb_printf("\tboot_args:               0x%08x\n"
               "\tboot_args->virtBase:     0x%08x\n"
               "\tboot_args->physBase:     0x%08x\n"
               "\tboot_args->topOfKernel:  0x%08x\n"
               "\tboot_args->memSize:      0x%08x\n", (unsigned int)args, args->virtBase,
               args->physBase, args->topOfKernelData, args->memSize);

    /*
     * Set up some globals. 
     */
    gPhysBase = args->physBase;
    gVirtBase = args->virtBase;
    gMemSize = args->memSize;
    gTopOfKernel = args->topOfKernelData;
    max_mem = mem_size = sane_size = gMemSize;

    /*
     * Map L2 tables for identity. The initial bootloader sets up section maps for one L1, so we are next. 
     */
    cpu_ttb = gTopOfKernel + L1_SIZE;
    bzero((void*)phys_to_virt(cpu_ttb), L1_SIZE);

    identityBaseVA = gVirtBase;
    identityCachePA = cpu_ttb + L1_SIZE;    /* After the first initial TTB. */
    kdb_printf("arm_vm_init: L2 address for identity mappings...\n"
               "\tmapping VA: 0x%08x\n" "\tmapping PA: 0x%08x\n",
               identityBaseVA, identityCachePA);

    managedBaseVA = MANAGED_BASE;
    managedCachePA = identityCachePA + l2_size(gMemSize);
    kdb_printf("arm_vm_init: L2 address for kernel managed mappings...\n"
               "\tmapping VA: 0x%08x\n" "\tmapping PA: 0x%08x\n", managedBaseVA,
               managedCachePA);

    /*
     * Bit generous.. 
     */
    first_avail = managedCachePA + l2_size(0x40000000);

    /*
     * Configure tables for identity mapping.. 
     */
    kdb_printf("arm_vm_init: configuring tables for identity mapping...\n");
    l2_cache_to_range(identityCachePA, identityBaseVA, phys_to_virt(cpu_ttb),
                      gMemSize, TRUE);

    l2_cache_to_range(managedCachePA, managedBaseVA, phys_to_virt(cpu_ttb),
                      gMemSize, TRUE);

    /*
     * Create the first identity mappings. 
     */
    kdb_printf
        ("arm_vm_init: creating main identity mapping (section offset is 0x%x).\n",
         sectionOffset);
    l2_map_linear_range(identityCachePA, gPhysBase - sectionOffset,
                        gPhysBase + gMemSize);

    /*
     * Set high exception vectors.. Steal one page from first_avail. 
     */
    kdb_printf("arm_vm_init: exception vectors are at 0x%08x.\n",
               &ExceptionVectorsBase);

    /*
     * Map them... 
     */
#ifndef USE_VBAR_EXCVECT
    uint32_t *vecpt_start = (uint32_t*)(first_avail), *vectp, *va_vecpt;
    vectp = (uint32_t *) addr_to_tte(phys_to_virt(cpu_ttb), VECTORS_BASE);
    *vectp = (((uint32_t) vecpt_start) | L1_TYPE_PTE);
    va_vecpt = (vm_offset_t)phys_to_virt(vecpt_start) + pte_offset(VECTORS_BASE);
    *va_vecpt =
        virt_to_phys(&ExceptionVectorsBase) | L2_ACCESS_PRW | L2_SMALL_PAGE;
#endif

    /*
     * Burn it away... 
     */
#if defined(BOARD_CONFIG_S5L8930X) || defined(BOARD_CONFIG_S5L8920X) || defined(BOARD_CONFIG_S5L8922X)
    first_avail += 6144 * L1_SIZE;  /* temporary..... */
#else
    first_avail += 1 * L1_SIZE; /* temporary..... */
#endif
    avail_end = gPhysBase + gMemSize;

    /*
     * Clean caches and flush TLBs.. 
     */
    kdb_printf("arm_vm_init: switching translation-tables now...\n");
    set_mmu_ttb(cpu_ttb);
    set_mmu_ttb_alt(cpu_ttb);
    set_mmu_ttbcr(2);
    flush_mmu_tlb();

    /*
     * Set settings for pmap. 
     */
    kernel_pmap->pm_l1_phys = cpu_ttb;
    kernel_pmap->pm_l1_virt = phys_to_virt(cpu_ttb);

    /*
     * Set up segment information. 
     */
    kdb_printf("arm_vm_init: setting up segment information...\n");
    sectTEXTB =
        (vm_offset_t) (uint32_t *) getsegdatafromheader(&_mh_execute_header,
                                                        "__TEXT",
                                                        &sectSizeTEXT);
    sectDATAB =
        (vm_offset_t) (uint32_t *) getsegdatafromheader(&_mh_execute_header,
                                                        "__DATA",
                                                        &sectSizeDATA);
    sectLINKB =
        (vm_offset_t) (uint32_t *) getsegdatafromheader(&_mh_execute_header,
                                                        "__LINKEDIT",
                                                        &sectSizeLINK);
    sectKLDB =
        (vm_offset_t) (uint32_t *) getsegdatafromheader(&_mh_execute_header,
                                                        "__KLD", &sectSizeKLD);
    sectHIBB =
        (vm_offset_t) (uint32_t *) getsegdatafromheader(&_mh_execute_header,
                                                        "__HIB", &sectSizeHIB);
    sectPRELINKB =
        (vm_offset_t) (uint32_t *) getsegdatafromheader(&_mh_execute_header,
                                                        "__PRELINK_TEXT",
                                                        &sectSizePRELINK);
    etext = (vm_offset_t) sectTEXTB + sectSizeTEXT;
    edata = (vm_offset_t) sectDATAB + sectSizeDATA;
    sdata = (vm_offset_t) sectDATAB;
    end = round_page(getlastaddr());    /* Force end to next page */
    vm_kernel_slide = 0;
    vm_kernel_etext = etext;
    vm_kernel_stext = sectTEXTB;
    vm_kernel_top = phys_to_virt(gTopOfKernel);
    vm_kernel_base = gVirtBase;

    segDATA = getsegbynamefromheader(&_mh_execute_header,
                    "__DATA");
    sectDCONST = getsectbynamefromheader(&_mh_execute_header,
                    "__DATA", "__const");

    segHIBB  = (vm_offset_t) getsegdatafromheader(&_mh_execute_header,
                    "__HIB", &segSizeHIB);
    segPRELINKB = (vm_offset_t) getsegdatafromheader(&_mh_execute_header,
                    "__PRELINK_TEXT", &segSizePRELINK);

    sectCONSTB = (vm_offset_t) sectDCONST->addr;
    sectSizeConst = sectDCONST->size;
    sconstdata = sectCONSTB;
    econstdata = sectCONSTB + sectSizeConst;

    if (sectSizeConst & PAGE_MASK) {
        kernel_section_t *ns = nextsect(segDATA, sectDCONST);
        if (ns && !(ns->addr & PAGE_MASK))
            doconstro_override = TRUE;
    } else
        doconstro_override = TRUE;

    /*
     * Bootstrap pmap. 
     */
    uint32_t bootstrap_addr = managedBaseVA + 0x1000;
    pmap_bootstrap(gMemSize, (vm_offset_t *) & bootstrap_addr, 0);

    /*
     * Subsystems. 
     */
    printf_init();
    panic_init();

    PE_init_kprintf(TRUE);
    kprintf("kprintf initialized!\n");

#ifndef USE_VBAR_EXCVECT
    /*
     * Verify vectors are in the right place. 
     */
    verify_lowGlo();

    /*
     * Set up low globals
     */
    lowGlo->lgOSVersion = (uint32_t) &version;
    lowGlo->lgRebootFlag = (uint32_t) &flag_kdp_trigger_reboot;
    lowGlo->lgManualPacket = (uint32_t) &manual_pkt;
#endif

    return;
}
