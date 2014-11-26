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
 * CPU i-cache/d-cache
 */

#include <arm/arch.h>
#include <arm/asm_help.h>

#ifdef _ARM_ARCH_7
#undef EnterThumb
#undef EnterThumb_NoAlign
#define EnterThumb_NoAlign EnterARM_NoAlign
#define EnterThumb EnterARM
#endif

#ifndef  _ARM_ARCH_7
#define DCACHE_LINE_SZ 32
#endif

/**
 * invalidate_dcache
 *
 * Invalidate entire ARM data-cache.
 */
EnterThumb(invalidate_dcache)
    mov     r0, #0
    mcr     p15, 0, r0, c7, c6, 0
#ifdef _ARM_ARCH_7
    dsb     sy
#else
    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4
#endif
    bx      lr

/**
 * invalidate_dcache_region
 *
 * Invalidate an ARM data-cache region.
 */
EnterThumb(invalidate_dcache_region)
#ifdef _ARM_ARCH_7
    mrc     p15, 1, r3, c0, c0, 0
    and     r3, r3, #7
    mov     r4, #16
    mov     r4, r4, lsl r3
    mov     r3, #0
    sub     r3, r4, #1
    and     r2, r0, r3
    bic     r0, r0, r3
#else
    and     r2, r0, #(DCACHE_LINE_SZ - 1)
    bic     r0, r0, #(DCACHE_LINE_SZ - 1)
#endif
    add     r1, r1, r2
    sub     r1, r1, #1
    mov     r1, r1, lsr #5
1:
    mcr     p15, 0, r0, c7, c6, 1
#ifdef _ARM_ARCH_7
    add     r0, r0, r4
#else
    add     r0, r0, #DCACHE_LINE_SZ
#endif
    subs    r1, r1, #1
    bpl     1b
#ifdef _ARM_ARCH_7
    dsb     sy
#else
    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4
#endif
    bx      lr

/**
 * dcache_incoherent_io_store64
 *
 * Shim function to clean_dcache_region
 */
EnterThumb(dcache_incoherent_io_store64)
    mov     r1, r2
    LoadConstantToReg(_gPhysBase, r2)
    ldr     r2, [r2]
    sub     r0, r0, r2
    LoadConstantToReg(_gVirtBase, r2)
    ldr     r2, [r2]
    add     r0, r0, r2

/**
 * clean_dcache_region
 *
 * Clean an ARM data-cache region.
 */
EnterThumb_NoAlign(clean_dcache_region)
#ifdef _ARM_ARCH_7
    mrc     p15, 1, r3, c0, c0, 0
    and     r3, r3, #7
    mov     r4, #16
    mov     r4, r4, lsl r3
    mov     r3, #0
    sub     r3, r4, #1
    and     r2, r0, r3
    bic     r0, r0, r3
#else
    and     r2, r0, #(DCACHE_LINE_SZ - 1)
    bic     r0, r0, #(DCACHE_LINE_SZ - 1)
#endif
    add     r1, r1, r2
    sub     r1, r1, #1
    mov     r1, r1, lsr #5
1:
    mcr     p15, 0, r0, c7, c10, 1
#ifdef _ARM_ARCH_7
    add     r0, r0, r4
#else
    add     r0, r0, #DCACHE_LINE_SZ
#endif
    subs    r1, r1, #1
    bpl     1b
#ifdef _ARM_ARCH_7
    dsb     sy
#else
    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4
#endif
    bx      lr

/**
 * cleanflush_dcache_region
 *
 * Clean and flush entire ARM data-cache.
 */
EnterThumb(cleanflush_dcache)
    mov     r0, #0
1:
    mcr     p15, 0, r0, c7, c14, 2
#ifdef _ARM_ARCH_7
    mrc     p15, 1, r3, c0, c0, 0
    and     r3, r3, #7
    mov     r4, #16
    mov     r4, r4, lsl r3
    add     r0, r0, r4
#else
    add     r0, r0, #DCACHE_LINE_SZ
#endif
    tst     r0, #0x1000
    beq     1b
    bic     r0, r0, #0x1000
    adds    r0, r0, #0x40000000
    bcc     1b
#ifdef _ARM_ARCH_7
    dsb     sy
#else
    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4
#endif
    bx      lr

/**
 * dcache_incoherent_io_flush64
 *
 * Shim function to cleanflush_dcache_region
 */
EnterThumb(dcache_incoherent_io_flush64)
    mov     r1, r2
    LoadConstantToReg(_gPhysBase, r2)
    ldr     r2, [r2]
    sub     r0, r0, r2
    LoadConstantToReg(_gVirtBase, r2)
    ldr     r2, [r2]
    add     r0, r0, r2

/**
 * cleanflush_dcache_region
 *
 * Clean and flush an ARM data-cache region.
 */
EnterThumb_NoAlign(cleanflush_dcache_region)
#ifdef _ARM_ARCH_7
    mrc     p15, 1, r3, c0, c0, 0
    and     r3, r3, #7
    mov     r4, #16
    mov     r4, r4, lsl r3
    mov     r3, #0
    sub     r3, r4, #1
    and     r2, r0, r3
    bic     r0, r0, r3
#else
    and     r2, r0, #(DCACHE_LINE_SZ - 1)
    bic     r0, r0, #(DCACHE_LINE_SZ - 1)
