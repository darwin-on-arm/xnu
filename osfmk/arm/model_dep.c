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

extern struct timeval gIOLastSleepTime;
extern struct timeval gIOLastWakeTime;
extern char firmware_version[32];
extern uint32_t debug_enabled;
extern const char version[];
extern char osversion[];

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
void panic_arm_thread_backtrace(void *_frame, int nframes, const char *msg, boolean_t regdump, arm_saved_state_t *regs, int crashed, char* crashstr);

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

/**
 * panic_display_time
 */
void panic_display_time(void)
{
    clock_sec_t secs;
    clock_usec_t usecs;

    /* Header. */
    kdb_printf("Epoch Time:        sec       usec\n");

    /* Boot. */
    clock_get_boottime_nanotime(&secs, &usecs);
    kdb_printf("  Boot    : 0x%08x 0x%08x\n", secs, usecs);
    kdb_printf("  Sleep   : 0x%08x 0x%08x\n", gIOLastSleepTime.tv_sec, gIOLastSleepTime.tv_usec);
    kdb_printf("  Wake    : 0x%08x 0x%08x\n", gIOLastWakeTime.tv_sec, gIOLastWakeTime.tv_usec);

    /* Uptime. */
    clock_get_calendar_microtime(&secs, &secs);
    kdb_printf("  Calendar: 0x%08x 0x%08x\n\n", secs, usecs);

    return;
}

/**
 * panic_backlog
 */
void panic_backlog(uint32_t stackptr)
{
#ifndef __LP64__
    thread_t currthr = current_thread();

    /* Make sure the crash is after we set the first thread. */
    if(!currthr) {
        return;
    }

    task_t task = currthr->task;
    /* If the task is null, return... */
    if(!task) {
       return;
    }

    char* name;

    if (task->bsd_info && (name = proc_name_address(task->bsd_info))) {
        /* */
    } else {
       name = (char*)"unknown task";
    }

    kdb_printf("Panicked task %p: %d pages, %d threads: pid %d: %s\n"
               "panicked thread: %p, backtrace: 0x%08x\n",
               task, task->all_image_info_size, task->thread_count, (task->bsd_info != NULL) ? proc_pid(task->bsd_info) : 0, name,
               currthr, stackptr);
    panic_arm_thread_backtrace(stackptr, 32, NULL, FALSE, NULL, TRUE, "");

    if(panicDebugging)
        print_threads(stackptr);
#endif
}

/**
 * DebuggerCommon
 *
 * The common debugger routine.
 */
void DebuggerCommon(__unused unsigned int reason, void *ctx, const char *message)
{
    /* Dim the screen. */
    dim_screen();

    /* Print out debugger messages. */
    kdb_printf("Debugger message: %s\n", message);
    kdb_printf("OS version: %s\n", (osversion[0] != 0) ? osversion : "Not yet set");
    kdb_printf("Kernel version: %s\n",version);
    kdb_printf("iBoot version: %s\n", firmware_version);
    kdb_printf("secure boot?: %s\n", debug_enabled ? "NO" : "YES");

    /* iOS compatibility :) */
    kdb_printf("Paniclog version: %d\n", 1);

    /* Display panic information. */
    panic_display_time();
    panic_display_zprint();
#if CONFIG_ZLEAKS
    panic_display_ztrace();
#endif /* CONFIG_ZLEAKS */

#ifndef __LP64__
    /* Anyway. */
    uint32_t stackptr;
    arm_saved_state_t st;
    __asm__ __volatile("mov %0, r7" : "=r"(stackptr));

    if(!ctx) {
        bzero(&st, sizeof(st));
        st.r[7] = stackptr;
        panic_backlog(stackptr);

        /* Reboot if not debugging. */
        if(PE_reboot_on_panic() || !panicDebugging) {
	    halt_all_cpus(TRUE);
        }

        /* Go into the debugger with a dummy state. */
        kdp_raise_exception(EXC_BREAKPOINT, 0, 0, &st);
    } else {
        arm_saved_state_t* st = (arm_saved_state_t*)ctx;
        panic_backlog(st->r[7]);

        /* Reboot if not debugging. */
        if(PE_reboot_on_panic() || !panicDebugging) {
            halt_all_cpus(TRUE);
        }

        /* Go into the debugger. */
        kdp_raise_exception(EXC_BREAKPOINT, 0, 0, &ctx);
    }
#endif

} 

