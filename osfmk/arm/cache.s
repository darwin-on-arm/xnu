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

#if __ARM_ARCH == 6
#undef EnterThumb
#undef EnterThumb_NoAlign
#define EnterThumb_NoAlign EnterARM_NoAlign
#define EnterThumb EnterARM
#endif

/**
 * flush_dcache
 *
 * Invalidate entire ARM data-cache.
 */
EnterThumb(flush_dcache)
    mov     r0, #0
    mcr     p15, 0, r0, c7, c6, 0
    nop
    nop
    nop
    nop
    bx      lr

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
    nop
    nop
    nop
    nop
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
#if __ARM_ARCH == 7
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
#if __ARM_ARCH == 6
    mcr     p15, 0, r0, c7, c5, 4
    mcr     p15, 0, r0, c7, c10, 4
#endif

    /* Enable L2 cache */
#ifdef _ARM_ARCH_7
    mrc     p15, 0, r0, c1, c0, 1
    orr     r0, r0, #(1 << 1)
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
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 4

#if __ARM_ARCH == 7
    isb     sy
    dsb     sy
#else
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
