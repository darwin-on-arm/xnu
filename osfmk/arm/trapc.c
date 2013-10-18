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
 * ARM trap handlers.
 */

#include <mach/mach_types.h>
#include <mach/mach_traps.h>
#include <mach/thread_status.h>
#include <mach_assert.h>
#include <mach_kdp.h>
#include <kern/thread.h>
#include <kern/kalloc.h>
#include <stdarg.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <stdarg.h>
#include <machine/machine_routines.h>
#include <arm/misc_protos.h>
#include <pexpert/pexpert.h>
#include <pexpert/arm/boot.h>
#include <pexpert/arm/protos.h>
#include <vm/vm_fault.h>
#include <vm/vm_kern.h>     /* For kernel_map */
#include <libkern/OSByteOrder.h>
#include <arm/armops.h>

/* xxx prot flags are broken so they're all VM_PROT_READ */

typedef enum {
    SLEH_ABORT_TYPE_PREFETCH_ABORT = 3,
    SLEH_ABORT_TYPE_DATA_ABORT = 4,
} sleh_abort_reasons;

void
arm_mach_do_exception(void)
{
    exception_triage(0, 0, 0);
}

void
doexception(int exc, mach_exception_code_t code, mach_exception_subcode_t sub)
{
    mach_exception_data_type_t   codes[EXCEPTION_CODE_MAX];

    codes[0] = code;    
    codes[1] = sub;
    exception_triage(exc, codes, 2);
}

/**
 * __arm_get_dfsr
 *
 * Get the current data fault status register.
 */
static inline uint32_t __arm_get_dfsr(void)
{
    uint32_t    arm_register;
    __asm__ __volatile__ ("mrc    p15, 0, %0, c5, c0, 0" : "=r"(arm_register));
    return arm_register;
}

/**
 * __arm_get_dfar
 *
 * Get the current data fault address register.
 */
static inline uint32_t __arm_get_dfar(void)
{
    uint32_t    arm_register;
    __asm__ __volatile__ ("mrc    p15, 0, %0, c6, c0, 0" : "=r"(arm_register));
    return arm_register;
}

/**
 * __arm_get_ifsr
 *
 * Get the current instruction fault status register.
 */
static inline uint32_t __arm_get_ifsr(void)
{
    uint32_t    arm_register;
    __asm__ __volatile__ ("mrc    p15, 0, %0, c5, c0, 1" : "=r"(arm_register));
    return arm_register;
}

/**
 * __arm_get_dfsr
 *
 * Get the current data fault status register.
 */
static inline uint32_t __arm_get_ifar(void)
{
    uint32_t    arm_register;
    __asm__ __volatile__ ("mrc    p15, 0, %0, c6, c0, 1" : "=r"(arm_register));
    return arm_register;
}

/**
 * ifsr_to_human
 *
 * Return a human readable representation of the IFSR bits.
 */
static char* ifsr_to_human(uint32_t ifsr) {
    switch((ifsr & 0xF)) {
        case 0:
            return "No function, reset value";
        case 1:
            return "Alignment fault";
        case 2:
            return "Debug event fault";
        case 3:
            return "Access flag fault on section";
        case 4:
            return "No function";
        case 5:
            return "Translation fault on section";
        case 6:
            return "Access flag fault on page";
        case 7:
            return "Translation fault on page";
        case 8:
            return "Precise external abort";
        case 9:
            return "Domain fault on section";
        case 10:
            return "No function";
        case 11:
            return "Domain fault on page";
        case 12:
            return "External abort on translation, level one";
        case 13:
            return "Permission fault on section";
        case 14:
            return "External abort on translation, level two";
        case 15:
            return "Permission fault on page";
        default:
            return "Unknown";
    }
    return "Unknown";
}

/**
 * sleh_abort
 *
 * Handle prefetch and data aborts. (EXC_BAD_ACCESS IS NOT HERE YET)
 */
