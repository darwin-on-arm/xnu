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
 * ARM system startup
 */

#include <arm/arch.h>
#include <arm/asm_help.h>
#include <assym.s>
#include <mach/arm/asm.h>

/*
 * During system initialization, there are two possible methods of
 * initial bootstrap.
 *
 * The old BootKit loader prepared the initial virtual memory mappings
 * for the kernel. When we boot using a shim loader, we don't get this
 * luxury, so we have to do it ourselves. Isn't that quite fun?
 *
 *  - r0 = kernel boot-args structure.
 *
 * The boot-args structure will be updated to then be virtual.
 */

EnterARM(_start)
    /* First, disable interrupts so that the BL doesn't get any. */
    LOAD_ADDR(lr, arm_init)
    cpsid   if

    /* If MMU is initialized, go the quick way. */
    mrc     p15, 0, r4, c1, c0, 0
    and     r4, #0x1
    cmp     r4, #0x1
    beq     mmu_initialized

    /*
     * MMU initialization part begins here. -----------------------
     *
     * Basically, all of SDRAM gets remapped to the virtual base.
     */

    /* Adjust DACR register. */
    mov     r4, #0x1

    mcr     p15, 0, r4, c3, c0, 0
#ifdef _ARM_ARCH_7
    isb     sy
#endif

    /* Clean TLB and instruction cache. */
    mov     r4, #0
    mcr     p15, 0, r4, c8, c7, 0
    mcr     p15, 0, r4, c7, c5, 0
    mcr     p15, 0, r4, c2, c0, 2

    /*
     * Create a dumb mapping for right now. This mapping lies
     * at the top of kernel data.
     */
    mov     r10, #0x80000000    /* xxx kaslr, remaining part is patching __nl_symbol_ptr */
    str     r10, [r0, BOOT_ARGS_VIRTBASE]

    ldr     r4, [r0, BOOT_ARGS_TOP_OF_KERNEL]
    ldr     r10, [r0, BOOT_ARGS_VIRTBASE]
    ldr     r11, [r0, BOOT_ARGS_PHYSBASE]
    ldr     r12, [r0, BOOT_ARGS_MEMSIZE]

    orr     r5, r4, #0x18

    /* Now, we have to set our TTB to this value. */
    mcr     p15, 0, r5, c2, c0, 0

    /* Make our section mappings now. */
    mov     r6, #0xE            /* This is a section descriptor */
    orr     r6, r6, #0x400      /* Permissions */

    /* Identity map UART for right now */
    LoadConstantToReg((0x49000000 + 0x20000), r7)
    mov     r7, r7, lsr#20
    add     r5, r4, r7, lsl#2
    mov     r7, r7, lsl#20
    orr     r8, r7, r6
    str     r8, [r5]

    mov     r7, pc, lsr#20
    add     r5, r4, r7, lsl#2   /* Make the TTE offset */

    /* God, I hope we're loaded at the beginning of SDRAM. */
    mov     r7, r7, lsl#20
    orr     r8, r7, r6

    /* Store our section mappings. */
    str     r8, [r5]

    /* Get the physical address... */
    mov     r1, r11
    add     r5, r4, r10, lsr#18
map:
    /* Just map all of SDRAM. */
    orr     r8, r1, r6
    str     r8, [r5], #4
    add     r1, r1, #_1MB
    subs    r12, r12, #_1MB
    bne     map

    /* Start MMU. */
    mrc     p15, 0, r3, c1, c0, 0
    orr     r3, r3, #1
    mcr     p15, 0, r3, c1, c0, 0

    /*
     * Hopefully, if we got here, things are looking good and we
     * are running in VM mode.
     */

     /*
      * xxx KASLR: we need to jump to a trampoline.
      * The address in r3 is relative, we convert it to a KVA and jump.
      */
    adr     r3, start_trampoline
    sub     r3, r3, r11
    add     r3, r3, r10
    bx      r3
start_trampoline:
    nop     

fix_boot_args_hack_for_bootkit:
    /* Fix up boot-args */
    sub     r0, r0, r11
    add     r0, r0, r10

    /* Goddamn section offset. */
    LOAD_ADDR(r12, sectionOffset)
    mov     sp, #0
    str     sp, [r12]

#if USE_VBAR_EXCVECT
    /*
     * VBAR Note:
     * The exception vectors are mapped high also at 0xFFFF0000 for compatibility purposes.
     */

    /* Set low vectors. */
    mrc     p15, 0, r4, c1, c0, 0
    bic     r4, r4, #(1 << 13)
    mcr     p15, 0, r4, c1, c0, 0

    /* Set NS-VBAR to ExceptionVectorsBase */
    LOAD_ADDR(r4, ExceptionVectorsBase)
    mcr     p15, 0, r4, c12, c0, 0

#else

    /* Now, the vectors could be mapped low. Fix that. */
    mrc     p15, 0, r4, c1, c0, 0
    orr     r4, r4, #(1 << 13)
    mcr     p15, 0, r4, c1, c0, 0

#endif

    /*
     * MMU initialization end. ------------------------------------
     */

mmu_initialized:
    /*
     * Zero out the frame pointer so that the kernel fp tracer
     * doesn't go farther than it needs to.
     */
    mov     r7, #0

    /* Enable unaligned memory access and caching */
    mrc     p15, 0, r4, c1, c0, 0
    orr     r4, r4, #(1 << 22)  /* Force unaligned accesses, fixes OMAP boot. */
    bic     r4, r4, #(1 << 1)
    orr     r4, r4, #(1 << 23)  /* Unaligned memory access */
    orr     r4, r4, #(1 << 12)  /* Enable I-cache */
    mcr     p15, 0, r4, c1, c0, 0

    /* Invalid Data/Inst TLB */
    mov     r4, #0
    mcr     p15, 0, r4, c8, c7, 0

    /* Invalidate caches */
    mcr     p15, 0, r4, c7, c5, 0

    /* Set CONTEXIDR to 0, kernel ASID. */
    mcr     p15, 0, r4, c13, c0, 1

    /* Set up initial sp. */
    LOAD_ADDR(sp, intstack_top)

    /* Boot to ARM init. */
    bx      lr

/*
 * Initial stack
 */
.data
.align 4
.globl _intstack_top
.globl _intstack
_intstack:
.space (8192), 0
_intstack_top:

/*
 * ARM SMP stack.
 */
.globl _debstack_top
.globl _debstack
_debstack:
.space (8192), 0
_debstack_top:

LOAD_ADDR_GEN_DEF(ExceptionVectorsBase)
LOAD_ADDR_GEN_DEF(arm_init)
LOAD_ADDR_GEN_DEF(intstack_top)
LOAD_ADDR_GEN_DEF(sectionOffset)
