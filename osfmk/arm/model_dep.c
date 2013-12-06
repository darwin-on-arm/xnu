/*
 * Copyright (c) 2009 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

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
 * ARM machine startup functions
 */

#include <platforms.h>
#include <mach/arm/vm_param.h>
#include <string.h>
#include <sys/time.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <mach/machine.h>
#include <mach/time_value.h>
#include <kern/spl.h>
#include <kern/assert.h>
#include <kern/debug.h>
#include <kern/misc_protos.h>
#include <kern/startup.h>
#include <kern/clock.h>
#include <kern/cpu_data.h>
#include <kern/machine.h>
#include <arm/pmap.h>

#include <arm/misc_protos.h>

#include <pexpert/pexpert.h>
#include <pexpert/arm/boot.h>

#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>

#include <kern/thread.h>
#include <kern/sched.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

#include <libkern/kernel_mach_header.h>
#include <libkern/OSKextLibPrivate.h>

#include <IOKit/IOPlatformExpert.h>

extern unsigned int panic_is_inited;
extern struct timeval gIOLastSleepTime;
extern struct timeval gIOLastWakeTime;
extern char firmware_version[32];
extern uint32_t debug_enabled;
extern const char version[];
extern char osversion[];
extern boolean_t panicDialogDesired;
extern int enable_timing;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*
 * Frame pointer definition.
 */

typedef struct _cframe_t {
    struct _cframe_t *prev;
    uintptr_t caller;
} cframe_t;

unsigned int nosym = 1;

void print_threads(uint32_t stackptr);
void panic_arm_thread_backtrace(void *_frame, int nframes, const char *msg,
                                boolean_t regdump, arm_saved_state_t * regs,
                                int crashed, char *crashstr);

/**
 * machine_init
 *
 * Machine-specific initialization.
 */
void machine_init(void)
{
    debug_log_init();
    clock_config();
    return;
}

static void machine_conf(void)
{
    machine_info.memory_size = (typeof(machine_info.memory_size)) mem_size;
}

unsigned int debug_boot_arg;

/*
 * Debugger/DebuggerWithContext.
 *
 * Back to plagiarizing from i386.
 */

/* Routines for address - symbol translation. Not called unless the "keepsyms"
 * boot-arg is supplied.
 */

static int
panic_print_macho_symbol_name(kernel_mach_header_t *mh, vm_address_t search, const char *module_name)
{
    kernel_nlist_t  *sym = NULL;
    struct load_command     *cmd;
    kernel_segment_command_t    *orig_ts = NULL, *orig_le = NULL;
    struct symtab_command   *orig_st = NULL;
    unsigned int            i;
    char                    *strings, *bestsym = NULL;
    vm_address_t            bestaddr = 0, diff, curdiff;

    /* Assume that if it's loaded and linked into the kernel, it's a valid Mach-O */
    
    cmd = (struct load_command *) &mh[1];
    for (i = 0; i < mh->ncmds; i++) {
        if (cmd->cmd == LC_SEGMENT_KERNEL) {
            kernel_segment_command_t *orig_sg = (kernel_segment_command_t *) cmd;
            
            if (strncmp(SEG_TEXT, orig_sg->segname,
                    sizeof(orig_sg->segname)) == 0)
                orig_ts = orig_sg;
            else if (strncmp(SEG_LINKEDIT, orig_sg->segname,
                    sizeof(orig_sg->segname)) == 0)
                orig_le = orig_sg;
            else if (strncmp("", orig_sg->segname,
                    sizeof(orig_sg->segname)) == 0)
                orig_ts = orig_sg; /* pre-Lion i386 kexts have a single unnamed segment */
        }
        else if (cmd->cmd == LC_SYMTAB)
            orig_st = (struct symtab_command *) cmd;
        
        cmd = (struct load_command *) ((uintptr_t) cmd + cmd->cmdsize);
    }
    
    if ((orig_ts == NULL) || (orig_st == NULL) || (orig_le == NULL))
        return 0;
    
    if ((search < orig_ts->vmaddr) ||
        (search >= orig_ts->vmaddr + orig_ts->vmsize)) {
        /* search out of range for this mach header */
        return 0;
    }
    
    sym = (kernel_nlist_t *)(uintptr_t)(orig_le->vmaddr + orig_st->symoff - orig_le->fileoff);
    strings = (char *)(uintptr_t)(orig_le->vmaddr + orig_st->stroff - orig_le->fileoff);
    diff = search;
    
    for (i = 0; i < orig_st->nsyms; i++) {
        if (sym[i].n_type & N_STAB) continue;

        if (sym[i].n_value <= search) {
            curdiff = search - (vm_address_t)sym[i].n_value;
            if (curdiff < diff) {
                diff = curdiff;
                bestaddr = sym[i].n_value;
                bestsym = strings + sym[i].n_un.n_strx;
            }
        }
    }
    
    if (bestsym != NULL) {
        if (diff != 0) {
            kdb_printf("%s : %s + 0x%lx", module_name, bestsym, (unsigned long)diff);
        } else {
            kdb_printf("%s : %s", module_name, bestsym);
        }
        return 1;
    }
    return 0;
}

