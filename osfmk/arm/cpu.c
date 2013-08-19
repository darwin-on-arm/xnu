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
 * CPU bootstrap, used to create core structures for the boot processor.
 *
 * SMP support coming soon.
 */

#include <kern/kalloc.h>
#include <kern/misc_protos.h>
#include <kern/machine.h>
#include <mach/processor_info.h>
#include <arm/pmap.h>
#include <arm/machine_routines.h>
#include <vm/vm_kern.h>
#include <kern/timer_call.h>
#include <kern/etimer.h>
#include <kern/processor.h>
#include <arm/misc_protos.h>
#include <mach/machine.h>
#include <arm/arch.h>

struct processor	BootProcessor;
cpu_data_t          cpu_data_master;

/**
 * cpu_bootstrap
 *
 * Initialize core processor data for CPU #0 during initialization.
 */
void
cpu_bootstrap(void)
{
    cpu_data_ptr[0] = &cpu_data_master;
    
    cpu_data_master.cpu_this = &cpu_data_master;
    cpu_data_master.cpu_processor = &BootProcessor;
}

/**
 * cpu_init
 *
 * Initialize more core processor data for CPU #0 during initialization.
 */
void
cpu_init(void)
{
	cpu_data_t	*cdp = current_cpu_datap();

	timer_call_initialize_queue(&cdp->rt_timer.queue);
	cdp->rt_timer.deadline = EndOfAllTime;

	cdp->cpu_type = CPU_TYPE_ARM;
#if defined(_ARM_ARCH_7)
	cdp->cpu_subtype = CPU_SUBTYPE_ARM_V7;
#elif defined(_ARM_ARCH_V6)
    cdp->cpu_subtype = CPU_SUBTYPE_ARM_V6;
#else
    cdp->cpu_subtype = CPU_SUBTYPE_ARM_ALL;
#endif

}

/**
 * get_cpu_number
 * 
 * Return the current processor number from the PCB. (Right now since we have
 * no SMP support, we have only 1 processor.
 *
 * When SMP becomes of real importance, we can get the current MPCore number
 * by reading some registers in the CP15 coprocessor.
 */
int get_cpu_number(void)
{
    return 0;
}

/**
 * current_cpu_datap
 * 
 * Return the current processor PCB.
 */
cpu_data_t* current_cpu_datap(void)
{
    int smp_number = get_cpu_number();
    cpu_data_t* current_cpu_data;
    
    if(smp_number == 0)
        return &cpu_data_master;
    
    current_cpu_data = cpu_datap(smp_number);
    if(!current_cpu_data) {
        panic("cpu_data for slot %d is not available yet\n", smp_number);
    }
    
    return current_cpu_data;
}

/**
 * cpu_processor_alloc
 *
 * Allocate a processor_t data structure for the specified processor.
 * The boot processor will always use internal data, since vm won't be
 * initialized to allocate data.
 */
processor_t cpu_processor_alloc(boolean_t is_boot_cpu)
{
	if (is_boot_cpu) {
		return &BootProcessor;
    }

    /* Should now kalloc one since we have VM now */
    
    return NULL;
}

/**
 * current_processor
 *
 * Return the processor_t to the for the current processor.
 */
processor_t current_processor(void)
{
	return current_cpu_datap()->cpu_processor;
}

/**
 * cpu_to_processor
 *
 * Return the procesor_t for a specified processor. Please don't
 * do bad things. 
 *
 * This function needs validation to ensure that the cpu data accessed
 * isn't null/exceeding buffer boundaries. 
 */
processor_t cpu_to_processor(int cpu)
{
    assert(cpu_datap(cpu) != NULL);
	return cpu_datap(cpu)->cpu_processor;
}

/*
 * For sysctl().
 */
cpu_type_t cpu_type(void)
{
	return current_cpu_datap()->cpu_type;
}

cpu_subtype_t cpu_subtype(void)
{
	return current_cpu_datap()->cpu_subtype;
}

cpu_threadtype_t cpu_threadtype(void)
{
	return CPU_THREADTYPE_NONE;
}

ast_t* ast_pending(void)
{
    return &(current_cpu_datap()->cpu_pending_ast);
}

