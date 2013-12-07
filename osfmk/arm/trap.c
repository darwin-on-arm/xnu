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
#include <vm/vm_kern.h>         /* For kernel_map */
#include <libkern/OSByteOrder.h>
#include <arm/armops.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef enum {
    SLEH_ABORT_TYPE_PREFETCH_ABORT = 3,
    SLEH_ABORT_TYPE_DATA_ABORT = 4,
} sleh_abort_reasons;

void doexception(int exc, mach_exception_code_t code,
                 mach_exception_subcode_t sub)
{
    mach_exception_data_type_t codes[EXCEPTION_CODE_MAX];

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
    uint32_t arm_register;
    __asm__ __volatile__("mrc    p15, 0, %0, c5, c0, 0":"=r"(arm_register));
    return arm_register;
}

/**
 * __arm_get_dfar
 *
 * Get the current data fault address register.
 */
static inline uint32_t __arm_get_dfar(void)
{
    uint32_t arm_register;
    __asm__ __volatile__("mrc    p15, 0, %0, c6, c0, 0":"=r"(arm_register));
    return arm_register;
}

/**
 * __arm_get_ifsr
 *
 * Get the current instruction fault status register.
 */
static inline uint32_t __arm_get_ifsr(void)
{
    uint32_t arm_register;
    __asm__ __volatile__("mrc    p15, 0, %0, c5, c0, 1":"=r"(arm_register));
    return arm_register;
}

/**
 * __arm_get_dfsr
 *
 * Get the current data fault status register.
 */
static inline uint32_t __arm_get_ifar(void)
{
    uint32_t arm_register;
    __asm__ __volatile__("mrc    p15, 0, %0, c6, c0, 1":"=r"(arm_register));
    return arm_register;
}

/**
 * ifsr_to_human
 *
 * Return a human readable representation of the IFSR bits.
 */
