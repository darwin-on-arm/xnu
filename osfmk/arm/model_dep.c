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

void print_threads(void);
void panic_arm_thread_backtrace(void *_frame, int nframes, const char *msg, boolean_t regdump, arm_saved_state_t *regs);

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
 * DebuggerWithContext
 *
 * Calls the debugger/panics the system and prints a backtrace using
 * the provided context's frame pointer.
 */
void DebuggerWithContext(__unused unsigned int reason, void *ctx, const char *message)
{
	void *stackptr;
    
    dim_screen();
    
	kdb_printf("============================================\n"
               "= Debugger (Context) called: <%s>\n"
               "============================================\n", message);
    
    hw_atomic_add(&debug_mode, 1);
    
#ifndef __LP64__
    print_threads();
    
    /* Disable preemption, and dump information. */
	if (panicstr) {
        ml_set_interrupts_enabled(FALSE);
		disable_preemption();
        if(ctx) {
            abort_information_context_t* abort_context = (abort_information_context_t*)ctx;
            panic_arm_backtrace(abort_context->gprs[7], 20, NULL, FALSE, NULL);
        }
    }
#endif

    kprintf("Debugger: We are hanging here.\n");
    while(1) {};
        
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
	void *stackptr;
    
    dim_screen();
    
	kdb_printf("============================================\n"
               "= Debugger called: <%s>\n"
               "============================================\n", message);

    hw_atomic_add(&debug_mode, 1);
    
    /* Disable preemption, and dump information. */
    ml_set_interrupts_enabled(FALSE);

	if (panicstr) {
		disable_preemption();
    }

#ifndef __LP64__
    print_threads();
    
    /* Just print a backtrace anyways, useful for bringup. */
    __asm__ __volatile("mov %0, r7" : "=r"(stackptr));
    panic_arm_backtrace(stackptr, 20, NULL, FALSE, NULL);
#endif

    kprintf("Debugger: We are hanging here.\n\n");
    kprintf(ANSI_COLOR_YELLOW "for @b3ll: aelins!" ANSI_COLOR_RESET "\n");
    
    while(1) {};
    
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
void print_threads(void)
{
    if(!kernel_task)
        return;
    
	queue_head_t *task_list = &tasks;
	task_t task = TASK_NULL;
	thread_t thread = THREAD_NULL;
    
    kprintf("\n*** Dumping all tasks and threads ***\n");
    
	queue_iterate(task_list, task, task_t, tasks) {
		char* name;

		if (task->bsd_info && (name = proc_name_address(task->bsd_info))) {
			/* */
		}
		else {
			name = (char*)"<unknown>";
		}
        
        queue_iterate(&task->threads, thread, thread_t, task_threads) {
            
            kprintf("\ntask %p, thread %p, task_name: \"%s\"\n", thread, task, name);
        
            assert(thread);
            
            if(thread->continuation) {
                kprintf("      kernel continuation: %p ", thread->continuation);
                panic_print_symbol_name(thread->continuation);
                kprintf("\n");
            }
            
            if(!thread->continuation && thread->machine.iss) {
                char crash_string[] = ANSI_COLOR_GREEN "Crashed ";
                
                if(!thread->continuation) {
                    kprintf("    %sThread has ARM register state (kernel, savearea %p, kstack 0x%08x)%s:\n",
                            (current_thread() == thread) ? crash_string : "",
                            thread->machine.iss, thread->kernel_stack,
                            ANSI_COLOR_RESET
                            );
                    kprintf("      r0:  0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n"
                            "      r4:  0x%08x  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x\n"
                            "      r8:  0x%08x  r9: 0x%08x  10: 0x%08x  11: 0x%08x\n"
                            "      12:  0x%08x  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n"
                            "    cpsr:  0x%08x\n",
                            thread->machine.iss->r[0],
                            thread->machine.iss->r[1],
                            thread->machine.iss->r[2],
                            thread->machine.iss->r[3],
                            thread->machine.iss->r[4],
                            thread->machine.iss->r[5],
                            thread->machine.iss->r[6],
                            thread->machine.iss->r[7],
                            thread->machine.iss->r[8],
                            thread->machine.iss->r[9],
                            thread->machine.iss->r[10],
                            thread->machine.iss->r[11],
                            thread->machine.iss->r[12],
                            thread->machine.iss->sp,
                            thread->machine.iss->lr,
                            thread->machine.iss->pc, thread->machine.iss->cpsr
                            );
                    panic_arm_thread_backtrace(thread->machine.iss->r[7], 20, NULL, FALSE, NULL);
                }
            }
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
    
    kprintf("\n*** Dumping backtrace from context ***\n");
    
    /*
     * This is probably not thread safe, it's a boilerplate.
     */
    
    kdb_printf("Backtrace (cpu %d, fp 0x%08x):\n", cpu, (uint32_t)_frame);
    
	for (frame_index = 0; frame_index < nframes; frame_index++) {
		vm_offset_t curframep = (vm_offset_t) frame;
        
        if(frame <= VM_MIN_KERNEL_ADDRESS)
            break;
        
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
void panic_arm_thread_backtrace(void *_frame, int nframes, const char *msg, boolean_t regdump, arm_saved_state_t *regs)
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
    
    kdb_printf("          Backtrace (cpu %d, fp 0x%08x):\n", cpu, (uint32_t)_frame);
    
	for (frame_index = 0; frame_index < nframes; frame_index++) {
		vm_offset_t curframep = (vm_offset_t) frame;
        
        if(frame <= VM_MIN_KERNEL_ADDRESS)
            break;
        
		if (!curframep)
			break;
        
		if (curframep & 0x3) {
			kdb_printf("          Unaligned frame\n");
			goto invalid;
		}
        
        if((frame_index >= 1) && (frame == frame->prev)) {
            kdb_printf("          Looping frame\n");
            goto invalid;
        }
        
		kdb_printf("                fp: %p   lr: 0x%lx ", frame, frame->caller);
		if (frame_index < DUMPFRAMES)
			raddrs[frame_index] = frame->caller;
        
        panic_print_symbol_name((vm_address_t)frame->caller);
        
		kdb_printf("\n");
        
		frame = frame->prev;
    }
    
	if (frame_index >= nframes)
		kdb_printf("          Backtrace continues...\n");
    
	goto out;
    
invalid:
	kdb_printf("          Backtrace terminated-invalid frame pointer %p\n",frame);
out:
    
    kdb_printf("          Flat caller information: [ ");
    for(i = 0; i < frame_index; i++) {
        kdb_printf("0x%08x ", raddrs[i]);
    }
    kdb_printf("]\n");
    
}


static void machine_conf(void)
{
	machine_info.memory_size = (typeof(machine_info.memory_size))mem_size;
}

/**
 * machine_startup
 *
 * Configure core kernel variables and go to Mach kernel bootstrap.
 */
void machine_startup(void)
{
    machine_conf();
    
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
#if 0
    int num = -(state->r[12]);
    kprintf("MACH Trap: (%d/%s)\n"
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

