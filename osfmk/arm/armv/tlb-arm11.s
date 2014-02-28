/*-
 * Copyright (c) 2010 Per Odlund <per.odlund@armagedon.se>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* arm11 assembly functions for manipulating caches and other core functions.
 * Based on cpufuncs for v6 and xscale.
 */

#include <mach/arm/asm.h>
#include <arm/asm_help.h>
#include <arm/arch.h>

#ifdef _ARM_ARCH_6

#define ENTRY_NP ENTRY
#define _C_LABEL(x) _ ##x

ENTRY(arm11_cpu_sleep)
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, 4   /* wait for interrupt */
	RET
END(arm11_cpu_sleep)

ENTRY(arm11_wait)
	mrc	p15, 0, r0, c2, c0, 0	@ arbitrary read of CP15
	add	r0, r0, #0		@ a stall
	bx	lr
END(arm11_wait)

ENTRY(arm11_context_switch)
    /*
	 * We can assume that the caches will only contain kernel addresses
	 * at this point.  So no need to flush them again.
	 */
	mcr	p15, 0, r0, c7, c10, 4	/* drain the write buffer */
	mcr	p15, 0, r0, c2, c0, 0	/* set the new TTB */
	mcr	p15, 0, r0, c8, c7, 0	/* and flush the I+D tlbs */

	/* Paranoia -- make sure the pipeline is empty. */
	nop
	nop
	nop
	RET
END(arm11_context_switch)

ENTRY(arm11_tlb_flushID)
	mcr	p15, 0, r0, c8, c7, 0	@ /* flush I+D tlb */
	mcr	p15, 0, r0, c7, c10, 4	@ /* drain write buffer */
	mov	pc, lr
END(arm11_tlb_flushID)

ENTRY(arm11_tlb_flushID_RANGE)
	mcr	p15, 0, r0, c7, c10, 4	@ /* drain write buffer */
	mov r0, r0, lsr#12
	mov r1, r1, lsr#12
	mov r0, r0, lsl#12
	mov r1, r1, lsl#12
1:	mcr p15, 0, r0, c8, c7, 1 	@ flush I+D tlb single entry
	add r0, r0, #0x1000 		@ page size
	cmp r0, r1
	bcc 1b
	mov r2, #0
	mcr p15, 0, r2, c7, c5, 4 	@ BPIALL
	mcr	p15, 0, r0, c7, c10, 4	@ /* drain write buffer */
	bx lr
END(arm11_tlb_flushID_RANGE)

ENTRY(arm11_tlb_flushID_SE)
	mcr	p15, 0, r0, c8, c7, 0	@ /* flush I+D tlb */
	mcr	p15, 0, r0, c7, c10, 4	@ /* drain write buffer */
	mov	pc, lr
END(arm11_tlb_flushID_SE)


ENTRY_NP(arm11_setttb)
	mcr	p15, 0, r0, c2, c0, 0	/* load new TTB */

	mcr	p15, 0, r0, c8, c7, 0	/* invalidate I+D TLBs */
	mcr	p15, 0, r0, c7, c10, 4	/* drain write buffer */
	RET
END(arm11_setttb)

/* Other functions. */

ENTRY_NP(arm11_drain_writebuf)
	mcr	p15, 0, r0, c7, c10, 4	/* drain write buffer */
	mov	pc, lr
END(arm11_drain_writebuf)

#endif