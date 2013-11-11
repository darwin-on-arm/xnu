/*	cpufunc.h,v 1.40.22.4 2007/11/08 10:59:33 matt Exp	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
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
 * cpufunc.h
 *
 * Prototypes for cpu, mmu and tlb related functions.
 */

#ifndef _ARM32_CPUFUNC_H_
#define _ARM32_CPUFUNC_H_

typedef int bool, vsize_t;

extern void	armv7_setttb(u_int, bool);

extern void	armv7_icache_sync_range(vaddr_t, vsize_t);
extern void	armv7_dcache_wb_range(vaddr_t, vsize_t);
extern void	armv7_dcache_wbinv_range(vaddr_t, vsize_t);
extern void	armv7_dcache_inv_range(vaddr_t, vsize_t);
extern void	armv7_idcache_wbinv_range(vaddr_t, vsize_t);

extern void	armv7_icache_sync_all(void);
extern void	armv7_cpu_sleep(int);
extern void	armv7_context_switch(u_int);
extern void	armv7_tlb_flushID_SE(u_int);
extern void 	armv7_tlb_flushID_RANGE(u_int, u_int);
extern void 	armv7_tlb_flushID(void);
extern void 	armv7_tlb_flushID_ASID(u_int);
extern void	armv7_drain_writebuf(void);

extern void	armv7_set_context_id(u_int);

extern void	armv7_setup(char *string);
extern void 	armv7_dcache_wbinv_all (void);
extern void	armv7_idcache_wbinv_all(void);

#endif