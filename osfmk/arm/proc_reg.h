/*	$NetBSD: armreg.h,v 1.37 2007/01/06 00:50:54 christos Exp $	*/

/*-
 * Copyright (c) 1998, 2001 Ben Harris
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
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
 * $FreeBSD$
 */

#ifndef MACHINE_ARMREG_H
#define MACHINE_ARMREG_H

#define INSN_SIZE	4
#define INSN_COND_MASK	0xf0000000	/* Condition mask */
#define PSR_MODE        0x0000001f      /* mode mask */
#define PSR_USR26_MODE  0x00000000
#define PSR_FIQ26_MODE  0x00000001
#define PSR_IRQ26_MODE  0x00000002
#define PSR_SVC26_MODE  0x00000003
#define PSR_USR32_MODE  0x00000010
#define PSR_FIQ32_MODE  0x00000011
#define PSR_IRQ32_MODE  0x00000012
#define PSR_SVC32_MODE  0x00000013
#define PSR_ABT32_MODE  0x00000017
#define PSR_UND32_MODE  0x0000001b
#define PSR_SYS32_MODE  0x0000001f
#define PSR_32_MODE     0x00000010
#define PSR_FLAGS	0xf0000000    /* flags */

#define PSR_C_bit (1 << 29)       /* carry */

/* The high-order byte is always the implementor */
#define CPU_ID_IMPLEMENTOR_MASK	0xff000000
#define CPU_ID_ARM_LTD		0x41000000 /* 'A' */
#define CPU_ID_DEC		0x44000000 /* 'D' */
#define CPU_ID_INTEL		0x69000000 /* 'i' */
#define	CPU_ID_TI		0x54000000 /* 'T' */
#define	CPU_ID_FARADAY		0x66000000 /* 'f' */

/* How to decide what format the CPUID is in. */
#define CPU_ID_ISOLD(x)		(((x) & 0x0000f000) == 0x00000000)
#define CPU_ID_IS7(x)		(((x) & 0x0000f000) == 0x00007000)
#define CPU_ID_ISNEW(x)		(!CPU_ID_ISOLD(x) && !CPU_ID_IS7(x))

/* On ARM3 and ARM6, this byte holds the foundry ID. */
#define CPU_ID_FOUNDRY_MASK	0x00ff0000
#define CPU_ID_FOUNDRY_VLSI	0x00560000

/* On ARM7 it holds the architecture and variant (sub-model) */
#define CPU_ID_7ARCH_MASK	0x00800000
#define CPU_ID_7ARCH_V3		0x00000000
#define CPU_ID_7ARCH_V4T	0x00800000
#define CPU_ID_7VARIANT_MASK	0x007f0000

/* On more recent ARMs, it does the same, but in a different format */
#define CPU_ID_ARCH_MASK	0x000f0000
#define CPU_ID_ARCH_V3		0x00000000
#define CPU_ID_ARCH_V4		0x00010000
#define CPU_ID_ARCH_V4T		0x00020000
#define CPU_ID_ARCH_V5		0x00030000
#define CPU_ID_ARCH_V5T		0x00040000
#define CPU_ID_ARCH_V5TE	0x00050000
#define CPU_ID_ARCH_V5TEJ	0x00060000
#define CPU_ID_ARCH_V6		0x00070000
#define CPU_ID_CPUID_SCHEME	0x000f0000
#define CPU_ID_VARIANT_MASK	0x00f00000

/* Next three nybbles are part number */
#define CPU_ID_PARTNO_MASK	0x0000fff0

/* Intel XScale has sub fields in part number */
#define CPU_ID_XSCALE_COREGEN_MASK	0x0000e000 /* core generation */
#define CPU_ID_XSCALE_COREREV_MASK	0x00001c00 /* core revision */
#define CPU_ID_XSCALE_PRODUCT_MASK	0x000003f0 /* product number */

/* And finally, the revision number. */
#define CPU_ID_REVISION_MASK	0x0000000f

/* Individual CPUs are probably best IDed by everything but the revision. */
#define CPU_ID_CPU_MASK		0xfffffff0

/* Fake CPU IDs for ARMs without CP15 */
#define CPU_ID_ARM2		0x41560200
#define CPU_ID_ARM250		0x41560250

/* Pre-ARM7 CPUs -- [15:12] == 0 */
#define CPU_ID_ARM3		0x41560300
#define CPU_ID_ARM600		0x41560600
#define CPU_ID_ARM610		0x41560610
#define CPU_ID_ARM620		0x41560620

/* ARM7 CPUs -- [15:12] == 7 */
#define CPU_ID_ARM700		0x41007000 /* XXX This is a guess. */
#define CPU_ID_ARM710		0x41007100
#define CPU_ID_ARM7500		0x41027100
#define CPU_ID_ARM710A		0x41047100 /* inc ARM7100 */
#define CPU_ID_ARM7500FE	0x41077100
#define CPU_ID_ARM710T		0x41807100
#define CPU_ID_ARM720T		0x41807200
#define CPU_ID_ARM740T8K	0x41807400 /* XXX no MMU, 8KB cache */
#define CPU_ID_ARM740T4K	0x41817400 /* XXX no MMU, 4KB cache */

