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
 * ARM commpage support. Time is still broken though, willfix.
 */

#include <mach/mach_types.h>
#include <mach/machine.h>
#include <mach/vm_map.h>
#include <machine/commpage.h>
#include <machine/pmap.h>
#include <kern/processor.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <machine/cpu_capabilities.h>

void *common_page_ptr = NULL;
decl_simple_lock_data(static,commpage_active_cpus_lock)

void commpage_update_active_cpus(void)
{
    if(!common_page_ptr)
        return;

    /* Lock the lock and update the global page value */
    simple_lock(&commpage_active_cpus_lock);
    *(uint32_t*)_COMMPAGE_NUMBER_OF_CPUS = processor_avail_count;

    /* Done. */
    simple_unlock(&commpage_active_cpus_lock);
}

void commpage_populate(void)
{
    /* Map the commonpage first. */
    pmap_create_sharedpage();

    /* Set the commonpage PtrValue. */
    common_page_ptr = (void*)_COMM_PAGE_BASE_ADDRESS;

    /* Start stuffing things into the commonpage. */
    *(uint32_t*)_COMMPAGE_CPUFAMILY = CPUFAMILY_ARM_13;	    /* Cortex-A8 */
    *(uint16_t*)_COMMPAGE_MYSTERY_VALUE = 3;                /* It's a mystery! */
    *(uint32_t*)_COMMPAGE_CPU_CAPABILITIES = kUP;           /* No capabilities, UP system. */

    /* Update CPU count */
    simple_lock_init(&commpage_active_cpus_lock, 0);
    commpage_update_active_cpus();

    if(*(uint32_t*)_COMMPAGE_NUMBER_OF_CPUS == 0) {
    	panic("commpage number_of_cpus == 0");
    }

    return;
}

void commpage_set_timestamp(uint64_t tbr, uint64_t secs, uint32_t ticks_per_sec)
{
    assert(common_page_ptr);

    /* Update the timestamp value. */
    commpage_timeofday_data_t* tofd = (commpage_timeofday_data_t*)_COMMPAGE_TIMEBASE_INFO;

    tofd->TimeBase = tbr;
    tofd->TimeStamp_sec = secs;
    tofd->TimeBaseTicks_per_sec = 0;

    return;
}