/**
 * DebuggerWithContext
 *
 * Calls the debugger/panics the system and prints a backtrace using
 * the provided context's frame pointer.
 */
void DebuggerWithContext(__unused unsigned int reason, void *ctx, const char *message)
{
    hw_atomic_add(&debug_mode, 1);
    DebuggerCommon(reason, ctx, message);
    hw_atomic_sub(&debug_mode, 1);   
    return;
}

/**
 * Debugger
 *
 * Calls the debugger/panics the system and prints a backtrace using
 * the current context's frame pointer.
 */
void Debugger(const char *message)
{
    hw_atomic_add(&debug_mode, 1);
    DebuggerCommon(EXC_BREAKPOINT, NULL, message);
    hw_atomic_sub(&debug_mode, 1);   
    return;
}

/**
 * panic_print_macho_symbol_name
 *
 * Print the symbol name from a specified Mach-O object, if this is the kernel
 * the symbol section may be jettisoned. Use the "keepsyms" boot-arg to prevent
 * that.
 */
static int panic_print_macho_symbol_name(kernel_mach_header_t *mh, vm_address_t search, const char *module_name)
{
    kernel_nlist_t	*sym = NULL;
    struct load_command		*cmd;
    kernel_segment_command_t	*orig_ts = NULL, *orig_le = NULL;
    struct symtab_command	*orig_st = NULL;
    unsigned int			i;
    char					*strings, *bestsym = NULL;
    vm_address_t			bestaddr = 0, diff, curdiff;
    
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
            kdb_printf(" (%s: " ANSI_COLOR_RED "%s" ANSI_COLOR_RESET " + 0x%lx)", module_name, bestsym, (unsigned long)diff);
        } else {
            kdb_printf(" (%s: " ANSI_COLOR_RED "%s" ANSI_COLOR_RESET ")", module_name, bestsym);
        }
        return 1;
    }
    return 0;
}

/**
 * panic_print_symbol_name
 *
 * Attempt to find the symbol name for a specified address. Searches through
 * the kernel and kernel extensions.
 */
static void panic_print_symbol_name(vm_address_t search)
{
    /* try searching in the kernel */
    if (panic_print_macho_symbol_name(&_mh_execute_header, search, "mach_kernel") == 0) {
        /* oh well */
        return;
    }
}

#ifndef __LP64__
/**
 * print_threads
 *
 * Dump all running tasks and threads.
 */
void print_threads(uint32_t stackptr)
{
    if(!kernel_task)
        return;
    
	queue_head_t *task_list = &tasks;
	task_t task = TASK_NULL;
	thread_t thread = THREAD_NULL;
    
    kdb_printf("\n*********  Dumping thread state and stacks *********\n");
    
	queue_iterate(task_list, task, task_t, tasks) {
		char* name;

		if (task->bsd_info && (name = proc_name_address(task->bsd_info))) {
			/* */
		}
		else {
			name = (char*)"unknown task";
		}
        
        kdb_printf("Task 0x%x: %d pages, %d threads: pid %d: %s\n", task, task->all_image_info_size, task->thread_count, (task->bsd_info != NULL) ? proc_pid(task->bsd_info) : 0, name);
        
        queue_iterate(&task->threads, thread, thread_t, task_threads) {
            char crashed[] = ">>>>>>>>";

            kdb_printf("\tthread %p\n", thread);
            assert(thread);

            if(thread->continuation) {
                kdb_printf("%s\tkernel continuation: %p ", ((current_thread() == thread) ? crashed : "\t"), thread->continuation);
                if(!nosym)
                    panic_print_symbol_name(thread->continuation);
                kdb_printf("\n");
            }

            if(stackptr && (current_thread() == thread)) {
                kdb_printf("%s\tkernel backtrace: %x\n", ((current_thread() == thread) ? crashed : "\t"), stackptr);
                panic_arm_thread_backtrace(stackptr, 32, NULL, FALSE, NULL, TRUE, ">>>>>>>>");
                kdb_printf("\n");
                continue;
            }
            
            if(!thread->continuation && thread->machine.iss) {
                kdb_printf("%s\tkernel state:\n", ((current_thread() == thread) ? crashed : "\t"));
                kdb_printf("%s\t  r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                           "%s\t  r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                           "%s\t  r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
                           "%s\t r12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                           "%s\tcpsr: 0x%08x fsr: 0x%08x far: 0x%08x\n",
                           ((current_thread() == thread) ? crashed : "\t"), thread->machine.iss->r[0], thread->machine.iss->r[1], thread->machine.iss->r[2], thread->machine.iss->r[3], 
                           ((current_thread() == thread) ? crashed : "\t"), thread->machine.iss->r[4], thread->machine.iss->r[5], thread->machine.iss->r[6], thread->machine.iss->r[7],
                           ((current_thread() == thread) ? crashed : "\t"), thread->machine.iss->r[8], thread->machine.iss->r[9], thread->machine.iss->r[10], thread->machine.iss->r[11],
                           ((current_thread() == thread) ? crashed : "\t"), thread->machine.iss->r[12], thread->machine.iss->sp, thread->machine.iss->lr, thread->machine.iss->pc,
                           ((current_thread() == thread) ? crashed : "\t"), thread->machine.iss->cpsr, 0, 0);
                if(thread->machine.iss->r[7]) {
                    kdb_printf("%s\tkernel backtrace: %x\n", ((current_thread() == thread) ? crashed : "\t"), thread->machine.iss->r[7]);
                    panic_arm_thread_backtrace(thread->machine.iss->r[7], 32, NULL, FALSE, NULL, ((current_thread() == thread) ? TRUE : FALSE), "\t");
                }
            }

            kdb_printf("\n");
        }
    }
}
#endif