/* Post-ARM7 CPUs */
#define CPU_ID_ARM810		0x41018100
#define CPU_ID_ARM920T		0x41129200
#define CPU_ID_ARM920T_ALT	0x41009200
#define CPU_ID_ARM922T		0x41029220
#define CPU_ID_ARM926EJS	0x41069260
#define CPU_ID_ARM940T		0x41029400 /* XXX no MMU */
#define CPU_ID_ARM946ES		0x41049460 /* XXX no MMU */
#define	CPU_ID_ARM966ES		0x41049660 /* XXX no MMU */
#define	CPU_ID_ARM966ESR1	0x41059660 /* XXX no MMU */
#define CPU_ID_ARM1020E		0x4115a200 /* (AKA arm10 rev 1) */
#define CPU_ID_ARM1022ES	0x4105a220
#define CPU_ID_ARM1026EJS	0x4106a260
#define CPU_ID_ARM1136JS	0x4107b360
#define CPU_ID_ARM1136JSR1	0x4117b360
#define CPU_ID_ARM1176JZS	0x410fb760
#define CPU_ID_CORTEXA5 	0x410fc050
#define CPU_ID_CORTEXA7 	0x410fc070
#define CPU_ID_CORTEXA8R0	0x410fc080
#define CPU_ID_CORTEXA8R1	0x411fc080
#define CPU_ID_CORTEXA8R2	0x412fc080
#define CPU_ID_CORTEXA8R3	0x413fc080
#define CPU_ID_CORTEXA9R1	0x411fc090
#define CPU_ID_CORTEXA9R2	0x412fc090
#define CPU_ID_CORTEXA9R3	0x413fc090
#define CPU_ID_CORTEXA15	0x410fc0f0
#define CPU_ID_SA110		0x4401a100
#define CPU_ID_SA1100		0x4401a110
#define	CPU_ID_TI925T		0x54029250
#define CPU_ID_MV88FR131	0x56251310 /* Marvell Feroceon 88FR131 Core */
#define CPU_ID_MV88FR331	0x56153310 /* Marvell Feroceon 88FR331 Core */
#define CPU_ID_MV88FR571_VD	0x56155710 /* Marvell Feroceon 88FR571-VD Core (ID from datasheet) */

/*
 * LokiPlus core has also ID set to 0x41159260 and this define cause execution of unsupported
 * L2-cache instructions so need to disable it. 0x41159260 is a generic ARM926E-S ID.
 */
#ifdef SOC_MV_LOKIPLUS
#define CPU_ID_MV88FR571_41	0x00000000
#else
#define CPU_ID_MV88FR571_41	0x41159260 /* Marvell Feroceon 88FR571-VD Core (actual ID from CPU reg) */
#endif

#define CPU_ID_MV88SV581X_V7		0x561F5810 /* Marvell Sheeva 88SV581x v7 Core */
#define CPU_ID_MV88SV584X_V7		0x562F5840 /* Marvell Sheeva 88SV584x v7 Core */
/* Marvell's CPUIDs with ARM ID in implementor field */
#define CPU_ID_ARM_88SV581X_V7		0x413FC080 /* Marvell Sheeva 88SV581x v7 Core */

#define	CPU_ID_FA526		0x66015260
#define	CPU_ID_FA626TE		0x66056260
#define CPU_ID_SA1110		0x6901b110
#define CPU_ID_IXP1200		0x6901c120
#define CPU_ID_80200		0x69052000
#define CPU_ID_PXA250    	0x69052100 /* sans core revision */
#define CPU_ID_PXA210    	0x69052120
#define CPU_ID_PXA250A		0x69052100 /* 1st version Core */
#define CPU_ID_PXA210A		0x69052120 /* 1st version Core */
#define CPU_ID_PXA250B		0x69052900 /* 3rd version Core */
#define CPU_ID_PXA210B		0x69052920 /* 3rd version Core */
#define CPU_ID_PXA250C		0x69052d00 /* 4th version Core */
#define CPU_ID_PXA210C		0x69052d20 /* 4th version Core */
#define	CPU_ID_PXA27X		0x69054110
#define	CPU_ID_80321_400	0x69052420
#define	CPU_ID_80321_600	0x69052430
#define	CPU_ID_80321_400_B0	0x69052c20
#define	CPU_ID_80321_600_B0	0x69052c30
#define	CPU_ID_80219_400	0x69052e20 /* A0 stepping/revision. */
#define	CPU_ID_80219_600	0x69052e30 /* A0 stepping/revision. */
#define	CPU_ID_81342		0x69056810
#define	CPU_ID_IXP425		0x690541c0
#define	CPU_ID_IXP425_533	0x690541c0
#define	CPU_ID_IXP425_400	0x690541d0
#define	CPU_ID_IXP425_266	0x690541f0
#define	CPU_ID_IXP435		0x69054040
#define	CPU_ID_IXP465		0x69054200

/* ARM3-specific coprocessor 15 registers */
#define ARM3_CP15_FLUSH		1
#define ARM3_CP15_CONTROL	2
#define ARM3_CP15_CACHEABLE	3
#define ARM3_CP15_UPDATEABLE	4
#define ARM3_CP15_DISRUPTIVE	5	