static char *ifsr_to_human(uint32_t ifsr)
{
    switch ((ifsr & 0xF)) {
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
 * sleh_fatal_exception
 */
void sleh_fatal_exception(abort_information_context_t * arm_ctx, char *message)
{
    debug_mode = TRUE;
    printf("Fatal exception: %s\n", message);
    printf("ARM register state: (saved state %p)\n"
           "  r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
           "  r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
           "  r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
           "  12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
           "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n", arm_ctx,
           arm_ctx->r[0], arm_ctx->r[1], arm_ctx->r[2], arm_ctx->r[3],
           arm_ctx->r[4], arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7],
           arm_ctx->r[8], arm_ctx->r[9], arm_ctx->r[10], arm_ctx->r[11],
           arm_ctx->r[12], arm_ctx->sp, arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr,
           arm_ctx->fsr, arm_ctx->far);
    printf("Current thread: %p\n", current_thread());

    uint32_t ttbcr, ttbr0, ttbr1;
    __asm__ __volatile__("mrc p15, 0, %0, c2, c0, 0":"=r"(ttbr0));
    __asm__ __volatile__("mrc p15, 0, %0, c2, c0, 1":"=r"(ttbr1));
    __asm__ __volatile__("mrc p15, 0, %0, c2, c0, 2":"=r"(ttbcr));

    printf("Control registers:\n"
           "  ttbcr: 0x%08x  ttbr0:  0x%08x  ttbr1:  0x%08x\n",
           ttbcr, ttbr0, ttbr1);
    Debugger("fatal exception");
    printf("We are hanging here ...\n");

    Halt_system();
}

/**
 * sleh_abort
 *
 * Handle prefetch and data aborts. (EXC_BAD_ACCESS IS NOT HERE YET)
 */
static int __abort_count = 0;
void sleh_abort(void *context, int reason)
{
    uint32_t dfsr = 0, dfar = 0, ifsr = 0, ifar = 0, cpsr, exception_type =
        0, exception_subcode = 0;
    abort_information_context_t *arm_ctx =
        (abort_information_context_t *) context;
    thread_t thread = current_thread();

    /*
     * Make sure we get the correct registers only if required. 
     */
#if 0
    kprintf("sleh_abort: pc %x lr %x far %x fsr %x psr %x\n", arm_ctx->pc, arm_ctx->lr, arm_ctx->far, arm_ctx->fsr, arm_ctx->cpsr);
#endif
    if (reason == SLEH_ABORT_TYPE_DATA_ABORT) {
        dfsr = arm_ctx->fsr;
        dfar = arm_ctx->far;
    } else if (reason == SLEH_ABORT_TYPE_PREFETCH_ABORT) {
        ifsr = arm_ctx->fsr;
        ifar = arm_ctx->far;
    } else {
        sleh_fatal_exception(arm_ctx, "sleh_abort: weird abort");
    }

    /*
     * We do not want anything entering sleh_abort recursively. 
     */
    if (__abort_count != 0) {
        sleh_fatal_exception(arm_ctx, "sleh_abort: recursive abort");
    }
    __abort_count++;

    /*
     * Panic if it's an alignment fault?
     */
    if ((ifsr == 1) || (dfsr == 1)) {
        sleh_fatal_exception(arm_ctx, "sleh_abort: alignment fault");
    }

    if (!kernel_map) {
        sleh_fatal_exception(arm_ctx,
                             "sleh_abort: kernel map is NULL, probably a fault before vm_bootstrap?");
    }

    if (!thread) {
        sleh_fatal_exception(arm_ctx, "sleh_abort: current thread is null?");
    }

    if(ml_at_interrupt_context()) {
        sleh_fatal_exception(arm_ctx, "sleh_abort: Abort in interrupt handler");
    }

    /*
     * See if the abort was in Kernel or User mode. 
     */
    cpsr = arm_ctx->cpsr & 0x1F;

    /*
     * Kernel mode. (ARM Supervisor) 
     */
    if (cpsr == 0x13) {
        switch (reason) {
            /*
             * Prefetch aborts always include the IFSR and IFAR. 
             */
        case SLEH_ABORT_TYPE_PREFETCH_ABORT:{
                /*
                 * Die in a fire. 
                 */
                vm_map_t map;
                kern_return_t code;

                /*
                 * Get the kernel thread map. 
                 */
                map = kernel_map;

                /*
                 * Attempt to fault the page. 
                 */
                __abort_count--;
                code =
                    vm_fault(map, vm_map_trunc_page(arm_ctx->pc),
                             (VM_PROT_EXECUTE | VM_PROT_READ), FALSE,
                             THREAD_UNINT, NULL, vm_map_trunc_page(0));

                if (code != KERN_SUCCESS) {

                    if (current_debugger) {
                        if (kdp_raise_exception(EXC_BREAKPOINT, 0, 0, &arm_ctx))
                            return;
                    }
                    
                    /*
                     * Still, die in a fire. 
                     */
                    panic_context(0, (void *) arm_ctx,
                                  "Kernel prefetch abort. (faulting address: 0x%08x, saved state 0x%08x)\n"
                                  "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                                  "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                                  "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                                  "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                                  "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                                  ifar, arm_ctx, arm_ctx->r[0], arm_ctx->r[1],
                                  arm_ctx->r[2], arm_ctx->r[3], arm_ctx->r[4],
                                  arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7],
                                  arm_ctx->r[8], arm_ctx->r[9], arm_ctx->r[10],
                                  arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp,
                                  arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr, ifsr,
                                  ifar);
                }
                return;
            }
        case SLEH_ABORT_TYPE_DATA_ABORT:{
                vm_map_t map;
                kern_return_t code;

                /*
                 * Get the current thread map. 
                 */
                map = thread->map;

                /*
                 * Attempt to fault the page. 
                 */
                __abort_count--;
                code =
                    vm_fault(map, vm_map_trunc_page(dfar),
                             (dfsr & 0x800) ? (VM_PROT_READ | VM_PROT_WRITE)
                             : (VM_PROT_READ), FALSE, THREAD_UNINT, NULL,
                             vm_map_trunc_page(0));

                if (code != KERN_SUCCESS) {
                    /*
                     * Still, die in a fire. 
                     */
                    code =
                        vm_fault(kernel_map, vm_map_trunc_page(dfar),
                                 (dfsr & 0x800) ? (VM_PROT_READ | VM_PROT_WRITE)
                                 : (VM_PROT_READ), FALSE, THREAD_UNINT, NULL,
                                 vm_map_trunc_page(0));
                    if (code != KERN_SUCCESS) {
                        /*
                         * Attempt to fault the page against the kernel map. 
                         */
                        if (!thread->recover) {
                            panic_context(0, (void *) arm_ctx,
                                          "Kernel data abort. (faulting address: 0x%08x, saved state 0x%08x)\n"
                                          "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                                          "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                                          "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                                          "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                                          "cpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                                          dfar, arm_ctx, arm_ctx->r[0], arm_ctx->r[1],
                                          arm_ctx->r[2], arm_ctx->r[3],
                                          arm_ctx->r[4], arm_ctx->r[5],
                                          arm_ctx->r[6], arm_ctx->r[7],
                                          arm_ctx->r[8], arm_ctx->r[9],
                                          arm_ctx->r[10], arm_ctx->r[11],
                                          arm_ctx->r[12], arm_ctx->sp,
                                          arm_ctx->lr, arm_ctx->pc,
                                          arm_ctx->cpsr, dfsr, dfar);
                        } else {
                            /*
                             * If there's a recovery routine, use it. 
                             */
                            if (thread->map == kernel_map)
                                panic
                                    ("Attempting to use a recovery routine on a kernel map thread");

                            if (!thread->map)
                                sleh_fatal_exception(arm_ctx,
                                                     "Current thread has no thread map, what?");

                            arm_ctx->pc = thread->recover;
                            arm_ctx->cpsr &= ~(1 << 5);
                            thread->recover = NULL;
                            return;
                        }
                    }
                }
                return;
            }
        default:
            panic("sleh_abort: unknown kernel mode abort, type %d\n", reason);
        }
        /*
         * User mode (ARM User) 
         */
    } else if (cpsr == 0x10) {
        switch (reason) {
            /*
             * User prefetch abort 
             */
        case SLEH_ABORT_TYPE_PREFETCH_ABORT:{
                /*
                 * Attempt to fault it. Same as data except address comes from IFAR. 
                 */
                vm_map_t map;
                kern_return_t code;

                /*
                 * Get the current thread map. 
                 */
                map = thread->map;
                /*
                 * Attempt to fault the page. 
                 */
                assert(get_preemption_level() == 0);
                __abort_count--;
                code =
                    vm_fault(map, vm_map_trunc_page(arm_ctx->pc),
                             (VM_PROT_EXECUTE | VM_PROT_READ), FALSE,
                             THREAD_UNINT, NULL, vm_map_trunc_page(0));

                /*
                 * Additionally, see if we can fault one page higher as the instruction
                 * may be on a page split boundary. libobjc and all require this???
                 *
                 * Prefaulting the instruction before allows the prefetch mechanism
                 * to not abort.
                 */
                if((arm_ctx->pc & 0xfff) >= 0xff0)
                    vm_fault(map, vm_map_trunc_page(arm_ctx->pc) + PAGE_SIZE,
                             (VM_PROT_EXECUTE | VM_PROT_READ), FALSE,
                             THREAD_UNINT, NULL, vm_map_trunc_page(0));

                if ((code != KERN_SUCCESS) && (code != KERN_ABORTED)) {
                    exception_type = EXC_BAD_ACCESS;
                    exception_subcode = 0;

                    /*
                     * Debug only. 
                     */
                    printf
                        (ANSI_COLOR_RED "%s[%d]: " ANSI_COLOR_YELLOW "usermode prefetch abort, EXC_BAD_ACCESS at 0x%08x in map %p (pmap %p) (%s)" ANSI_COLOR_RESET" \n",
                         proc_name_address(thread->task->bsd_info),
                         proc_pid(thread->task->bsd_info), arm_ctx->pc, map,
                         map->pmap, ifsr_to_human(ifsr));
                    printf("Thread has ARM register state:\n"
                           "    r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                           "    r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                           "    r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                           "   r12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                           "  cpsr: 0x%08x\n", arm_ctx->r[0], arm_ctx->r[1],
                           arm_ctx->r[2], arm_ctx->r[3], arm_ctx->r[4],
                           arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7],
                           arm_ctx->r[8], arm_ctx->r[9], arm_ctx->r[10],
                           arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp,
                           arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr);
                    printf("dyld_all_image_info_addr: 0x%08x   dyld_all_image_info_size: 0x%08x\n",
                            thread->task->all_image_info_addr, thread->task->all_image_info_size);
                } else {
                    /*
                     * Retry execution of instruction. 
                     */
                    ml_set_interrupts_enabled(TRUE);
                    return;
                }
                break;
            }
            /*
             * User Data Abort 
             */
        case SLEH_ABORT_TYPE_DATA_ABORT:{
                /*
                 * Attempt to fault it. Same as instruction except address comes from DFAR. 
                 */
                vm_map_t map;
                kern_return_t code;

                /*
                 * Get the current thread map. 
                 */
                map = thread->map;

                /*
                 * Attempt to fault the page. 
                 */
                assert(get_preemption_level() == 0);
                __abort_count--;
                code =
                    vm_fault(map, vm_map_trunc_page(dfar),
                             (dfsr & 0x800) ? (VM_PROT_READ | VM_PROT_WRITE)
                             : (VM_PROT_READ), FALSE, THREAD_UNINT, NULL,
                             vm_map_trunc_page(0));
                if ((code != KERN_SUCCESS) && (code != KERN_ABORTED)) {
                    exception_type = EXC_BAD_ACCESS;
                    exception_subcode = 0;

                    /*
                     * Only for debug. 
                     */
                    printf
                        (ANSI_COLOR_RED "%s[%d]: " ANSI_COLOR_BLUE "usermode data abort, EXC_BAD_ACCESS at 0x%08x in map %p (pmap %p) (%s)" ANSI_COLOR_RESET "\n",
                         proc_name_address(thread->task->bsd_info),
                         proc_pid(thread->task->bsd_info), dfar, map, map->pmap,
                         ifsr_to_human(dfsr));
                    printf("Thread has ARM register state:\n"
                           "    r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                           "    r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                           "    r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                           "   r12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                           "  cpsr: 0x%08x\n", arm_ctx->r[0], arm_ctx->r[1],
                           arm_ctx->r[2], arm_ctx->r[3], arm_ctx->r[4],
                           arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7],
                           arm_ctx->r[8], arm_ctx->r[9], arm_ctx->r[10],
                           arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp,
                           arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr);
                    printf("dyld_all_image_info_addr: 0x%08x   dyld_all_image_info_size: 0x%08x\n",
                            thread->task->all_image_info_addr, thread->task->all_image_info_size);
                } else {
                    /*
                     * Retry execution of instruction. 
                     */
                    ml_set_interrupts_enabled(TRUE);
                    return;
                }
                break;
            }
        default:
            exception_type = EXC_BREAKPOINT;
            exception_subcode = 0;
            break;
        }
        /*
         * Unknown mode. 
         */
    } else {
        panic("sleh_abort: Abort in unknown mode, cpsr: 0x%08x\n", cpsr);
    }

    /*
     * If there was a user exception, handle it. 
     */
    if (exception_type) {
        ml_set_interrupts_enabled(TRUE);
        doexception(exception_type, exception_subcode, 0);
    }

    /*
     * Done. 
     */
    return;
}