extern kmod_info_t * kmod; /* the list of modules */

static void
panic_print_kmod_symbol_name(vm_address_t search)
{
    u_int i;

    if (gLoadedKextSummaries == NULL)
        return;
    for (i = 0; i < gLoadedKextSummaries->numSummaries; ++i) {
        OSKextLoadedKextSummary *summary = gLoadedKextSummaries->summaries + i;

        if ((search >= summary->address) &&
            (search < (summary->address + summary->size)))
        {
            kernel_mach_header_t *header = (kernel_mach_header_t *)(uintptr_t) summary->address;
            if (panic_print_macho_symbol_name(header, search, summary->name) == 0) {
                kdb_printf("%s + %llu", summary->name, (unsigned long)search - summary->address);
            }
            break;
        }
    }
}

static void
panic_print_symbol_name(vm_address_t search)
{
    /* try searching in the kernel */
    if (panic_print_macho_symbol_name(&_mh_execute_header, search, "mach_kernel") == 0) {
        /* that failed, now try to search for the right kext */
        panic_print_kmod_symbol_name(search);
    }
}

/* Generate a backtrace, given a frame pointer - this routine
 * should walk the stack safely. The trace is appended to the panic log
 * and conditionally, to the console. If the trace contains kernel module
 * addresses, display the module name, load address and dependencies.
 */

#define DUMPFRAMES 32
#define PBT_TIMEOUT_CYCLES (5 * 1000 * 1000 * 1000ULL)
void
panic_arm_backtrace(void *_frame, int nframes, const char *msg, boolean_t regdump, arm_saved_state_t *regs)
{
    cframe_t    *frame = (cframe_t *)_frame;
    vm_offset_t raddrs[DUMPFRAMES];
    vm_offset_t PC = 0;
    int frame_index;
    boolean_t keepsyms = FALSE;
    int cn = cpu_number();

    PE_parse_boot_argn("keepsyms", &keepsyms, sizeof (keepsyms));

    if (msg != NULL) {
        kdb_printf("%s", msg);
    }

    kdb_printf("Backtrace (CPU %d), "
    "Frame : Return Address\n", cn);

    for (frame_index = 0; frame_index < nframes; frame_index++) {
        vm_offset_t curframep = (vm_offset_t) frame;

        if (!curframep)
            break;

        if (curframep & 0x3) {
            kdb_printf("Unaligned frame\n");
            goto invalid;
        }

        if (!kvtophys(curframep) ||
            !kvtophys(curframep + sizeof(cframe_t) - 1)) {
            kdb_printf("No mapping exists for frame pointer\n");
            goto invalid;
        }

        kdb_printf("%p : 0x%lx ", frame, frame->caller);
        if (frame_index < DUMPFRAMES)
            raddrs[frame_index] = frame->caller;

        /* Display address-symbol translation only if the "keepsyms"
         * boot-arg is suppplied, since we unload LINKEDIT otherwise.
         * This routine is potentially unsafe; also, function
         * boundary identification is unreliable after a strip -x.
         *
         * Right now, OSKextRemoveKextBootstrap is nulled out, so it
         * doesn't really matter at the moment. Dofix.
         */
#if 0
        if (keepsyms)
#endif
            panic_print_symbol_name((vm_address_t)frame->caller);
        
        kdb_printf("\n");

        frame = frame->prev;
    }

    if (frame_index >= nframes)
        kdb_printf("\tBacktrace continues...\n");

    goto out;

invalid:
    kdb_printf("Backtrace terminated-invalid frame pointer %p\n",frame);
out:

    /* Identify kernel modules in the backtrace and display their
     * load addresses and dependencies. This routine should walk
     * the kmod list safely.
     */
    if (frame_index)
        kmod_panic_dump((vm_offset_t *)&raddrs[0], frame_index);

    if (PC != 0)
        kmod_panic_dump(&PC, 1);

    panic_display_system_configuration();
}


