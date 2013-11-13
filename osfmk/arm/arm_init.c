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
 * ARM processor init
 */

#include <mach/mach_types.h>
#include <arm/pmap.h>
#include <arm/cpu_data.h>
#include <pexpert/arm/boot.h>
#include <pexpert/arm/protos.h>
#include <arm/armops.h>
#include <arm/misc_protos.h>
#include <kern/startup.h>
#include "proc_reg.h"

extern uint8_t *irqstack;

extern int disableConsoleOutput, serialmode;

/**
 * arm_processor_identify
 *
 * Print out information about the currently executing processor.
 */
void arm_processor_identify(void)
{
    get_cachetype_cp15();
    identify_arm_cpu();
}

/**
 * arm_init
 *
 * Initialize the core ARM subsystems, this routine is called from the
 * boot loader. A basic identity mapping is created in __start, however,
 * arm_vm_init will create new mappings.
 */
void arm_init(boot_args * args)
{
    cpu_data_t *bootProcessorData;
    processor_t bootProcessor;
    uint32_t baMaxMem;
    uint64_t maxMem;
    thread_t thread;

    /*
     * We are in. 
     */
    PE_early_puts("arm_init: starting up\n");

    /*
     * arm_init is only called on processor #0, the others will enter using arm_slave_init. 
     */
    bootProcessor = cpu_processor_alloc(TRUE);
    if (!bootProcessor) {
        panic("cpu_processor_alloc failed\n");
    }

    /*
     * Pin the processor information to CPU #0. 
     */
    PE_early_puts("arm_init: calling cpu_bootstrap\n");
    cpu_bootstrap();

    /*
     * Initialize core processor data. 
     */
    bootProcessorData = current_cpu_datap();

    bootProcessorData->cpu_number = 0;
    bootProcessorData->cpu_active_stack = (vm_offset_t)&irqstack;
    bootProcessorData->cpu_phys_number = 0;
    bootProcessorData->cpu_preemption_level = 1;
    bootProcessorData->cpu_interrupt_level = 0;
    bootProcessorData->cpu_running = 1;
    bootProcessorData->cpu_pending_ast = AST_NONE;

    /*
     * Initialize the core thread subsystem (This sets up a template
     * which will then be used to initialize the rest of the thread
     * system later.)
     *
     * Additionally, this also sets the current kernel thread register
     * to our bootstrap thread.
     */
    PE_early_puts("arm_init: calling thread_bootstrap\n");
    thread_bootstrap();

    /*
     * CPU initialization. 
     */
    PE_early_puts("arm_init: calling cpu_init\n");
    cpu_init();

    /*
     * Mach processor bootstrap. 
     */
    PE_early_puts("arm_init: calling processor_bootstrap\n");
    processor_bootstrap();

    /*
     * Initialize the ARM platform expert. 
     */
    PE_early_puts("arm_init: calling PE_init_platform\n");
    PE_init_platform(FALSE, (void *) args);

    /*
     * Initialize kprintf, but no VM is running yet. 
     */
    PE_init_kprintf(FALSE);

    /*
     * Set maximum memory size based on boot-args. 
     */
    if (!PE_parse_boot_argn("maxmem", &baMaxMem, sizeof(baMaxMem)))
        maxMem = 0;
    else
        maxMem = (uint64_t) baMaxMem *(1024 * 1024);

    /*
     * After this, we'll no longer be using physical mappings created by the bootloader. 
     */
    arm_vm_init(maxMem, args);

    /*
     * Kernel early bootstrap. 
     */
    kernel_early_bootstrap();

    /*
     * PE platform init. 
     */
    PE_init_platform(TRUE, (void *) args);

    /*
     * Enable I+D cache. 
     */
    char tempbuf[16];

    if (PE_parse_boot_argn("-no-cache", tempbuf, sizeof(tempbuf))) {
        kprintf("cache: No caching enabled (I+D).\n");
    } else {
        kprintf("cache: initializing i+dcache ... ");
        cache_initialize();
        kprintf("done\n");
    }

    /*
     * Specify serial mode. 
     */
    serialmode = 0;
    if (PE_parse_boot_argn("serial", &serialmode, sizeof(serialmode))) {
        /*
         * We want a serial keyboard and/or console 
         */
        kprintf("Serial mode specified: %08X\n", serialmode);
    }

    if (serialmode & 1) {
        (void) switch_to_serial_console();
        disableConsoleOutput = FALSE;   /* Allow printfs to happen */
    }

    /*
     * Start system timers. 
     */
    thread = current_thread();
    thread->machine.preempt_count = 1;
    thread->machine.cpu_data = cpu_datap(cpu_number());
    thread->kernel_stack = irqstack;
    timer_start(&thread->system_timer, mach_absolute_time());

    /*
     * Processor identification.
     */
    arm_processor_identify();

    /*
     * VFP/float initialization. 
     */
    init_vfp();

    /*
     * Machine startup. 
     */
    machine_startup();

    /*
     * If we return, something very bad is happening. 
     */
    panic("20:02:14 <DHowett> wwwwwwwat is HAAAAAAAPPENING\n");

    /*
     * Last chance. 
     */
    while (1) ;
}
