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
/*-
 * Copyright (c) 1995 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpu.c
 *
 * Probing and configuration for the master CPU
 *
 * Created      : 10/10/95
 */
/*
 * CPU bootstrap, used to create core structures for the boot processor.
 *
 * SMP support coming soon.
 */

#include <kern/kalloc.h>
#include <kern/misc_protos.h>
#include <kern/machine.h>
#include <mach/processor_info.h>
#include <arm/pmap.h>
#include <arm/machine_routines.h>
#include <vm/vm_kern.h>
#include <kern/timer_call.h>
#include <kern/etimer.h>
#include <kern/processor.h>
#include <arm/misc_protos.h>
#include <mach/machine.h>
#include <arm/arch.h>
#include "proc_reg.h"

struct processor BootProcessor;
cpu_data_t cpu_data_master;

enum cpu_class {
    CPU_CLASS_NONE,
    CPU_CLASS_ARM2,
    CPU_CLASS_ARM2AS,
    CPU_CLASS_ARM3,
    CPU_CLASS_ARM6,
    CPU_CLASS_ARM7,
    CPU_CLASS_ARM7TDMI,
    CPU_CLASS_ARM8,
    CPU_CLASS_ARM9TDMI,
    CPU_CLASS_ARM9ES,
    CPU_CLASS_ARM9EJS,
    CPU_CLASS_ARM10E,
    CPU_CLASS_ARM10EJ,
    CPU_CLASS_CORTEXA,
    CPU_CLASS_SA1,
    CPU_CLASS_XSCALE,
    CPU_CLASS_ARM11J,
    CPU_CLASS_MARVELL
};