/* ARM3 Control register bits */
#define ARM3_CTL_CACHE_ON	0x00000001
#define ARM3_CTL_SHARED		0x00000002
#define ARM3_CTL_MONITOR	0x00000004

/* CPUID registers */
#define ARM_PFR0_ARM_ISA_MASK	0x0000000f

#define ARM_PFR0_THUMB_MASK	0x000000f0
#define ARM_PFR0_THUMB		0x10
#define ARM_PFR0_THUMB2		0x30

#define ARM_PFR0_JAZELLE_MASK	0x00000f00
#define ARM_PFR0_THUMBEE_MASK	0x0000f000

#define ARM_PFR1_ARMV4_MASK	0x0000000f
#define ARM_PFR1_SEC_EXT_MASK	0x000000f0
#define ARM_PFR1_MICROCTRL_MASK	0x00000f00

/*
 * Post-ARM3 CP15 registers:
 *
 *	1	Control register
 *
 *	2	Translation Table Base
 *
 *	3	Domain Access Control
 *
 *	4	Reserved
 *
 *	5	Fault Status
 *
 *	6	Fault Address
 *
 *	7	Cache/write-buffer Control
 *
 *	8	TLB Control
 *
 *	9	Cache Lockdown
 *
 *	10	TLB Lockdown
 *
 *	11	Reserved
 *
 *	12	Reserved
 *
 *	13	Process ID (for FCSE)
 *
 *	14	Reserved
 *
 *	15	Implementation Dependent
 */

/* Some of the definitions below need cleaning up for V3/V4 architectures */

/* CPU control register (CP15 register 1) */
#define CPU_CONTROL_MMU_ENABLE	0x00000001 /* M: MMU/Protection unit enable */
#define CPU_CONTROL_AFLT_ENABLE	0x00000002 /* A: Alignment fault enable */
#define CPU_CONTROL_DC_ENABLE	0x00000004 /* C: IDC/DC enable */
#define CPU_CONTROL_WBUF_ENABLE 0x00000008 /* W: Write buffer enable */
#define CPU_CONTROL_32BP_ENABLE 0x00000010 /* P: 32-bit exception handlers */
#define CPU_CONTROL_32BD_ENABLE 0x00000020 /* D: 32-bit addressing */
#define CPU_CONTROL_LABT_ENABLE 0x00000040 /* L: Late abort enable */
#define CPU_CONTROL_BEND_ENABLE 0x00000080 /* B: Big-endian mode */
#define CPU_CONTROL_SYST_ENABLE 0x00000100 /* S: System protection bit */
#define CPU_CONTROL_ROM_ENABLE	0x00000200 /* R: ROM protection bit */
#define CPU_CONTROL_CPCLK	0x00000400 /* F: Implementation defined */
#define CPU_CONTROL_BPRD_ENABLE 0x00000800 /* Z: Branch prediction enable */
#define CPU_CONTROL_IC_ENABLE   0x00001000 /* I: IC enable */
#define CPU_CONTROL_VECRELOC	0x00002000 /* V: Vector relocation */
#define CPU_CONTROL_ROUNDROBIN	0x00004000 /* RR: Predictable replacement */
#define CPU_CONTROL_V4COMPAT	0x00008000 /* L4: ARMv4 compat LDR R15 etc */
#define CPU_CONTROL_FI_ENABLE	0x00200000 /* FI: Low interrupt latency */
#define CPU_CONTROL_UNAL_ENABLE 0x00400000 /* U: unaligned data access */
#define CPU_CONTROL_V6_EXTPAGE	0x00800000 /* XP: ARMv6 extended page tables */
#define CPU_CONTROL_L2_ENABLE	0x04000000 /* L2 Cache enabled */
#define CPU_CONTROL_AF_ENABLE	0x20000000 /* Access Flag enable */

#define CPU_CONTROL_IDC_ENABLE	CPU_CONTROL_DC_ENABLE

/* ARM11x6 Auxiliary Control Register (CP15 register 1, opcode2 1) */
#define	ARM11X6_AUXCTL_RS	0x00000001 /* return stack */
#define	ARM11X6_AUXCTL_DB	0x00000002 /* dynamic branch prediction */
#define	ARM11X6_AUXCTL_SB	0x00000004 /* static branch prediction */
#define	ARM11X6_AUXCTL_TR	0x00000008 /* MicroTLB replacement strat. */
#define	ARM11X6_AUXCTL_EX	0x00000010 /* exclusive L1/L2 cache */
#define	ARM11X6_AUXCTL_RA	0x00000020 /* clean entire cache disable */
#define	ARM11X6_AUXCTL_RV	0x00000040 /* block transfer cache disable */
#define	ARM11X6_AUXCTL_CZ	0x00000080 /* restrict cache size */

/* ARM1136 Auxiliary Control Register (CP15 register 1, opcode2 1) */
#define ARM1136_AUXCTL_PFI	0x80000000 /* PFI: partial FI mode. */
					   /* This is an undocumented flag
					    * used to work around a cache bug
					    * in r0 steppings. See errata
					    * 364296.
					    */
