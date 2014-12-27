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
 * CPU i-cache/d-cache operations for arm
 */

#include <arm/arch.h>
#include <arm/asm_help.h>

/**
 * invalidate_dcache
 *
 * Invalidate entire ARM data-cache.
 */
EnterARM(invalidate_dcache)
    bl _arm_dcache_inv_all

/**
 * invalidate_dcache_region
 *
 * Invalidate an ARM data-cache region.
 */
EnterARM(invalidate_dcache_region)
    bl _arm_dcache_inv_range

/**
 * dcache_incoherent_io_store64
 *
 * Shim function to clean_dcache_region
 */
EnterARM(dcache_incoherent_io_store64)
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
EnterARM_NoAlign(clean_dcache_region)
    bl _arm_dcache_wb_range

/**
 * cleanflush_dcache_region
 *
 * Clean and flush entire ARM data-cache.
 */
EnterARM(cleanflush_dcache)
    bl _arm_dcache_wbinv_all

/**
 * dcache_incoherent_io_flush64
 *
 * Shim function to cleanflush_dcache_region
 */
EnterARM(dcache_incoherent_io_flush64)
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
EnterARM_NoAlign(cleanflush_dcache_region)
    bl _arm_dcache_wbinv_range

/**
 * flush_dcache64
 *
 * Shim function to flush_dcache
 */
EnterARM(flush_dcache64)
    mov     r1, r2
    mov     r2, r3

/**
 * flush_dcache
 *
 * Flush entire ARM data-cache.
 */
EnterARM_NoAlign(flush_dcache)
    bl _arm_dcache_wbinv_all

/**
 * invalidate_icache64
 *
 * Shim function to invalidate_icache
 */
EnterARM(invalidate_icache64)
    mov     r1, r2
    mov     r2, r3

/**
 * invalidate_icache
 *
 * Invalidate entire ARM i-cache.
 */
EnterARM_NoAlign(invalidate_icache)
    bl _arm_icache_sync_all

/**
 * invalidate_icache_region
 *
 * Invalidate an ARM i-cache region.
 */
EnterARM(invalidate_icache_region)
    bl _arm_icache_sync_range

/**
 * invalidate_branch_target_cache
 *
 * Invalidate the entire ARM branch target buffer (or cache).
 */
EnterARM(invalidate_branch_target_cache)
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
EnterARM(flush_mmu_tlb)
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
#ifdef _ARM_ARCH_6
    mcr     p15, 0, r0, c7, c5, 4
    mcr     p15, 0, r0, c7, c10, 4
#endif

    /* Enable L2 cache */
#ifdef _ARM_ARCH_7
    mrc     p15, 0, r0, c1, c0, 1
    orr     r0, r0, #(1 << 1)

#ifdef BOARD_CONFIG_MSM8960_TOUCHPAD
    /*
     * Cortex-A9 specific.....? God knows what is in the TouchPad.
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
