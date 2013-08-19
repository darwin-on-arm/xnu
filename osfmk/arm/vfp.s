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
 * VFP initialization
 */

#include <arm/arch.h>
#include <arm/asm_help.h>

#define VFP_ENABLE   (1 << 30)
/**
 * init_vfp
 *
 * Enable the use of VFP/NEON. This just enables cp10/cp11.
 */
EnterARM(init_vfp)
    mrc     p15, 0, r0, c1, c0, 2
    mov     r1, #0xf00000
    orr     r0, r0, r1
    mcr     p15, 0, r0, c1, c0, 2
    bx      lr

/**
 * vfp_context_save
 *
 * Save the current VFP state.
 */
EnterARM(vfp_context_save)
    /* Set enable state so things don't move */
    vmrs    r2, fpexc
    mov     r1, #VFP_ENABLE
    vmsr    fpexc, r1
    
    /* Store registers */
    vstmia  r0, {d0-d15}
    add     r0, r0, #0x80
    vstmia  r0, {d16-d31}
    add     r0, r0, #0x80
    
    /* Restore state */
    vmrs    r1, fpscr
    str     r1, [r0]
    vmsr    fpexc, r2
    bx      lr

/**
 * vfp_context_load
 *
 * Load the saved VFP state into the current registers.
 */
EnterARM(vfp_context_load)
    /* Set enable state so things don't move */
    vmrs    r2, fpexc
    mov     r1, #VFP_ENABLE
    vmsr    fpexc, r1
    
    /* Load registers */
    vldmia  r0, {d0-d15}
    add     r0, r0, #0x80
    vldmia  r0, {d16-d31}
    add     r0, r0, #0x80
    
    /* Restore state */
    ldr     r1, [r0]
    vmsr    fpscr, r1
    vmsr    fpexc, r2
    
    /* end. */
    bx      lr

/**
 * vfp_enable_exception
 *
 * Enable/disable the exception state based on input boolean
 * value,
 */
EnterARM(vfp_enable_exception)
    /* Get current state */
    vmrs    r2, fpexc
    
    /* Check the value */
    cmp     r0, #0
    mov     r1, #0
    
    /* It's one, set the enable state */
    orrne   r1, r1, #VFP_ENABLE
    
    /* Set the value in FPEXC */
    vmsr    fpexc, r1
    
    /* Return the original state. */
    mov     r0, r2
    bx      lr