/* ARM1176 Auxiliary Control Register (CP15 register 1, opcode2 1) */   
#define	ARM1176_AUXCTL_PHD	0x10000000 /* inst. prefetch halting disable */
#define	ARM1176_AUXCTL_BFD	0x20000000 /* branch folding disable */
#define	ARM1176_AUXCTL_FSD	0x40000000 /* force speculative ops disable */
#define	ARM1176_AUXCTL_FIO	0x80000000 /* low intr latency override */

/* XScale Auxillary Control Register (CP15 register 1, opcode2 1) */
#define	XSCALE_AUXCTL_K		0x00000001 /* dis. write buffer coalescing */
#define	XSCALE_AUXCTL_P		0x00000002 /* ECC protect page table access */
/* Note: XSCale core 3 uses those for LLR DCcahce attributes */
#define	XSCALE_AUXCTL_MD_WB_RA	0x00000000 /* mini-D$ wb, read-allocate */
#define	XSCALE_AUXCTL_MD_WB_RWA	0x00000010 /* mini-D$ wb, read/write-allocate */
#define	XSCALE_AUXCTL_MD_WT	0x00000020 /* mini-D$ wt, read-allocate */
#define	XSCALE_AUXCTL_MD_MASK	0x00000030

/* Xscale Core 3 only */
#define XSCALE_AUXCTL_LLR	0x00000400 /* Enable L2 for LLR Cache */

/* Marvell Extra Features Register (CP15 register 1, opcode2 0) */
#define MV_DC_REPLACE_LOCK	0x80000000 /* Replace DCache Lock */
#define MV_DC_STREAM_ENABLE	0x20000000 /* DCache Streaming Switch */
#define MV_WA_ENABLE		0x10000000 /* Enable Write Allocate */
#define MV_L2_PREFETCH_DISABLE	0x01000000 /* L2 Cache Prefetch Disable */
#define MV_L2_INV_EVICT_ERR	0x00800000 /* L2 Invalidates Uncorrectable Error Line Eviction */
#define MV_L2_ENABLE		0x00400000 /* L2 Cache enable */
#define MV_IC_REPLACE_LOCK	0x00080000 /* Replace ICache Lock */
#define MV_BGH_ENABLE		0x00040000 /* Branch Global History Register Enable */
#define MV_BTB_DISABLE		0x00020000 /* Branch Target Buffer Disable */
#define MV_L1_PARERR_ENABLE	0x00010000 /* L1 Parity Error Enable */

/* Cache type register definitions */
#define	CPU_CT_ISIZE(x)		((x) & 0xfff)		/* I$ info */
#define	CPU_CT_DSIZE(x)		(((x) >> 12) & 0xfff)	/* D$ info */
#define	CPU_CT_S		(1U << 24)		/* split cache */
#define	CPU_CT_CTYPE(x)		(((x) >> 25) & 0xf)	/* cache type */
#define	CPU_CT_FORMAT(x)	((x) >> 29)

#define	CPU_CT_CTYPE_WT		0	/* write-through */
#define	CPU_CT_CTYPE_WB1	1	/* write-back, clean w/ read */
#define	CPU_CT_CTYPE_WB2	2	/* w/b, clean w/ cp15,7 */
#define	CPU_CT_CTYPE_WB6	6	/* w/b, cp15,7, lockdown fmt A */
#define	CPU_CT_CTYPE_WB7	7	/* w/b, cp15,7, lockdown fmt B */

#define	CPU_CT_xSIZE_LEN(x)	((x) & 0x3)		/* line size */
#define	CPU_CT_xSIZE_M		(1U << 2)		/* multiplier */
#define	CPU_CT_xSIZE_ASSOC(x)	(((x) >> 3) & 0x7)	/* associativity */
#define	CPU_CT_xSIZE_SIZE(x)	(((x) >> 6) & 0x7)	/* size */

#define	CPU_CT_ARMV7		0x4
/* ARM v7 Cache type definitions */
#define	CPUV7_CT_CTYPE_WT	(1 << 31)
#define	CPUV7_CT_CTYPE_WB	(1 << 30)
#define	CPUV7_CT_CTYPE_RA	(1 << 29)
#define	CPUV7_CT_CTYPE_WA	(1 << 28)

#define	CPUV7_CT_xSIZE_LEN(x)	((x) & 0x7)		/* line size */
#define	CPUV7_CT_xSIZE_ASSOC(x)	(((x) >> 3) & 0x3ff)	/* associativity */
#define	CPUV7_CT_xSIZE_SET(x)	(((x) >> 13) & 0x7fff)	/* num sets */

#define	CPU_CLIDR_CTYPE(reg,x)	(((reg) >> ((x) * 3)) & 0x7)
#define	CPU_CLIDR_LOUIS(reg)	(((reg) >> 21) & 0x7)
#define	CPU_CLIDR_LOC(reg)	(((reg) >> 24) & 0x7)
#define	CPU_CLIDR_LOUU(reg)	(((reg) >> 27) & 0x7)