#endif
    add     r1, r1, r2
    sub     r1, r1, #1
    mov     r1, r1, lsr #5
1:
    mcr     p15, 0, r0, c7, c14, 1
#ifdef _ARM_ARCH_7
    add     r0, r0, r4
#else
    add     r0, r0, #DCACHE_LINE_SZ
#endif
    subs    r1, r1, #1
    bpl     1b
#ifdef _ARM_ARCH_7
    dsb     sy
#else
    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4
#endif
    bx      lr

/**
 * flush_dcache64/flush_dcache
 *
 * Flush entire ARM data-cache.
 */
EnterThumb(flush_dcache64)
    mov     r1, r2
    mov     r2, r3

EnterThumb_NoAlign(flush_dcache)
    cmp     r2, #0
    beq     _clean_dcache_region
    LoadConstantToReg(_gPhysBase, r2)
    ldr     r2, [r2]
    sub     r0, r0, r2
    LoadConstantToReg(_gVirtBase, r2)
    ldr     r2, [r2]
    add     r0, r0, r2
    b       _clean_dcache_region

/**
 * invalidate_icache64/invalidate_icache
 *
 * Invalidate entire ARM i-cache, shim on top of invalidate_icache.
 */
EnterThumb(invalidate_icache64)
    mov     r1, r2
    mov     r2, r3

/*
 * The use of EnterThumb_NoAlign is due to the previous function
 * calling into this one.
 */
EnterThumb_NoAlign(invalidate_icache)
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 0
#ifdef _ARM_ARCH_7
    dsb     sy
#else
    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4
#endif
    bx      lr

/**
 * invalidate_icache_region
 *
 * Invalidate an ARM i-cache region.
 */
EnterThumb(invalidate_icache_region)
#ifdef _ARM_ARCH_7
    mrc     p15, 1, r3, c0, c0, 0
    and     r3, r3, #7
    mov     r4, #16
    mov     r4, r4, lsl r3
    mov     r3, #0
    sub     r3, r4, #1
    and     r2, r0, r3
    bic     r0, r0, r3
#else
    and     r2, r0, #(DCACHE_LINE_SZ - 1)
    bic     r0, r0, #(DCACHE_LINE_SZ - 1)
#endif
    add     r1, r1, r2
    sub     r1, r1, #1
    mov     r1, r1, lsr #5
1:
    mcr     p15, 0, r0, c7, c5, 1
#ifdef _ARM_ARCH_7
    add     r0, r0, r4
#else
    add     r0, r0, #DCACHE_LINE_SZ
#endif
    subs    r1, r1, #1
    bpl     1b
#ifdef _ARM_ARCH_7
    dsb     sy
#else
    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4
#endif
    bx      lr

/**
 * invalidate_branch_target_cache
 *
 * Invalidate the entire ARM branch target buffer (or cache).
 */
EnterThumb(invalidate_branch_target_cache)
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 6
#ifdef _ARM_ARCH_7
    isb     sy
    dsb     sy
#else
    mcr     p15, 0, r0, c7, c5, 4
    mcr     p15, 0, r0, c7, c10, 4
#endif
    bx      lr
/**
 * flush_mmu_tlb
 *
 * Flush all cached entries in the ARM translation look-aside buffer.
 * This should be called before switching translation table bases.
 */
EnterThumb(flush_mmu_tlb)
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0
    mcr     p15, 0, r0, c7, c5, 0
#ifdef _ARM_ARCH_7
    isb     sy
    dsb     sy
#else
    mcr     p15, 0, r0, c7, c5, 4
    mcr     p15, 0, r0, c7, c10, 4
#endif
    bx      lr

/**
 * cache_initialize
 *
 * Start and initialize ARM caches.
 */
EnterARM(cache_initialize)
#ifndef _ARM_ARCH_7
    mcr     p15, 0, r0, c7, c5, 4
    mcr     p15, 0, r0, c7, c10, 4
#endif

    /* Enable L2 cache */
#ifdef _ARM_ARCH_7
    mrc     p15, 0, r0, c1, c0, 1
    orr     r0, r0, #(1 << 1)

#ifdef BOARD_CONFIG_MSM8960_TOUCHPAD
    /* 
     * Cortex-A9 specific.....? God knows what's in the TouchPad.
     * Fuck Qualcomm and their stupid closed SoCs and their stupid
     * implementational differences. 
     */
    orr     r0, r0, #(1 << 6)   /* Cache Coherency (Cortex-A9 SMP) */
    orr     r0, r0, #(1 << 2)   /* L1 D-Cache Prefetch (Dside) */
#endif

    mcr     p15, 0, r0, c1, c0, 1
#endif

    /* Enable caching. */
    mrc     p15, 0, r0, c1, c0, 0
    orr     r0, r0, #(1 << 11)
    orr     r0, r0, #(1 << 12)
    orr     r0, r0, #(1 << 2)
    mcr     p15, 0, r0, c1, c0, 0

    /* Clear caches */
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 0

    /* Clear prefetch buffer */
#ifdef _ARM_ARCH_7
    isb     sy
    dsb     sy
#else
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 4
    mcr     p15, 0, r0, c7, c10, 4
#endif

    bx      lr

/**
 * cache_deinitialize
 *
 * Deinitialize ARM caches.
 */
EnterARM(cache_deinitialize)
    bx      lr