static int __abort_count = 0;
void sleh_abort(void* context, int reason)
{
    uint32_t    dfsr, dfar, ifsr, ifar, cpsr, exception_type = 0, exception_subcode = 0;
    arm_saved_state_t* arm_ctx = (arm_saved_state_t*)context;
    thread_t thread = current_thread();

    /* Make sure we get the correct registers only if required. */
    if(reason == SLEH_ABORT_TYPE_DATA_ABORT) {
        dfsr    = __arm_get_dfsr();
        dfar    = __arm_get_dfar();
    } else if(reason == SLEH_ABORT_TYPE_PREFETCH_ABORT) {
        ifsr    = __arm_get_ifsr();
        ifar    = __arm_get_ifar();
    } else {
        panic("sleh_abort: weird abort, type %d (context at %p)", reason, context);
    }

    /* We do not want anything entering sleh_abort recursively. */
    if(__abort_count != 0) {
        panic("sleh_abort: recursive abort! (dfar: 0x%08x)", dfar);
    }
    __abort_count++;

    if(!kernel_map) {
        panic("sleh_abort: kernel_map is NULL\n");
    }

    if(!thread) {
        panic("sleh_abort: current thread is NULL\n");
    }

    /* See if the abort was in Kernel or User mode. */
    cpsr = arm_ctx->cpsr & 0x1F;

    /* Kernel mode. (ARM Supervisor) */
    if(cpsr == 0x13) {
        switch(reason) {
            /* Prefetch aborts always include the IFSR and IFAR. */
            case SLEH_ABORT_TYPE_PREFETCH_ABORT: {
                /* Die in a fire. */
                vm_map_t map;
                kern_return_t code;

                /* Get the kernel thread map. */
                map = kernel_map;

                /* Attempt to fault the page. */
                code = vm_fault(map, vm_map_trunc_page(arm_ctx->pc), (VM_PROT_READ | VM_PROT_WRITE), FALSE, THREAD_UNINT, NULL, vm_map_trunc_page(0));

                if(code != KERN_SUCCESS) {
                    /* Still, die in a fire. */
                    panic_context(0, (void*)arm_ctx, 
                                 "sleh_abort: prefetch abort in kernel mode: fault_addr=0x%x\n"
                                 "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                                 "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                                 "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                                 "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                                 "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                                 ifar, arm_ctx->r[0], arm_ctx->r[1], arm_ctx->r[2], arm_ctx->r[3],
                                 arm_ctx->r[4], arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7], arm_ctx->r[8],
                                 arm_ctx->r[9], arm_ctx->r[10], arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp,
                                 arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr, ifsr, ifar);
                } else {
                    __abort_count--;
                }
                return;
            }
            case SLEH_ABORT_TYPE_DATA_ABORT: {
                vm_map_t map;
                kern_return_t code;

                /* Get the current thread map. */
                map = thread->map;

                /* Attempt to fault the page. */
                code = vm_fault(map, vm_map_trunc_page(dfar), (VM_PROT_READ), FALSE, THREAD_UNINT, NULL, vm_map_trunc_page(0));
                if(code != KERN_SUCCESS) {

                    /* Still, die in a fire. */
                    if(!thread->recover) {
                        panic_context(0, (void*)arm_ctx, 
                                     "sleh_abort: data abort in kernel mode: fault_addr=0x%x\n"
                                     "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                                     "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                                     "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                                     "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                                     "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                                     dfar, arm_ctx->r[0], arm_ctx->r[1], arm_ctx->r[2], arm_ctx->r[3],
                                     arm_ctx->r[4], arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7], arm_ctx->r[8],
                                     arm_ctx->r[9], arm_ctx->r[10], arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp,
                                     arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr, dfsr, dfar);
                    } else {
                        /* If there's a recovery routine, use it. */
                        kprintf("[trap] data abort in kernel mode, transferring control to RecoveryRoutine %p, dfsr %x, dfar %x\n", thread->recover, dfsr, dfar);
                        arm_ctx->pc = thread->recover;
                        thread->recover = NULL;
                    }
                }
                __abort_count--;
                return;
            }
            default:
                panic("sleh_abort: unknown kernel mode abort, type %d\n", reason);
        }
    /* User mode (ARM User) */
    } else if(cpsr == 0x10) {
        switch(reason) {
            /* User prefetch abort */
            case SLEH_ABORT_TYPE_PREFETCH_ABORT: {
                /* Attempt to fault it. Same as data except address comes from IFAR. */
                vm_map_t map;
                kern_return_t code;

                /* Get the current thread map. */
                map = thread->map;

                /* Attempt to fault the page. */
                code = vm_fault(map, vm_map_trunc_page(ifar), (VM_PROT_READ), FALSE, THREAD_UNINT, NULL, vm_map_trunc_page(0));

                if((code != KERN_SUCCESS) && (code != KERN_ABORTED)) {
                    exception_type = EXC_BAD_ACCESS;
                    exception_subcode = ifar;

                    /* Debug only. */
                    printf("%s[%d]: usermode prefetch abort, EXC_BAD_ACCESS at 0x%08x in map %p (pmap %p) (%s)\n",
                           proc_name_address(thread->task->bsd_info), proc_pid(thread->task->bsd_info),
                           ifar, map, map->pmap, ifsr_to_human(ifsr));
                    printf("Thread has ARM register state:\n"
                           "    r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                           "    r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                           "    r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                           "   r12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                           "  cpsr: 0x%08x\n", arm_ctx->r[0], arm_ctx->r[1], arm_ctx->r[2], arm_ctx->r[3],
                           arm_ctx->r[4], arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7], arm_ctx->r[8],
                           arm_ctx->r[9], arm_ctx->r[10], arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp,
                           arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr
                           );
                    /* Even more debug. */
                    panic("sleh_abort: USER DEBUG");
                } else {
                    /* Retry execution of instruction. */
                    __abort_count--;
                    return;
                }
                break;
            }
            /* User Data Abort */
            case SLEH_ABORT_TYPE_DATA_ABORT: {
                /* Attempt to fault it. Same as instruction except address comes from DFAR. */
                vm_map_t map;
                kern_return_t code;

                /* Get the current thread map. */
                map = thread->map;

                /* Attempt to fault the page. */
                code = vm_fault(map, vm_map_trunc_page(dfar), (VM_PROT_READ), FALSE, THREAD_UNINT, NULL, vm_map_trunc_page(0));

                if((code != KERN_SUCCESS) && (code != KERN_ABORTED)) {
                    exception_type = EXC_BAD_ACCESS;
                    exception_subcode = dfar;

                    /* Only for debug. */
                    printf("%s[%d]: usermode data abort, EXC_BAD_ACCESS at 0x%08x in map %p (pmap %p) (%s)\n",
                           proc_name_address(thread->task->bsd_info), proc_pid(thread->task->bsd_info),
                           dfar, map, map->pmap, ifsr_to_human(dfsr));
                    printf("Thread has ARM register state:\n"
                           "    r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                           "    r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                           "    r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                           "   r12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                           "  cpsr: 0x%08x\n", arm_ctx->r[0], arm_ctx->r[1], arm_ctx->r[2], arm_ctx->r[3],
                           arm_ctx->r[4], arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7], arm_ctx->r[8],
                           arm_ctx->r[9], arm_ctx->r[10], arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp,
                           arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr
                           );
                    panic("sleh_abort: USER DEBUG");
                } else {
                    /* Retry execution of instruction. */
                    __abort_count--;
                    return;
                }
                break;
            }
            default:
                exception_type = EXC_BREAKPOINT;
                exception_subcode = 0;
                break;
        }
    /* Unknown mode. */
    } else {
        panic("sleh_abort: Abort in unknown mode, cpsr: 0x%08x\n", cpsr);
    }

    /* If there was a user exception, handle it. */
    if(exception_type) {
        __abort_count--;
        doexception(exception_type, 0, exception_subcode);
    }

    /* Done. */
    return;
}

/**
 * irq_handler
 *
 * Handle irqs and pass them over to the platform expert.
 */
boolean_t irq_handler(void* context)
{
    /* Disable system preemption, dispatch the interrupt and go. */
    __disable_preemption();

    /* Dispatch the interrupt. */
    boolean_t ret = pe_arm_dispatch_interrupt(context);

    /* Go. */
    __enable_preemption();

    return ret;
}

/**
 * sleh_undef
 *
 * Handle undefined instructions and VFP usage.
 */
void sleh_undef(arm_saved_state_t* state)
{
    panic("sleh_undef pc %x lr %x cpsr %x\n", state->pc, state->lr, state->cpsr);
}