#define	CACHE_ICACHE		1
#define	CACHE_DCACHE		2
#define	CACHE_SEP_CACHE		3
#define	CACHE_UNI_CACHE		4

/* Fault status register definitions */

#define FAULT_TYPE_MASK 0x0f
#define FAULT_USER      0x10

#define FAULT_WRTBUF_0  0x00 /* Vector Exception */
#define FAULT_WRTBUF_1  0x02 /* Terminal Exception */
#define FAULT_BUSERR_0  0x04 /* External Abort on Linefetch -- Section */
#define FAULT_BUSERR_1  0x06 /* External Abort on Linefetch -- Page */
#define FAULT_BUSERR_2  0x08 /* External Abort on Non-linefetch -- Section */
#define FAULT_BUSERR_3  0x0a /* External Abort on Non-linefetch -- Page */
#define FAULT_BUSTRNL1  0x0c /* External abort on Translation -- Level 1 */
#define FAULT_BUSTRNL2  0x0e /* External abort on Translation -- Level 2 */
#define FAULT_ALIGN_0   0x01 /* Alignment */
#define FAULT_ALIGN_1   0x03 /* Alignment */
#define FAULT_TRANS_S   0x05 /* Translation -- Section */
#define FAULT_TRANS_F   0x06 /* Translation -- Flag */
#define FAULT_TRANS_P   0x07 /* Translation -- Page */
#define FAULT_DOMAIN_S  0x09 /* Domain -- Section */
#define FAULT_DOMAIN_P  0x0b /* Domain -- Page */
#define FAULT_PERM_S    0x0d /* Permission -- Section */
#define FAULT_PERM_P    0x0f /* Permission -- Page */

#define	FAULT_IMPRECISE	0x400	/* Imprecise exception (XSCALE) */

/*
 * Address of the vector page, low and high versions.
 */
#ifndef __ASSEMBLER__
#define	ARM_VECTORS_LOW		0x00000000U
#define	ARM_VECTORS_HIGH	0xffff0000U
#else
#define	ARM_VECTORS_LOW		0
#define	ARM_VECTORS_HIGH	0xffff0000
#endif

/*
 * ARM Instructions
 *
 *       3 3 2 2 2
 *       1 0 9 8 7                                                     0
 *      +-------+-------------------------------------------------------+
 *      | cond  |              instruction dependant                    |
 *      |c c c c|                                                       |
 *      +-------+-------------------------------------------------------+
 */

#define INSN_SIZE		4		/* Always 4 bytes */
#define INSN_COND_MASK		0xf0000000	/* Condition mask */
#define INSN_COND_AL		0xe0000000	/* Always condition */

#define THUMB_INSN_SIZE		2		/* Some are 4 bytes.  */

#if !defined(__ASSEMBLER__) && !defined(_RUMPKERNEL)
#define	ARMREG_READ_INLINE(name, __insnstring)			\
static inline uint32_t armreg_##name##_read(void)		\
{								\
	uint32_t __rv;						\
	__asm __volatile("mrc " __insnstring : "=r"(__rv));	\
	return __rv;						\
}

#define	ARMREG_WRITE_INLINE(name, __insnstring)			\
static inline void armreg_##name##_write(uint32_t __val)	\
{								\
	__asm __volatile("mcr " __insnstring :: "r"(__val));	\
}

#define	ARMREG_READ64_INLINE(name, __insnstring)		\
static inline uint64_t armreg_##name##_read(void)		\
{								\
	uint64_t __rv;						\
	__asm __volatile("mrrc " __insnstring : "=r"(__rv));	\
	return __rv;						\
}

#define	ARMREG_WRITE64_INLINE(name, __insnstring)		\
static inline void armreg_##name##_write(uint64_t __val)	\
{								\
	__asm __volatile("mcrr " __insnstring :: "r"(__val));	\
}

/* cp10 registers */
ARMREG_READ_INLINE(fpsid, "p10,7,%0,c0,c0,0") /* VFP System ID */
ARMREG_READ_INLINE(fpscr, "p10,7,%0,c1,c0,0") /* VFP Status/Control Register */
ARMREG_WRITE_INLINE(fpscr, "p10,7,%0,c1,c0,0") /* VFP Status/Control Register */
ARMREG_READ_INLINE(mvfr1, "p10,7,%0,c6,c0,0") /* Media and VFP Feature Register 1 */
ARMREG_READ_INLINE(mvfr0, "p10,7,%0,c7,c0,0") /* Media and VFP Feature Register 0 */
ARMREG_READ_INLINE(fpexc, "p10,7,%0,c8,c0,0") /* VFP Exception Register */
ARMREG_WRITE_INLINE(fpexc, "p10,7,%0,c8,c0,0") /* VFP Exception Register */
ARMREG_READ_INLINE(fpinst, "p10,7,%0,c9,c0,0") /* VFP Exception Instruction */
ARMREG_WRITE_INLINE(fpinst, "p10,7,%0,c9,c0,0") /* VFP Exception Instruction */
ARMREG_READ_INLINE(fpinst2, "p10,7,%0,c10,c0,0") /* VFP Exception Instruction 2 */
ARMREG_WRITE_INLINE(fpinst2, "p10,7,%0,c10,c0,0") /* VFP Exception Instruction 2 */

