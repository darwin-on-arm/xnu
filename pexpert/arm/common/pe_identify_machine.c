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
 * Machine identification routine.
 */

#include <mach/mach_types.h>
#include <pexpert/pexpert.h>
#include <pexpert/machine/protos.h>
#include <pexpert/machine/boot.h>
#include <kern/debug.h>
#include <mach/vm_param.h>

clock_frequency_info_t gPEClockFrequencyInfo;

/*
 * Technically, the pe_arm_stuff should be implemented, but I'm choosing to ignore
 * those for right now. When done, put the crap in pe_identify_machine.
 *
 * Todo:
 *  - pe_arm_get_soc_base_phys
 *  - pe_arm_get_soc_revision
 */

/**
 * pe_identify_machine
 *
 * Fill out machine dependent timer information. Will actually also be refilled
 * during actual SoC dependent init.
 */
void pe_identify_machine(__unused boot_args * args)
{
    PE_early_puts("pe_identify_machine: Dummying out gPEClockFrequencyInfo (for now)\n");

    /*
     * Clear the gPEClockFrequencyInfo struct 
     */
    bzero((void *) &gPEClockFrequencyInfo, sizeof(clock_frequency_info_t));

    /*
     * Start with default values that were blatantly stolen from i386. 
     */
    gPEClockFrequencyInfo.timebase_frequency_hz = 1000000000;
    gPEClockFrequencyInfo.bus_frequency_hz = 100000000;
    gPEClockFrequencyInfo.cpu_frequency_hz = 300000000;

    gPEClockFrequencyInfo.bus_frequency_min_hz = gPEClockFrequencyInfo.bus_frequency_hz;
    gPEClockFrequencyInfo.bus_frequency_max_hz = gPEClockFrequencyInfo.bus_frequency_hz;
    gPEClockFrequencyInfo.cpu_frequency_min_hz = gPEClockFrequencyInfo.cpu_frequency_hz;
    gPEClockFrequencyInfo.cpu_frequency_max_hz = gPEClockFrequencyInfo.cpu_frequency_hz;

    gPEClockFrequencyInfo.dec_clock_rate_hz = gPEClockFrequencyInfo.timebase_frequency_hz;
    gPEClockFrequencyInfo.bus_clock_rate_hz = gPEClockFrequencyInfo.bus_frequency_hz;
    gPEClockFrequencyInfo.cpu_clock_rate_hz = gPEClockFrequencyInfo.cpu_frequency_hz;

    // Fill out the info from DeviceTree.
}