/**
 * panic_arm_backtrace
 *
 * Dump ARM frames for a backtrace. This needs validation for r7 to make sure
 * it isn't corrupted or the addresses go out of range.
 */

#define DUMPFRAMES 32
void panic_arm_backtrace(void *_frame, int nframes, const char *msg, boolean_t regdump, arm_saved_state_t *regs)
{
	int frame_index, i;
    int cpu = cpu_number();
    cframe_t	*frame = (cframe_t *)_frame;
	vm_offset_t raddrs[DUMPFRAMES];
	vm_offset_t PC = 0;
	
    /* 
     * Print out a backtrace of symbols.
     */
    
    if (msg != NULL) {
		kdb_printf("%s", msg);
	}
    
    kdb_printf("\n*** Dumping backtrace from context ***\n");
    
    /*
     * This is probably not thread safe, it's a boilerplate.
     */
    
    kdb_printf("Backtrace (cpu %d, fp 0x%08x):\n", cpu, (uint32_t)_frame);
    
	for (frame_index = 0; frame_index < nframes; frame_index++) {
		vm_offset_t curframep = (vm_offset_t) frame;
        
        if (!kvtophys(curframep) ||
            !kvtophys(curframep + sizeof(cframe_t) - 1)) {
            kdb_printf("No mapping exists for frame pointer\n");
            goto invalid;
        }
        
		if (!curframep)
			break;
        
        if((frame_index >= 1) && (frame == frame->prev)) {
            kdb_printf("Looping frame\n");
            goto invalid;
        }
        
		if (curframep & 0x3) {
			kdb_printf("Unaligned frame\n");
			goto invalid;
		}
        
		kdb_printf("      fp: %p   lr: 0x%lx ", frame, frame->caller);
		if (frame_index < DUMPFRAMES)
			raddrs[frame_index] = frame->caller;
        
        if(!nosym)
            panic_print_symbol_name((vm_address_t)frame->caller);

		kdb_printf("\n");
        
		frame = frame->prev;
    }
    
	if (frame_index >= nframes)
		kdb_printf("    Backtrace continues...\n");
    
	goto out;
    
invalid:
	kdb_printf("Backtrace terminated-invalid frame pointer %p\n",frame);
out:
    
    kdb_printf("Flat caller information: [ ");
    for(i = 0; i < frame_index; i++) {
        kdb_printf("0x%08x ", raddrs[i]);
    }
    kdb_printf("]\n");
    
	panic_display_system_configuration();

}