/**
 * irq_handler
 *
 * Handle irqs and pass them over to the platform expert.
 */
boolean_t irq_handler(void *context)
{
    /*
     * Disable system preemption, dispatch the interrupt and go. 
     */
    __disable_preemption();

    /*
     * Dispatch the interrupt. 
     */
    boolean_t ret = pe_arm_dispatch_interrupt(context);

    /*
     * Go. 
     */
    __enable_preemption();

    return ret;
}

void irq_iokit_dispatch(uint32_t irq)
{
    cpu_data_t *datap = current_cpu_datap();
    if(datap->handler) {
        datap->handler(datap->target, NULL, datap->nub, irq);
    }
}

/**
 * sleh_undef
 *
 * Handle undefined instructions and VFP usage.
 */
void sleh_undef(arm_saved_state_t * state)
{
    uint32_t cpsr, exception_type = 0, exception_subcode = 0;
    arm_saved_state_t *arm_ctx = (arm_saved_state_t *) state;
    thread_t thread = current_thread();

    if (!thread) {
        panic("sleh_undef: current thread is NULL\n");
    }

    /*
     * See if the abort was in Kernel or User mode. 
     */
    cpsr = arm_ctx->cpsr & 0x1F;

    /*
     * Kernel mode. (ARM Supervisor) 
     */
    if (cpsr == 0x13) {
        uint32_t instruction, thumb_offset;

        /*
         * Handle if it's a NEON/VFP instruction. 
         */
        thumb_offset = (state->cpsr & (1 << 5)) ? 1 : 0;
        copyin((uint8_t *) (arm_ctx->pc + thumb_offset), &instruction,
               sizeof(uint32_t));

        /*
         * Check the instruction encoding to see if it's a coprocessor instruction. 
         */
        instruction = OSSwapInt32(instruction);

        /*
         * NEON instruction. 
         */
        if (((OSSwapInt32(instruction) & 0xff100000) == 0xf4000000)
            || ((OSSwapInt32(instruction) & 0xfe000000) == 0xf2000000)) {
            panic_context(0, (void *) arm_ctx,
                          "NEON usage in a kernel mode context. (saved state 0x%08x)\n"
                          "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                          "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                          "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                          "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                          "cpsr: 0x%08x\n", arm_ctx, arm_ctx->r[0], arm_ctx->r[1],
                          arm_ctx->r[2], arm_ctx->r[3], arm_ctx->r[4],
                          arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7],
                          arm_ctx->r[8], arm_ctx->r[9], arm_ctx->r[10],
                          arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp,
                          arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr);
        }

        if ((((instruction & 0xF000000) >> 24) == 0xE)
            || (((instruction & 0xF000000) >> 24) == 0xD)
            || (((instruction & 0xF000000) >> 24) == 0xC)) {
            uint32_t cr = (instruction & 0xF00) >> 8;
            if (cr == 10 || cr == 11) {
                /*
                 * VFP instruction. 
                 */
                panic_context(0, (void *) arm_ctx,
                              "VFP usage in a kernel mode context (saved state 0x%08x)\n"
                              "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                              "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                              "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                              "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                              "cpsr: 0x%08x\n", arm_ctx, arm_ctx->r[0], arm_ctx->r[1],
                              arm_ctx->r[2], arm_ctx->r[3], arm_ctx->r[4],
                              arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7],
                              arm_ctx->r[8], arm_ctx->r[9], arm_ctx->r[10],
                              arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp,
                              arm_ctx->lr, arm_ctx->pc, arm_ctx->cpsr);
            }
        }

        /*
         * Fall through to bad kernel handler. 
         */
        panic_context(0, (void *) arm_ctx,
                      "Kernel undefined instruction. (saved state 0x%08x)\n"
                      "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                      "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                      "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                      "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                      "cpsr: 0x%08x\n", arm_ctx, arm_ctx->r[0], arm_ctx->r[1],
                      arm_ctx->r[2], arm_ctx->r[3], arm_ctx->r[4],
                      arm_ctx->r[5], arm_ctx->r[6], arm_ctx->r[7],
                      arm_ctx->r[8], arm_ctx->r[9], arm_ctx->r[10],
                      arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp, arm_ctx->lr,
                      arm_ctx->pc, arm_ctx->cpsr);
    } else if (cpsr == 0x10) {
        vm_map_t map;
        uint32_t instruction, thumb_offset;
        /*
         * Get the current thread map. 
         */
        map = thread->map;

        /*
         * Get the current instruction. 
         */
        thumb_offset = (arm_ctx->cpsr & (1 << 5)) ? 1 : 0;
        copyin((uint8_t *) (arm_ctx->pc + thumb_offset), &instruction,
               sizeof(uint32_t));

        /* i should really fix this crap proper... */

        /*
         * Check the instruction encoding to see if it's a coprocessor instruction. 
         */
        instruction = OSSwapInt32(instruction);

        /*
         * NEON instruction. 
         */
        if (((OSSwapInt32(instruction) & 0xff100000) == 0xf4000000)
            || ((OSSwapInt32(instruction) & 0xfe000000) == 0xf2000000)
            || (((instruction) & 0xff100000) == 0xf9000000)) {
            /*
             * NEON instruction. 
             */
            thread->machine.vfp_dirty = 0;
            if (!thread->machine.vfp_enable) {
                /*
                 * Enable VFP. 
                 */
                vfp_enable_exception(TRUE);
                vfp_context_load(&thread->machine.vfp_regs);
                /*
                 * Continue user execution. 
                 */
                thread->machine.vfp_enable = TRUE;
            }
            return;
        }

        /*
         * VFP instruction.. 
         */
        if ((((instruction & 0xF000000) >> 24) == 0xE)
            || (((instruction & 0xF000000) >> 24) == 0xD)
            || (((instruction & 0xF000000) >> 24) == 0xC)) {
            uint32_t cr = (instruction & 0xF00) >> 8;
            if (cr == 10 || cr == 11) {
                /*
                 * VFP instruction. 
                 */
                thread->machine.vfp_dirty = 0;
                if (!thread->machine.vfp_enable) {
                    /*
                     * Enable VFP. 
                     */
                    vfp_enable_exception(TRUE);
                    vfp_context_load(&thread->machine.vfp_regs);
                    /*
                     * Continue user execution. 
                     */
                    thread->machine.vfp_enable = TRUE;
                }
                return;
            }
        }

        /*
         * Try one last time.
         */
        instruction = OSSwapInt32(instruction);
        if ((((instruction & 0xF000000) >> 24) == 0xE)
            || (((instruction & 0xF000000) >> 24) == 0xD)
            || (((instruction & 0xF000000) >> 24) == 0xC)) {
            uint32_t cr = (instruction & 0xF00) >> 8;
            if (cr == 10 || cr == 11) {
                /*
                 * VFP instruction. 
                 */
                thread->machine.vfp_dirty = 0;
                if (!thread->machine.vfp_enable) {
                    /*
                     * Enable VFP. 
                     */
                    vfp_enable_exception(TRUE);
                    vfp_context_load(&thread->machine.vfp_regs);
                    /*
                     * Continue user execution. 
                     */
                    thread->machine.vfp_enable = TRUE;
                }
                return;
            }
        }

        printf
            (ANSI_COLOR_RED "%s[%d]: " ANSI_COLOR_GREEN "usermode undefined instruction, EXC_BAD_INSTRUCTION at 0x%08x in map %p (pmap %p)" ANSI_COLOR_RESET "\n",
             proc_name_address(thread->task->bsd_info),
             proc_pid(thread->task->bsd_info), arm_ctx->pc, map, map->pmap);
        printf("Thread has ARM register state:\n"
               "    r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
               "    r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
               "    r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
               "   r12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
               "  cpsr: 0x%08x\n", arm_ctx->r[0], arm_ctx->r[1], arm_ctx->r[2],
               arm_ctx->r[3], arm_ctx->r[4], arm_ctx->r[5], arm_ctx->r[6],
               arm_ctx->r[7], arm_ctx->r[8], arm_ctx->r[9], arm_ctx->r[10],
               arm_ctx->r[11], arm_ctx->r[12], arm_ctx->sp, arm_ctx->lr,
               arm_ctx->pc, arm_ctx->cpsr);
        printf("dyld_all_image_info_addr: 0x%08x   dyld_all_image_info_size: 0x%08x\n",
            thread->task->all_image_info_addr, thread->task->all_image_info_size);

        /*
         * xxx gate 
         */
        exception_type = EXC_BAD_INSTRUCTION;
        exception_subcode = 0;
    } else if (cpsr == 0x17) {
        panic("sleh_undef: undefined instruction in system mode");
    }

    /*
     * If there was a user exception, handle it. 
     */
    if (exception_type) {
        ml_set_interrupts_enabled(TRUE);
        doexception(exception_type, exception_subcode, 0);
    }

    /*
     * Done. 
     */
    return;
}