static const char *const generic_steppings[16] = {
    "rev 0", "rev 1", "rev 2", "rev 3",
    "rev 4", "rev 5", "rev 6", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

static const char *const sa110_steppings[16] = {
    "rev 0", "step J", "step K", "step S",
    "step T", "rev 5", "rev 6", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

static const char *const sa1100_steppings[16] = {
    "rev 0", "step B", "step C", "rev 3",
    "rev 4", "rev 5", "rev 6", "rev 7",
    "step D", "step E", "rev 10" "step G",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

static const char *const sa1110_steppings[16] = {
    "step A-0", "rev 1", "rev 2", "rev 3",
    "step B-0", "step B-1", "step B-2", "step B-3",
    "step B-4", "step B-5", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

static const char *const ixp12x0_steppings[16] = {
    "(IXP1200 step A)", "(IXP1200 step B)",
    "rev 2", "(IXP1200 step C)",
    "(IXP1200 step D)", "(IXP1240/1250 step A)",
    "(IXP1240 step B)", "(IXP1250 step B)",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

static const char *const xscale_steppings[16] = {
    "step A-0", "step A-1", "step B-0", "step C-0",
    "step D-0", "rev 5", "rev 6", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

static const char *const i80219_steppings[16] = {
    "step A-0", "rev 1", "rev 2", "rev 3",
    "rev 4", "rev 5", "rev 6", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

static const char *const i80321_steppings[16] = {
    "step A-0", "step B-0", "rev 2", "rev 3",
    "rev 4", "rev 5", "rev 6", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

static const char *const i81342_steppings[16] = {
    "step A-0", "rev 1", "rev 2", "rev 3",
    "rev 4", "rev 5", "rev 6", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

/* Steppings for PXA2[15]0 */
static const char *const pxa2x0_steppings[16] = {
    "step A-0", "step A-1", "step B-0", "step B-1",
    "step B-2", "step C-0", "rev 6", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

/* Steppings for PXA255/26x.
 * rev 5: PXA26x B0, rev 6: PXA255 A0
 */
static const char *const pxa255_steppings[16] = {
    "rev 0", "rev 1", "rev 2", "step A-0",
    "rev 4", "step B-0", "step A-0", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

/* Stepping for PXA27x */
static const char *const pxa27x_steppings[16] = {
    "step A-0", "step A-1", "step B-0", "step B-1",
    "step C-0", "rev 5", "rev 6", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

static const char *const ixp425_steppings[16] = {
    "step 0 (A0)", "rev 1 (ARMv5TE)", "rev 2", "rev 3",
    "rev 4", "rev 5", "rev 6", "rev 7",
    "rev 8", "rev 9", "rev 10", "rev 11",
    "rev 12", "rev 13", "rev 14", "rev 15",
};

struct cpuidtab {
    u_int32_t cpuid;
    enum cpu_class cpu_class;
    const char *cpu_name;
    const char *const *cpu_steppings;
};

const struct cpuidtab cpuids[] = {
    {CPU_ID_ARM2, CPU_CLASS_ARM2, "ARM2",
     generic_steppings},
    {CPU_ID_ARM250, CPU_CLASS_ARM2AS, "ARM250",
     generic_steppings},

    {CPU_ID_ARM3, CPU_CLASS_ARM3, "ARM3",
     generic_steppings},

    {CPU_ID_ARM600, CPU_CLASS_ARM6, "ARM600",
     generic_steppings},
    {CPU_ID_ARM610, CPU_CLASS_ARM6, "ARM610",
     generic_steppings},
    {CPU_ID_ARM620, CPU_CLASS_ARM6, "ARM620",
     generic_steppings},

    {CPU_ID_ARM700, CPU_CLASS_ARM7, "ARM700",
     generic_steppings},
    {CPU_ID_ARM710, CPU_CLASS_ARM7, "ARM710",
     generic_steppings},
    {CPU_ID_ARM7500, CPU_CLASS_ARM7, "ARM7500",
     generic_steppings},
    {CPU_ID_ARM710A, CPU_CLASS_ARM7, "ARM710a",
     generic_steppings},
    {CPU_ID_ARM7500FE, CPU_CLASS_ARM7, "ARM7500FE",
     generic_steppings},
    {CPU_ID_ARM710T, CPU_CLASS_ARM7TDMI, "ARM710T",
     generic_steppings},
    {CPU_ID_ARM720T, CPU_CLASS_ARM7TDMI, "ARM720T",
     generic_steppings},
    {CPU_ID_ARM740T8K, CPU_CLASS_ARM7TDMI, "ARM740T (8 KB cache)",
     generic_steppings},
    {CPU_ID_ARM740T4K, CPU_CLASS_ARM7TDMI, "ARM740T (4 KB cache)",
     generic_steppings},

    {CPU_ID_ARM810, CPU_CLASS_ARM8, "ARM810",
     generic_steppings},

    {CPU_ID_ARM920T, CPU_CLASS_ARM9TDMI, "ARM920T",
     generic_steppings},
    {CPU_ID_ARM920T_ALT, CPU_CLASS_ARM9TDMI, "ARM920T",
     generic_steppings},
    {CPU_ID_ARM922T, CPU_CLASS_ARM9TDMI, "ARM922T",
     generic_steppings},
    {CPU_ID_ARM926EJS, CPU_CLASS_ARM9EJS, "ARM926EJ-S",
     generic_steppings},
    {CPU_ID_ARM940T, CPU_CLASS_ARM9TDMI, "ARM940T",
     generic_steppings},
    {CPU_ID_ARM946ES, CPU_CLASS_ARM9ES, "ARM946E-S",
     generic_steppings},
    {CPU_ID_ARM966ES, CPU_CLASS_ARM9ES, "ARM966E-S",
     generic_steppings},
    {CPU_ID_ARM966ESR1, CPU_CLASS_ARM9ES, "ARM966E-S",
     generic_steppings},
    {CPU_ID_FA526, CPU_CLASS_ARM9TDMI, "FA526",
     generic_steppings},
    {CPU_ID_FA626TE, CPU_CLASS_ARM9ES, "FA626TE",
     generic_steppings},

    {CPU_ID_TI925T, CPU_CLASS_ARM9TDMI, "TI ARM925T",
     generic_steppings},

    {CPU_ID_ARM1020E, CPU_CLASS_ARM10E, "ARM1020E",
     generic_steppings},
    {CPU_ID_ARM1022ES, CPU_CLASS_ARM10E, "ARM1022E-S",
     generic_steppings},
    {CPU_ID_ARM1026EJS, CPU_CLASS_ARM10EJ, "ARM1026EJ-S",
     generic_steppings},

    {CPU_ID_CORTEXA5, CPU_CLASS_CORTEXA, "Cortex A5",
     generic_steppings},
    {CPU_ID_CORTEXA7, CPU_CLASS_CORTEXA, "Cortex A7",
     generic_steppings},
    {CPU_ID_CORTEXA8R0, CPU_CLASS_CORTEXA, "Cortex A8-r0",
     generic_steppings},
    {CPU_ID_CORTEXA8R1, CPU_CLASS_CORTEXA, "Cortex A8-r1",
     generic_steppings},
    {CPU_ID_CORTEXA8R2, CPU_CLASS_CORTEXA, "Cortex A8-r2",
     generic_steppings},
    {CPU_ID_CORTEXA8R3, CPU_CLASS_CORTEXA, "Cortex A8-r3",
     generic_steppings},
    {CPU_ID_CORTEXA9R1, CPU_CLASS_CORTEXA, "Cortex A9-r1",
     generic_steppings},
    {CPU_ID_CORTEXA9R2, CPU_CLASS_CORTEXA, "Cortex A9-r2",
     generic_steppings},
    {CPU_ID_CORTEXA9R3, CPU_CLASS_CORTEXA, "Cortex A9-r3",
     generic_steppings},
    {CPU_ID_CORTEXA15, CPU_CLASS_CORTEXA, "Cortex A15",
     generic_steppings},

    {CPU_ID_SA110, CPU_CLASS_SA1, "SA-110",
     sa110_steppings},
    {CPU_ID_SA1100, CPU_CLASS_SA1, "SA-1100",
     sa1100_steppings},
    {CPU_ID_SA1110, CPU_CLASS_SA1, "SA-1110",
     sa1110_steppings},

    {CPU_ID_IXP1200, CPU_CLASS_SA1, "IXP1200",
     ixp12x0_steppings},

    {CPU_ID_80200, CPU_CLASS_XSCALE, "i80200",
     xscale_steppings},

    {CPU_ID_80321_400, CPU_CLASS_XSCALE, "i80321 400MHz",
     i80321_steppings},
    {CPU_ID_80321_600, CPU_CLASS_XSCALE, "i80321 600MHz",
     i80321_steppings},
    {CPU_ID_80321_400_B0, CPU_CLASS_XSCALE, "i80321 400MHz",
     i80321_steppings},
    {CPU_ID_80321_600_B0, CPU_CLASS_XSCALE, "i80321 600MHz",
     i80321_steppings},

    {CPU_ID_81342, CPU_CLASS_XSCALE, "i81342",
     i81342_steppings},

    {CPU_ID_80219_400, CPU_CLASS_XSCALE, "i80219 400MHz",
     i80219_steppings},
    {CPU_ID_80219_600, CPU_CLASS_XSCALE, "i80219 600MHz",
     i80219_steppings},

    {CPU_ID_PXA27X, CPU_CLASS_XSCALE, "PXA27x",
     pxa27x_steppings},
    {CPU_ID_PXA250A, CPU_CLASS_XSCALE, "PXA250",
     pxa2x0_steppings},
    {CPU_ID_PXA210A, CPU_CLASS_XSCALE, "PXA210",
     pxa2x0_steppings},
    {CPU_ID_PXA250B, CPU_CLASS_XSCALE, "PXA250",
     pxa2x0_steppings},
    {CPU_ID_PXA210B, CPU_CLASS_XSCALE, "PXA210",
     pxa2x0_steppings},
    {CPU_ID_PXA250C, CPU_CLASS_XSCALE, "PXA255",
     pxa255_steppings},
    {CPU_ID_PXA210C, CPU_CLASS_XSCALE, "PXA210",
     pxa2x0_steppings},

    {CPU_ID_IXP425_533, CPU_CLASS_XSCALE, "IXP425 533MHz",
     ixp425_steppings},
    {CPU_ID_IXP425_400, CPU_CLASS_XSCALE, "IXP425 400MHz",
     ixp425_steppings},
    {CPU_ID_IXP425_266, CPU_CLASS_XSCALE, "IXP425 266MHz",
     ixp425_steppings},

    /*
     * XXX ixp435 steppings? 
     */
    {CPU_ID_IXP435, CPU_CLASS_XSCALE, "IXP435",
     ixp425_steppings},

    {CPU_ID_ARM1136JS, CPU_CLASS_ARM11J, "ARM1136J-S",
     generic_steppings},
    {CPU_ID_ARM1136JSR1, CPU_CLASS_ARM11J, "ARM1136J-S R1",
     generic_steppings},
    {CPU_ID_ARM1176JZS, CPU_CLASS_ARM11J, "ARM1176JZ-S",
     generic_steppings},

    {CPU_ID_MV88FR131, CPU_CLASS_MARVELL, "Feroceon 88FR131",
     generic_steppings},

    {CPU_ID_MV88FR571_VD, CPU_CLASS_MARVELL, "Feroceon 88FR571-VD",
     generic_steppings},
    {CPU_ID_MV88SV581X_V7, CPU_CLASS_MARVELL, "Sheeva 88SV581x",
     generic_steppings},
    {CPU_ID_ARM_88SV581X_V7, CPU_CLASS_MARVELL, "Sheeva 88SV581x",
     generic_steppings},
    {CPU_ID_MV88SV584X_V7, CPU_CLASS_MARVELL, "Sheeva 88SV584x",
     generic_steppings},

    {0, CPU_CLASS_NONE, NULL, NULL}
};

struct cpu_classtab {
    const char *class_name;
    const char *class_option;
};

const struct cpu_classtab cpu_classes[] = {
    {"unknown", NULL},          /* CPU_CLASS_NONE */
    {"ARM2", "CPU_ARM2"},       /* CPU_CLASS_ARM2 */
    {"ARM2as", "CPU_ARM250"},   /* CPU_CLASS_ARM2AS */
    {"ARM3", "CPU_ARM3"},       /* CPU_CLASS_ARM3 */
    {"ARM6", "CPU_ARM6"},       /* CPU_CLASS_ARM6 */
    {"ARM7", "CPU_ARM7"},       /* CPU_CLASS_ARM7 */
    {"ARM7TDMI", "CPU_ARM7TDMI"},   /* CPU_CLASS_ARM7TDMI */
    {"ARM8", "CPU_ARM8"},       /* CPU_CLASS_ARM8 */
    {"ARM9TDMI", "CPU_ARM9TDMI"},   /* CPU_CLASS_ARM9TDMI */
    {"ARM9E-S", "CPU_ARM9E"},   /* CPU_CLASS_ARM9ES */
    {"ARM9EJ-S", "CPU_ARM9E"},  /* CPU_CLASS_ARM9EJS */
    {"ARM10E", "CPU_ARM10"},    /* CPU_CLASS_ARM10E */
    {"ARM10EJ", "CPU_ARM10"},   /* CPU_CLASS_ARM10EJ */
    {"Cortex-A", "CPU_CORTEXA"},    /* CPU_CLASS_CORTEXA */
    {"SA-1", "CPU_SA110"},      /* CPU_CLASS_SA1 */
    {"XScale", "CPU_XSCALE_..."},   /* CPU_CLASS_XSCALE */
    {"ARM11J", "CPU_ARM11"},    /* CPU_CLASS_ARM11J */
    {"Marvell", "CPU_MARVELL"}, /* CPU_CLASS_MARVELL */
};

/*
 * Report the type of the specified arm processor. This uses the generic and
 * arm specific information in the cpu structure to identify the processor.
 * The remaining fields in the cpu structure are filled in appropriately.
 */

static const char *const wtnames[] = {
    "write-through",
    "write-back",
    "write-back",
    "**unknown 3**",
    "**unknown 4**",
    "write-back-locking",       /* XXX XScale-specific? */
    "write-back-locking-A",
    "write-back-locking-B",
    "**unknown 8**",
    "**unknown 9**",
    "**unknown 10**",
    "**unknown 11**",
    "**unknown 12**",
    "**unknown 13**",
    "write-back-locking-C",
    "**unknown 15**",
};

int arm_picache_size;
int arm_picache_line_size;
int arm_picache_ways;

int arm_pdcache_size;           /* and unified */
int arm_pdcache_line_size;
int arm_pdcache_ways;

int arm_pcache_type;
int arm_pcache_unified;

int arm_dcache_align;
int arm_dcache_align_mask;

u_int arm_cache_level;
u_int arm_cache_type[14];
u_int arm_cache_loc;

static int arm_dcache_l2_nsets;
static int arm_dcache_l2_assoc;
static int arm_dcache_l2_linesize;

void get_cachetype_cp15()
{
    u_int ctype, isize, dsize, cpuid;
    u_int clevel, csize, i, sel;
    u_int multiplier;
    u_char type;

    __asm __volatile("mrc p15, 0, %0, c0, c0, 1":"=r"(ctype));

    cpuid = armreg_midr_read();
    /*
     * ...and thus spake the ARM ARM:
     *
     * If an <opcode2> value corresponding to an unimplemented or
     * reserved ID register is encountered, the System Control
     * processor returns the value of the main ID register.
     */
    if (ctype == cpuid)
        goto out;

    if (CPU_CT_FORMAT(ctype) == CPU_CT_ARMV7) {
        __asm __volatile("mrc p15, 1, %0, c0, c0, 1":"=r"(clevel));
        arm_cache_level = clevel;
        arm_cache_loc = CPU_CLIDR_LOC(arm_cache_level);
        i = 0;
        while ((type = (clevel & 0x7)) && i < 7) {
            if (type == CACHE_DCACHE || type == CACHE_UNI_CACHE ||
                type == CACHE_SEP_CACHE) {
                sel = i << 1;
                __asm __volatile("mcr p15, 2, %0, c0, c0, 0"::"r"(sel));
                __asm __volatile("mrc p15, 1, %0, c0, c0, 0":"=r"(csize));
                arm_cache_type[sel] = csize;
                arm_dcache_align = 1 << (CPUV7_CT_xSIZE_LEN(csize) + 4);
                arm_dcache_align_mask = arm_dcache_align - 1;
            }
            if (type == CACHE_ICACHE || type == CACHE_SEP_CACHE) {
                sel = (i << 1) | 1;
                __asm __volatile("mcr p15, 2, %0, c0, c0, 0"::"r"(sel));
                __asm __volatile("mrc p15, 1, %0, c0, c0, 0":"=r"(csize));
                arm_cache_type[sel] = csize;
            }
            i++;
            clevel >>= 3;
        }
    } else {
        if ((ctype & CPU_CT_S) == 0)
            arm_pcache_unified = 1;

        /*
         * If you want to know how this code works, go read the ARM ARM.
         */

        arm_pcache_type = CPU_CT_CTYPE(ctype);

        if (arm_pcache_unified == 0) {
            isize = CPU_CT_ISIZE(ctype);
            multiplier = (isize & CPU_CT_xSIZE_M) ? 3 : 2;
            arm_picache_line_size = 1U << (CPU_CT_xSIZE_LEN(isize) + 3);
            if (CPU_CT_xSIZE_ASSOC(isize) == 0) {
                if (isize & CPU_CT_xSIZE_M)
                    arm_picache_line_size = 0;  /* not present */
                else
                    arm_picache_ways = 1;
            } else {
                arm_picache_ways = multiplier << (CPU_CT_xSIZE_ASSOC(isize) - 1);
            }
            arm_picache_size = multiplier << (CPU_CT_xSIZE_SIZE(isize) + 8);
        }

        dsize = CPU_CT_DSIZE(ctype);
        multiplier = (dsize & CPU_CT_xSIZE_M) ? 3 : 2;
        arm_pdcache_line_size = 1U << (CPU_CT_xSIZE_LEN(dsize) + 3);
        if (CPU_CT_xSIZE_ASSOC(dsize) == 0) {
            if (dsize & CPU_CT_xSIZE_M)
                arm_pdcache_line_size = 0;  /* not present */
            else
                arm_pdcache_ways = 1;
        } else {
            arm_pdcache_ways = multiplier << (CPU_CT_xSIZE_ASSOC(dsize) - 1);
        }
        arm_pdcache_size = multiplier << (CPU_CT_xSIZE_SIZE(dsize) + 8);

        arm_dcache_align = arm_pdcache_line_size;

        arm_dcache_l2_assoc = CPU_CT_xSIZE_ASSOC(dsize) + multiplier - 2;
        arm_dcache_l2_linesize = CPU_CT_xSIZE_LEN(dsize) + 3;
        arm_dcache_l2_nsets = 6 + CPU_CT_xSIZE_SIZE(dsize) -
            CPU_CT_xSIZE_ASSOC(dsize) - CPU_CT_xSIZE_LEN(dsize);

 out:
        arm_dcache_align_mask = arm_dcache_align - 1;
    }
}

static void print_enadis(int enadis, char *s)
{

    kprintf(" %s %sabled", s, (enadis == 0) ? "dis" : "en");
}

int ctrl;
enum cpu_class cpu_class = CPU_CLASS_NONE;

u_int cpu_pfr(int num)
{
    u_int feat = 0;

    switch (num) {
    case 0:
 __asm __volatile("mrc p15, 0, %0, c0, c1, 0":"=r"(feat));
        break;
    case 1:
 __asm __volatile("mrc p15, 0, %0, c0, c1, 1":"=r"(feat));
        break;
    default:
        panic("Processor Feature Register %d not implemented", num);
        break;
    }

    return (feat);
}

static
void identify_armv7(void)
{
    u_int feature;

    kprintf("Supported features:");
    /*
     * Get Processor Feature Register 0 
     */
    feature = cpu_pfr(0);

    if (feature & ARM_PFR0_ARM_ISA_MASK)
        kprintf(" ARM_ISA");

    if (feature & ARM_PFR0_THUMB2)
        kprintf(" THUMB2");
    else if (feature & ARM_PFR0_THUMB)
        kprintf(" THUMB");

    if (feature & ARM_PFR0_JAZELLE_MASK)
        kprintf(" JAZELLE");

    if (feature & ARM_PFR0_THUMBEE_MASK)
        kprintf(" THUMBEE");

    /*
     * Get Processor Feature Register 1 
     */
    feature = cpu_pfr(1);

    if (feature & ARM_PFR1_ARMV4_MASK)
        kprintf(" ARMv4");

    if (feature & ARM_PFR1_SEC_EXT_MASK)
        kprintf(" Security_Ext");

    if (feature & ARM_PFR1_MICROCTRL_MASK)
        kprintf(" M_profile");

    kprintf("\n");
}

void identify_arm_cpu(void)
{
    u_int cpuid, reg, size, sets, ways;
    u_int8_t type, linesize;
    int i;

    cpuid = armreg_midr_read();
    ctrl = armreg_sctrl_read();

    if (cpuid == 0) {
        kprintf("Processor failed probe - no CPU ID\n");
        return;
    }

    for (i = 0; cpuids[i].cpuid != 0; i++)
        if (cpuids[i].cpuid == (cpuid & CPU_ID_CPU_MASK)) {
            cpu_class = cpuids[i].cpu_class;
            kprintf("CPU: %s %s (%s core)\n",
                   cpuids[i].cpu_name,
                   cpuids[i].cpu_steppings[cpuid &
                                           CPU_ID_REVISION_MASK],
                   cpu_classes[cpu_class].class_name);
            break;
        }
    if (cpuids[i].cpuid == 0)
        kprintf("unknown CPU (ID = 0x%x)\n", cpuid);

    kprintf(" ");

    if ((cpuid & CPU_ID_ARCH_MASK) == CPU_ID_CPUID_SCHEME) {
        identify_armv7();
    } else {
        if (ctrl & CPU_CONTROL_BEND_ENABLE)
            kprintf(" Big-endian");
        else
            kprintf(" Little-endian");

        switch (cpu_class) {
        case CPU_CLASS_ARM6:
        case CPU_CLASS_ARM7:
        case CPU_CLASS_ARM7TDMI:
        case CPU_CLASS_ARM8:
            print_enadis(ctrl & CPU_CONTROL_IDC_ENABLE, "IDC");
            break;
        case CPU_CLASS_ARM9TDMI:
        case CPU_CLASS_ARM9ES:
        case CPU_CLASS_ARM9EJS:
        case CPU_CLASS_ARM10E:
        case CPU_CLASS_ARM10EJ:
        case CPU_CLASS_SA1:
        case CPU_CLASS_XSCALE:
        case CPU_CLASS_ARM11J:
        case CPU_CLASS_MARVELL:
            print_enadis(ctrl & CPU_CONTROL_DC_ENABLE, "DC");
            print_enadis(ctrl & CPU_CONTROL_IC_ENABLE, "IC");
#ifdef CPU_XSCALE_81342
            print_enadis(ctrl & CPU_CONTROL_L2_ENABLE, "L2");
#endif
#if defined(SOC_MV_KIRKWOOD) || defined(SOC_MV_DISCOVERY)
            i = sheeva_control_ext(0, 0);
            print_enadis(i & MV_WA_ENABLE, "WA");
            print_enadis(i & MV_DC_STREAM_ENABLE, "DC streaming");
            kprintf("\n ");
            print_enadis((i & MV_BTB_DISABLE) == 0, "BTB");
            print_enadis(i & MV_L2_ENABLE, "L2");
            print_enadis((i & MV_L2_PREFETCH_DISABLE) == 0, "L2 prefetch");
            kprintf("\n ");
#endif
            break;
        default:
            break;
        }
    }

    print_enadis(ctrl & CPU_CONTROL_WBUF_ENABLE, "WB");
    if (ctrl & CPU_CONTROL_LABT_ENABLE)
        kprintf(" LABT");
    else
        kprintf(" EABT");

    print_enadis(ctrl & CPU_CONTROL_BPRD_ENABLE, "branch prediction");
    kprintf("\n");

    if (arm_cache_level) {
        kprintf("LoUU:%d LoC:%d LoUIS:%d \n", CPU_CLIDR_LOUU(arm_cache_level) + 1,
               arm_cache_loc, CPU_CLIDR_LOUIS(arm_cache_level) + 1);
        i = 0;
        while (((type = CPU_CLIDR_CTYPE(arm_cache_level, i)) != 0) && i < 7) {
            kprintf("Cache level %d: \n", i + 1);
            if (type == CACHE_DCACHE || type == CACHE_UNI_CACHE ||
                type == CACHE_SEP_CACHE) {
                reg = arm_cache_type[2 * i];
                ways = CPUV7_CT_xSIZE_ASSOC(reg) + 1;
                sets = CPUV7_CT_xSIZE_SET(reg) + 1;
                linesize = 1 << (CPUV7_CT_xSIZE_LEN(reg) + 4);
                size = (ways * sets * linesize) / 1024;

                if (type == CACHE_UNI_CACHE)
                    kprintf(" %dKB/%dB %d-way unified cache", size, linesize, ways);
                else
                    kprintf(" %dKB/%dB %d-way data cache", size, linesize, ways);
                if (reg & CPUV7_CT_CTYPE_WT)
                    kprintf(" WT");
                if (reg & CPUV7_CT_CTYPE_WB)
                    kprintf(" WB");
                if (reg & CPUV7_CT_CTYPE_RA)
                    kprintf(" Read-Alloc");
                if (reg & CPUV7_CT_CTYPE_WA)
                    kprintf(" Write-Alloc");
                kprintf("\n");
            }

            if (type == CACHE_ICACHE || type == CACHE_SEP_CACHE) {
                reg = arm_cache_type[(2 * i) + 1];

                ways = CPUV7_CT_xSIZE_ASSOC(reg) + 1;
                sets = CPUV7_CT_xSIZE_SET(reg) + 1;
                linesize = 1 << (CPUV7_CT_xSIZE_LEN(reg) + 4);
                size = (ways * sets * linesize) / 1024;

                kprintf(" %dKB/%dB %d-way instruction cache", size, linesize, ways);
                if (reg & CPUV7_CT_CTYPE_WT)
                    kprintf(" WT");
                if (reg & CPUV7_CT_CTYPE_WB)
                    kprintf(" WB");
                if (reg & CPUV7_CT_CTYPE_RA)
                    kprintf(" Read-Alloc");
                if (reg & CPUV7_CT_CTYPE_WA)
                    kprintf(" Write-Alloc");
                kprintf("\n");
            }
            i++;
        }
    } else {
        /*
         * Print cache info. 
         */
        if (arm_picache_line_size == 0 && arm_pdcache_line_size == 0)
            return;

        if (arm_pcache_unified) {
            kprintf("  %dKB/%dB %d-way %s unified cache\n",
                   arm_pdcache_size / 1024,
                   arm_pdcache_line_size, arm_pdcache_ways, wtnames[arm_pcache_type]);
        } else {
            kprintf("  %dKB/%dB %d-way instruction cache\n",
                   arm_picache_size / 1024, arm_picache_line_size, arm_picache_ways);
            kprintf("  %dKB/%dB %d-way %s data cache\n",
                   arm_pdcache_size / 1024,
                   arm_pdcache_line_size, arm_pdcache_ways, wtnames[arm_pcache_type]);
        }
    }
}

/**
 * cpu_bootstrap
 *
 * Initialize core processor data for CPU #0 during initialization.
 */
void cpu_bootstrap(void)
{
    cpu_data_ptr[0] = &cpu_data_master;

    cpu_data_master.cpu_this = &cpu_data_master;
    cpu_data_master.cpu_processor = &BootProcessor;
}

/**
 * cpu_init
 *
 * Initialize more core processor data for CPU #0 during initialization.
 */
void cpu_init(void)
{
    cpu_data_t *cdp = current_cpu_datap();

    timer_call_initialize_queue(&cdp->rt_timer.queue);
    cdp->rt_timer.deadline = EndOfAllTime;

    cdp->cpu_type = CPU_TYPE_ARM;
#if defined(_ARM_ARCH_7)
    cdp->cpu_subtype = CPU_SUBTYPE_ARM_V7;
#elif defined(_ARM_ARCH_V6)
    cdp->cpu_subtype = CPU_SUBTYPE_ARM_V6;
#else
    cdp->cpu_subtype = CPU_SUBTYPE_ARM_ALL;
#endif
}

/**
 * get_cpu_number
 * 
 * Return the current processor number from the PCB. (Right now since we have
 * no SMP support, we have only 1 processor.
 *
 * When SMP becomes of real importance, we can get the current MPCore number
 * by reading some registers in the CP15 coprocessor.
 */
int get_cpu_number(void)
{
    return 0;
}

/**
 * current_cpu_datap
 * 
 * Return the current processor PCB.
 */
cpu_data_t *current_cpu_datap(void)
{
    int smp_number = get_cpu_number();
    cpu_data_t *current_cpu_data;

    if (smp_number == 0)
        return &cpu_data_master;

    current_cpu_data = cpu_datap(smp_number);
    if (!current_cpu_data) {
        panic("cpu_data for slot %d is not available yet\n", smp_number);
    }

    return current_cpu_data;
}

/**
 * cpu_processor_alloc
 *
 * Allocate a processor_t data structure for the specified processor.
 * The boot processor will always use internal data, since vm won't be
 * initialized to allocate data.
 */
processor_t cpu_processor_alloc(boolean_t is_boot_cpu)
{
    int ret;
    processor_t proc;

    if (is_boot_cpu) {
        return &BootProcessor;
    }

    /*
     * Allocate a new processor. 
     */
    ret = kmem_alloc(kernel_map, (vm_offset_t *) & proc, sizeof(*proc));
    if (ret != KERN_SUCCESS)
        return NULL;

    bzero((void *) proc, sizeof(*proc));
    return proc;
}

/**
 * current_processor
 *
 * Return the processor_t to the for the current processor.
 */
processor_t current_processor(void)
{
    return current_cpu_datap()->cpu_processor;
}

/**
 * cpu_to_processor
 *
 * Return the procesor_t for a specified processor. Please don't
 * do bad things. 
 *
 * This function needs validation to ensure that the cpu data accessed
 * isn't null/exceeding buffer boundaries. 
 */
processor_t cpu_to_processor(int cpu)
{
    assert(cpu_datap(cpu) != NULL);
    return cpu_datap(cpu)->cpu_processor;
}

/*
 * For sysctl().
 */
cpu_type_t cpu_type(void)
{
    return current_cpu_datap()->cpu_type;
}

/**
 * slot_type
 *
 * Return the current cpu type for a specified processor.
 */
cpu_type_t slot_type(int slot_num)
{
    return (cpu_datap(slot_num)->cpu_type);
}

/**
 * slot_subtype
 *
 * Return the current cpu subtype for a specified processor.
 */
cpu_subtype_t slot_subtype(int slot_num)
{
    return (cpu_datap(slot_num)->cpu_subtype);
}

/**
 * slot_threadtype
 *
 * Return the current SMT type for a specified processor.
 */
cpu_threadtype_t slot_threadtype(int slot_num)
{
    return CPU_THREADTYPE_NONE;
}

/**
 * cpu_subtype
 *
 * Return the current cpu type for the current processor.
 */
cpu_subtype_t cpu_subtype(void)
{
    return current_cpu_datap()->cpu_subtype;
}

/**
 * cpu_threadtype
 *
 * Return the current SMT type for the current processor.
 */
cpu_threadtype_t cpu_threadtype(void)
{
    return CPU_THREADTYPE_NONE;
}

/**
 * ast_pending
 *
 * Returns the current pending Asynchronous System Trap.
 */
ast_t *ast_pending(void)
{
    return (&current_cpu_datap()->cpu_pending_ast);
}

 /*ARGSUSED*/
kern_return_t cpu_control(int slot_num, processor_info_t info, unsigned int count)
{
    kprintf("cpu_control(%d,%p,%d) not implemented\n", slot_num, info, count);
    return (KERN_FAILURE);
}

kern_return_t cpu_info(processor_flavor_t flavor, int slot_num, processor_info_t info,
                       unsigned int *count)
{
    kprintf("cpu_info(%d,%d,%p,%p) not implemented\n", flavor, slot_num, info, count);
    return (KERN_FAILURE);
}

void cpu_machine_init(void)
{
    cpu_data_t *cdp = current_cpu_datap();

    PE_cpu_machine_init(cdp->cpu_id, FALSE);
    cdp->cpu_running = TRUE;
    ml_init_interrupt();
}

kern_return_t cpu_start(int cpu)
{
    kern_return_t ret;

    if (cpu == cpu_number()) {
        cpu_machine_init();
        return KERN_SUCCESS;
    }

    return KERN_SUCCESS;
}

kern_return_t cpu_info_count(processor_flavor_t flavor, unsigned int *count)
{
    *count = 0;
    return KERN_FAILURE;
}
