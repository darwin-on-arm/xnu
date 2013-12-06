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
 * Other prototypes for the ARM platform.
 */

#ifndef _ARM_MISC_PROTOS_H_
#define _ARM_MISC_PROTOS_H_

#include <mach/mach_types.h>
#include <pexpert/pexpert.h>
#include <machine/machine_routines.h>
#include <mach/vm_param.h>
#include <kern/processor.h>

typedef struct _abort_information_context {
    uint32_t r[13];
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t cpsr;
    uint32_t fsr;
    uint32_t far;
} abort_information_context_t;

extern processor_t cpu_processor_alloc(boolean_t is_boot_cpu);
extern void cpu_init(void);
extern void cpu_bootstrap(void);

extern void draw_panic_dialog(void);

#ifndef __LP64__
extern void arm_set_threadpid_user_readonly(uint32_t * address);
extern void arm_set_threadpid_priv_readwrite(uint32_t * address);
#else
extern void arm_set_threadpid_user_readonly(uint64_t * address);
extern void arm_set_threadpid_priv_readwrite(uint64_t * address);
#endif

extern int arm_usimple_lock(usimple_lock_t l);

void panic_arm_backtrace(void *_frame, int nframes, const char *msg,
                         boolean_t regdump, arm_saved_state_t * regs);

void arm_vm_init(uint32_t mem_limit, boot_args * args);

void sleh_abort(void *context, int reason);

void cache_initialize(void);

void get_cachetype_cp15();
void identify_arm_cpu(void);

#endif