/* cp15 c0 registers */
ARMREG_READ_INLINE(midr, "p15,0,%0,c0,c0,0") /* Main ID Register */
ARMREG_READ_INLINE(ctr, "p15,0,%0,c0,c0,1") /* Cache Type Register */
ARMREG_READ_INLINE(mpidr, "p15,0,%0,c0,c0,5") /* Multiprocess Affinity Register */
ARMREG_READ_INLINE(pfr0, "p15,0,%0,c0,c1,0") /* Processor Feature Register 0 */
ARMREG_READ_INLINE(pfr1, "p15,0,%0,c0,c1,1") /* Processor Feature Register 1 */
ARMREG_READ_INLINE(mmfr0, "p15,0,%0,c0,c1,4") /* Memory Model Feature Register 0 */
ARMREG_READ_INLINE(mmfr1, "p15,0,%0,c0,c1,5") /* Memory Model Feature Register 1 */
ARMREG_READ_INLINE(mmfr2, "p15,0,%0,c0,c1,6") /* Memory Model Feature Register 2 */
ARMREG_READ_INLINE(mmfr3, "p15,0,%0,c0,c1,7") /* Memory Model Feature Register 3 */
ARMREG_READ_INLINE(isar0, "p15,0,%0,c0,c2,0") /* Instruction Set Attribute Register 0 */
ARMREG_READ_INLINE(isar1, "p15,0,%0,c0,c2,1") /* Instruction Set Attribute Register 1 */
ARMREG_READ_INLINE(isar2, "p15,0,%0,c0,c2,2") /* Instruction Set Attribute Register 2 */
ARMREG_READ_INLINE(isar3, "p15,0,%0,c0,c2,3") /* Instruction Set Attribute Register 3 */
ARMREG_READ_INLINE(isar4, "p15,0,%0,c0,c2,4") /* Instruction Set Attribute Register 4 */
ARMREG_READ_INLINE(isar5, "p15,0,%0,c0,c2,5") /* Instruction Set Attribute Register 5 */
ARMREG_READ_INLINE(ccsidr, "p15,1,%0,c0,c0,0") /* Cache Size ID Register */
ARMREG_READ_INLINE(clidr, "p15,1,%0,c0,c0,1") /* Cache Level ID Register */
ARMREG_READ_INLINE(csselr, "p15,2,%0,c0,c0,0") /* Cache Size Selection Register */
ARMREG_WRITE_INLINE(csselr, "p15,2,%0,c0,c0,0") /* Cache Size Selection Register */
/* cp15 c1 registers */
ARMREG_READ_INLINE(sctrl, "p15,0,%0,c1,c0,0") /* System Control Register */
ARMREG_WRITE_INLINE(sctrl, "p15,0,%0,c1,c0,0") /* System Control Register */
ARMREG_READ_INLINE(auxctl, "p15,0,%0,c1,c0,1") /* Auxiliary Control Register */
ARMREG_WRITE_INLINE(auxctl, "p15,0,%0,c1,c0,1") /* Auxiliary Control Register */
ARMREG_READ_INLINE(cpacr, "p15,0,%0,c1,c0,2") /* Co-Processor Access Control Register */
ARMREG_WRITE_INLINE(cpacr, "p15,0,%0,c1,c0,2") /* Co-Processor Access Control Register */
/* cp15 c2 registers */
ARMREG_READ_INLINE(ttbr, "p15,0,%0,c2,c0,0") /* Translation Table Base Register 0 */
ARMREG_WRITE_INLINE(ttbr, "p15,0,%0,c2,c0,0") /* Translation Table Base Register 0 */
ARMREG_READ_INLINE(ttbr1, "p15,0,%0,c2,c0,1") /* Translation Table Base Register 1 */
ARMREG_WRITE_INLINE(ttbr1, "p15,0,%0,c2,c0,1") /* Translation Table Base Register 1 */
ARMREG_READ_INLINE(ttbcr, "p15,0,%0,c2,c0,2") /* Translation Table Base Register */
ARMREG_WRITE_INLINE(ttbcr, "p15,0,%0,c2,c0,2") /* Translation Table Base Register */
/* cp15 c5 registers */
ARMREG_READ_INLINE(dfsr, "p15,0,%0,c5,c0,0") /* Data Fault Status Register */
ARMREG_READ_INLINE(ifsr, "p15,0,%0,c5,c0,1") /* Instruction Fault Status Register */
/* cp15 c6 registers */
ARMREG_READ_INLINE(dfar, "p15,0,%0,c6,c0,0") /* Data Fault Address Register */
ARMREG_READ_INLINE(ifar, "p15,0,%0,c6,c0,2") /* Instruction Fault Address Register */
/* cp15 c7 registers */
ARMREG_WRITE_INLINE(icialluis, "p15,0,%0,c7,c1,0") /* Instruction Inv All (IS) */
ARMREG_WRITE_INLINE(bpiallis, "p15,0,%0,c7,c1,6") /* Branch Invalidate All (IS) */
ARMREG_READ_INLINE(par, "p15,0,%0,c7,c4,0") /* Physical Address Register */
ARMREG_WRITE_INLINE(iciallu, "p15,0,%0,c7,c5,0") /* Instruction Invalidate All */
ARMREG_WRITE_INLINE(icimvau, "p15,0,%0,c7,c5,1") /* Instruction Invalidate MVA */
ARMREG_WRITE_INLINE(isb, "p15,0,%0,c7,c5,4") /* Instruction Synchronization Barrier */
ARMREG_WRITE_INLINE(bpiall, "p15,0,%0,c5,c1,6") /* Breakpoint Invalidate All */
ARMREG_WRITE_INLINE(dcimvac, "p15,0,%0,c7,c6,1") /* Data Invalidate MVA to PoC */
ARMREG_WRITE_INLINE(dcisw, "p15,0,%0,c7,c6,2") /* Data Invalidate Set/Way */
ARMREG_WRITE_INLINE(ats1cpr, "p15,0,%0,c7,c8,0") /* AddrTrans CurState PL1 Read */
ARMREG_WRITE_INLINE(dccmvac, "p15,0,%0,c7,c10,1") /* Data Clean MVA to PoC */
ARMREG_WRITE_INLINE(dccsw, "p15,0,%0,c7,c10,2") /* Data Clean Set/Way */
ARMREG_WRITE_INLINE(dsb, "p15,0,%0,c7,c10,4") /* Data Synchronization Barrier */
ARMREG_WRITE_INLINE(dmb, "p15,0,%0,c7,c10,5") /* Data Memory Barrier */
ARMREG_WRITE_INLINE(dccmvau, "p15,0,%0,c7,c14,1") /* Data Clean MVA to PoU */
ARMREG_WRITE_INLINE(dccimvac, "p15,0,%0,c7,c14,1") /* Data Clean&Inv MVA to PoC */
ARMREG_WRITE_INLINE(dccisw, "p15,0,%0,c7,c14,2") /* Data Clean&Inv Set/Way */
/* cp15 c8 registers */
ARMREG_WRITE_INLINE(tlbiallis, "p15,0,%0,c8,c3,0") /* Invalidate entire unified TLB, inner shareable */
ARMREG_WRITE_INLINE(tlbimvais, "p15,0,%0,c8,c3,1") /* Invalidate unified TLB by MVA, inner shareable */
ARMREG_WRITE_INLINE(tlbiasidis, "p15,0,%0,c8,c3,2") /* Invalidate unified TLB by ASID, inner shareable */
ARMREG_WRITE_INLINE(tlbimvaais, "p15,0,%0,c8,c3,3") /* Invalidate unified TLB by MVA, all ASID, inner shareable */
ARMREG_WRITE_INLINE(itlbiall, "p15,0,%0,c8,c5,0") /* Invalidate entire instruction TLB */
ARMREG_WRITE_INLINE(itlbimva, "p15,0,%0,c8,c5,1") /* Invalidate instruction TLB by MVA */
ARMREG_WRITE_INLINE(itlbiasid, "p15,0,%0,c8,c5,2") /* Invalidate instruction TLB by ASID */
ARMREG_WRITE_INLINE(dtlbiall, "p15,0,%0,c8,c6,0") /* Invalidate entire data TLB */
ARMREG_WRITE_INLINE(dtlbimva, "p15,0,%0,c8,c6,1") /* Invalidate data TLB by MVA */
ARMREG_WRITE_INLINE(dtlbiasid, "p15,0,%0,c8,c6,2") /* Invalidate data TLB by ASID */
ARMREG_WRITE_INLINE(tlbiall, "p15,0,%0,c8,c7,0") /* Invalidate entire unified TLB */
ARMREG_WRITE_INLINE(tlbimva, "p15,0,%0,c8,c7,1") /* Invalidate unified TLB by MVA */
ARMREG_WRITE_INLINE(tlbiasid, "p15,0,%0,c8,c7,2") /* Invalidate unified TLB by ASID */
ARMREG_WRITE_INLINE(tlbimvaa, "p15,0,%0,c8,c7,3") /* Invalidate unified TLB by MVA, all ASID */
/* cp15 c9 registers */
ARMREG_READ_INLINE(pmcr, "p15,0,%0,c9,c12,0") /* PMC Control Register */
ARMREG_WRITE_INLINE(pmcr, "p15,0,%0,c9,c12,0") /* PMC Control Register */
ARMREG_READ_INLINE(pmcntenset, "p15,0,%0,c9,c12,1") /* PMC Count Enable Set */
ARMREG_WRITE_INLINE(pmcntenset, "p15,0,%0,c9,c12,1") /* PMC Count Enable Set */
ARMREG_READ_INLINE(pmcntenclr, "p15,0,%0,c9,c12,2") /* PMC Count Enable Clear */
ARMREG_WRITE_INLINE(pmcntenclr, "p15,0,%0,c9,c12,2") /* PMC Count Enable Clear */
ARMREG_READ_INLINE(pmovsr, "p15,0,%0,c9,c12,3") /* PMC Overflow Flag Status */
ARMREG_WRITE_INLINE(pmovsr, "p15,0,%0,c9,c12,3") /* PMC Overflow Flag Status */
ARMREG_READ_INLINE(pmccntr, "p15,0,%0,c9,c13,0") /* PMC Cycle Counter */
ARMREG_WRITE_INLINE(pmccntr, "p15,0,%0,c9,c13,0") /* PMC Cycle Counter */
ARMREG_READ_INLINE(pmuserenr, "p15,0,%0,c9,c14,0") /* PMC User Enable */
ARMREG_WRITE_INLINE(pmuserenr, "p15,0,%0,c9,c14,0") /* PMC User Enable */
/* cp15 c13 registers */
ARMREG_READ_INLINE(contextidr, "p15,0,%0,c13,c0,1") /* Context ID Register */
ARMREG_WRITE_INLINE(contextidr, "p15,0,%0,c13,c0,1") /* Context ID Register */
ARMREG_READ_INLINE(tpidrprw, "p15,0,%0,c13,c0,4") /* PL1 only Thread ID Register */
ARMREG_WRITE_INLINE(tpidrprw, "p15,0,%0,c13,c0,4") /* PL1 only Thread ID Register */
/* cp14 c12 registers */
ARMREG_READ_INLINE(vbar, "p15,0,%0,c12,c0,0")	/* Vector Base Address Register */
ARMREG_WRITE_INLINE(vbar, "p15,0,%0,c12,c0,0")	/* Vector Base Address Register */
/* cp15 c14 registers */
/* cp15 Global Timer Registers */
ARMREG_READ_INLINE(cnt_frq, "p15,0,%0,c14,c0,0") /* Counter Frequency Register */
ARMREG_WRITE_INLINE(cnt_frq, "p15,0,%0,c14,c0,0") /* Counter Frequency Register */
ARMREG_READ_INLINE(cntk_ctl, "p15,0,%0,c14,c1,0") /* Timer PL1 Control Register */
ARMREG_WRITE_INLINE(cntk_ctl, "p15,0,%0,c14,c1,0") /* Timer PL1 Control Register */
ARMREG_READ_INLINE(cntp_tval, "p15,0,%0,c14,c2,0") /* PL1 Physical TimerValue Register */
ARMREG_WRITE_INLINE(cntp_tval, "p15,0,%0,c14,c2,0") /* PL1 Physical TimerValue Register */
ARMREG_READ_INLINE(cntp_ctl, "p15,0,%0,c14,c2,1") /* PL1 Physical Timer Control Register */
ARMREG_WRITE_INLINE(cntp_ctl, "p15,0,%0,c14,c2,1") /* PL1 Physical Timer Control Register */
ARMREG_READ_INLINE(cntv_tval, "p15,0,%0,c14,c3,0") /* Virtual TimerValue Register */
ARMREG_WRITE_INLINE(cntv_tval, "p15,0,%0,c14,c3,0") /* Virtual TimerValue Register */
ARMREG_READ_INLINE(cntv_ctl, "p15,0,%0,c14,c3,1") /* Virtual Timer Control Register */
ARMREG_WRITE_INLINE(cntv_ctl, "p15,0,%0,c14,c3,1") /* Virtual Timer Control Register */
ARMREG_READ64_INLINE(cntp_ct, "p15,0,%Q0,%R0,c14") /* Physical Count Register */
ARMREG_WRITE64_INLINE(cntp_ct, "p15,0,%Q0,%R0,c14") /* Physical Count Register */
ARMREG_READ64_INLINE(cntv_ct, "p15,1,%Q0,%R0,c14") /* Virtual Count Register */
ARMREG_WRITE64_INLINE(cntv_ct, "p15,1,%Q0,%R0,c14") /* Virtual Count Register */
ARMREG_READ64_INLINE(cntp_cval, "p15,2,%Q0,%R0,c14") /* PL1 Physical Timer CompareValue Register */
ARMREG_WRITE64_INLINE(cntp_cval, "p15,2,%Q0,%R0,c14") /* PL1 Physical Timer CompareValue Register */
ARMREG_READ64_INLINE(cntv_cval, "p15,3,%Q0,%R0,c14") /* PL1 Virtual Timer CompareValue Register */
ARMREG_WRITE64_INLINE(cntv_cval, "p15,3,%Q0,%R0,c14") /* PL1 Virtual Timer CompareValue Register */
/* cp15 c15 registers */
ARMREG_READ_INLINE(cbar, "p15,4,%0,c15,c0,0")	/* Configuration Base Address Register */
ARMREG_READ_INLINE(pmcrv6, "p15,0,%0,c15,c12,0") /* PMC Control Register (armv6) */
ARMREG_WRITE_INLINE(pmcrv6, "p15,0,%0,c15,c12,0") /* PMC Control Register (armv6) */
ARMREG_READ_INLINE(pmccntrv6, "p15,0,%0,c15,c12,1") /* PMC Cycle Counter (armv6) */
ARMREG_WRITE_INLINE(pmccntrv6, "p15,0,%0,c15,c12,1") /* PMC Cycle Counter (armv6) */

#endif /* !__ASSEMBLER__ */

#endif /* !MACHINE_ARMREG_H */