/* Formatting difference */
#define DUMPFRAMES 32
void panic_arm_thread_backtrace(void *_frame, int nframes, const char *msg, boolean_t regdump, arm_saved_state_t *regs, int crashed, char* crashstr)
{
	int frame_index, i;
    int cpu = cpu_number();
    cframe_t	*frame = (cframe_t *)_frame;
	vm_offset_t raddrs[DUMPFRAMES];
	vm_offset_t PC = 0;
 
    /*
     * Print out a backtrace of symbols.
     */
    
    if (msg != NULL) {
		kdb_printf("%s", msg);
	}
    
    /*
     * This is probably not thread safe, it's a boilerplate.
     */
    
	for (frame_index = 0; frame_index < nframes; frame_index++) {
		vm_offset_t curframep = (vm_offset_t) frame;
        
		if (!curframep)
			break;
        
        if (!kvtophys(curframep) ||
            !kvtophys(curframep + sizeof(cframe_t) - 1)) {
            goto invalid;
        }
        
		if (curframep & 0x3) {
			goto invalid;
		}
        
        if((frame_index >= 1) && (frame == frame->prev)) {
            goto invalid;
        }
        
		kdb_printf("%s\t  lr: 0x%08x  fp: 0x%08x ", (crashed ? crashstr : "\t"), frame->caller, frame);
		if (frame_index < DUMPFRAMES)
			raddrs[frame_index] = frame->caller;
    
        if(!nosym)
            panic_print_symbol_name((vm_address_t)frame->caller);
        
		kdb_printf("\n");
        
		frame = frame->prev;
    }

	goto out;
    
invalid:
out:
    
    if(nosym)
        return;    
}


static void machine_conf(void)
{
	machine_info.memory_size = (typeof(machine_info.memory_size))mem_size;
}

unsigned int debug_boot_arg;

/**
 * machine_startup
 *
 * Configure core kernel variables and go to Mach kernel bootstrap.
 */
void machine_startup(void)
{
    machine_conf();
    
    if (PE_parse_boot_argn("debug", &debug_boot_arg, sizeof (debug_boot_arg))) {
        panicDebugging = TRUE;
        if (debug_boot_arg & DB_HALT) halt_in_debugger=1;
        if (debug_boot_arg & DB_PRT) disable_debug_output=FALSE; 
        if (debug_boot_arg & DB_SLOG) systemLogDiags=TRUE; 
        if (debug_boot_arg & DB_LOG_PI_SCRN) logPanicDataToScreen=TRUE;
    } else {
        debug_boot_arg = 0;
    }

    char nosym_temp[16];
    if (PE_parse_boot_argn("symbolicate-panics", &nosym_temp, sizeof (nosym_temp))) {
        nosym = 0;
    }

    /*
     * Cause a breakpoint trap to the debugger before proceeding
     * any further if the proper option bit was specified in
     * the boot flags.
     */
     if (halt_in_debugger) {
        Debugger("inline call to debugger(machine_startup)");
        halt_in_debugger = 0;
        active_debugger =1;
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
	return(PE_boot_args());
}

/**
 * mach_syscall_trace
 */
extern const char *mach_syscall_name_table[];
void mach_syscall_trace(arm_saved_state_t* state)
{
    int num = -(state->r[12]);
    kdb_printf("MACH Trap: (%d/%s)\n",  num, mach_syscall_name_table[num]);

#if 0
    int num = -(state->r[12]);
    kdb_printf("MACH Trap: (%d/%s)\n"
            "r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
            "r4: 0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
            "r8: 0x%08x  r9: 0x%08x r10: 0x%08x r11: 0x%08x\n"
            "12: 0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
            "cpsr: 0x%08x\n", num, mach_syscall_name_table[num],
            state->r[0], state->r[1], state->r[2], state->r[3],
            state->r[4], state->r[5], state->r[6], state->r[7],
            state->r[8], state->r[9], state->r[10], state->r[11],
            state->r[12], state->sp, state->lr, state->pc, state->cpsr);
#endif
}


/*
 * Halt a cpu.
 */
void
halt_cpu(void)
{
    halt_all_cpus(FALSE);
}

int reset_mem_on_reboot = 1;

/*
 * Halt the system or reboot.
 */
void
halt_all_cpus(boolean_t reboot)
{
    if (reboot) {
        printf("MACH Reboot\n");
        if (PE_halt_restart)
                (*PE_halt_restart)(kPERestartCPU);
    } else {
        printf("CPU halted\n");
        if (PE_halt_restart)
                (*PE_halt_restart)(kPEHaltCPU);
    }
    while(1);
}