void
DebuggerWithContext(
    __unused unsigned int   reason,
    __unused void       *ctx,
    const char      *message)
{
    Debugger(message);
}

void
Debugger(
    const char  *message)
{
    unsigned long pi_size = 0;
    void *stackptr;
    int cn = cpu_number();

    hw_atomic_add(&debug_mode, 1);   
    if (!panic_is_inited) {
        /* Halt forever. */
        asm("cpsid if; wfi");
    }

    printf("Debugger called: <%s>\n", message);
    kprintf("Debugger called: <%s>\n", message);

    /*
     * Skip the graphical panic box if no panic string.
     * This is the case if we're being called from
     *   host_reboot(,HOST_REBOOT_DEBUGGER)
     * as a quiet way into the debugger.
     */

    if (panicstr) {
        disable_preemption();

        /* Obtain current frame pointer */
#if defined (__arm__)
        __asm__ volatile("mov %0, r7" : "=r" (stackptr));
#endif

        /* Print backtrace - callee is internally synchronized */
        panic_arm_backtrace(stackptr, 48, NULL, FALSE, NULL);

        /* Draw panic dialog if needed */
        draw_panic_dialog();
    }

    /* Enter KDP if necessary. */
    if(current_debugger)
        __asm__ __volatile__("bkpt #0");

    hw_atomic_sub(&debug_mode, 1);   
}

/**
 * machine_startup
 *
 * Configure core kernel variables and go to Mach kernel bootstrap.
 */
void machine_startup(void)
{
    machine_conf();

    if (PE_parse_boot_argn("debug", &debug_boot_arg, sizeof(debug_boot_arg))) {
        panicDebugging = TRUE;
        if (debug_boot_arg & DB_HALT)
            halt_in_debugger = 1;
        if (debug_boot_arg & DB_PRT)
            disable_debug_output = FALSE;
        if (debug_boot_arg & DB_SLOG)
            systemLogDiags = TRUE;
        if (debug_boot_arg & DB_LOG_PI_SCRN)
            logPanicDataToScreen = TRUE;
    } else {
        debug_boot_arg = 0;
    }

    /*
     * Cause a breakpoint trap to the debugger before proceeding
     * any further if the proper option bit was specified in
     * the boot flags.
     */
    if (halt_in_debugger) {
        Debugger("inline call to debugger(machine_startup)");
        halt_in_debugger = 0;
        active_debugger = 1;
    }

    kernel_bootstrap();
    return;
}

/**
 * machine_boot_info
 *
 * Return string of boot args passed to kernel.
 */
char *machine_boot_info(char *buf, vm_size_t size)
{
    return (PE_boot_args());
}

/**
 * mach_syscall_trace
 */
extern const char *mach_syscall_name_table[];
void mach_syscall_trace(arm_saved_state_t * state)
{

}

/*
 * Halt a cpu.
 */
void halt_cpu(void)
{
    halt_all_cpus(FALSE);
}

int reset_mem_on_reboot = 1;
/*
 * Halt the system or reboot.
 */
void halt_all_cpus(boolean_t reboot)
{
    if (reboot) {
        printf("MACH Reboot\n");
        if (PE_halt_restart)
            (*PE_halt_restart) (kPERestartCPU);
    } else {
        printf("CPU halted\n");
        if (PE_halt_restart)
            (*PE_halt_restart) (kPEHaltCPU);
    }
    while (1) ;
}
