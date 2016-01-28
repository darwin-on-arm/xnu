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
 * Compatibility support to bridge old pe_arm_xxx api and new
 * AWESOME SOC DISPATCH STUFF.
 */

#include <mach/mach_types.h>

#include <pexpert/pexpert.h>
#include <pexpert/arm/protos.h>
#include <pexpert/arm/boot.h>

#include <machine/machine_routines.h>

#include <kern/debug.h>

/**
 * pe_arm_init_interrupts
 *
 * Initialize the SoC dependent interrrupt controller.
 */
uint32_t pe_arm_init_interrupts(__unused void *args)
{
    if (gPESocDispatch.interrupt_init == NULL)
        panic("gPESocDispatch.interrupt_init was null, did you forget to set up the table?");

    gPESocDispatch.interrupt_init();

    return 0;
}

/**
 * pe_arm_init_timebase
 *
 * Initialize the SoC dependent timebase.
 */
uint32_t pe_arm_init_timebase(__unused void *args)
{
    if (gPESocDispatch.timebase_init == NULL)
        panic("gPESocDispatch.timebase_init was null, did you forget to set up the table?");

    gPESocDispatch.timebase_init();

    return 0;
}

/**
 * pe_arm_dispatch_interrupt
 *
 * Dispatch IRQ requests to the SoC specific handler.
 */
boolean_t pe_arm_dispatch_interrupt(void *context)
{
    if (gPESocDispatch.handle_interrupt == NULL)
        panic("gPESocDispatch.handle_interrupt was null, did you forget to set up the table?");

    gPESocDispatch.handle_interrupt(context);

    return TRUE;
}

/**
 * pe_arm_get_timebase
 *
 * Get current system timebase from the SoC handler.
 */
uint64_t pe_arm_get_timebase(__unused void *args)
{
    if (gPESocDispatch.get_timebase == NULL)
        panic("gPESocDispatch.get_timebase was null, did you forget to set up the table?");

    return gPESocDispatch.get_timebase();
}

/**
 * pe_arm_set_timer_enabled
 *
 * Set platform timer enabled status.
 */
void pe_arm_set_timer_enabled(boolean_t enable)
{
    if (gPESocDispatch.timer_enabled == NULL)
        panic("gPESocDispatch.timer_enabled was null, did you forget to set up the table?");

    gPESocDispatch.timer_enabled(enable);
}

/*
 * iOS like functionality.
 */
uint32_t debug_enabled = 1;

uint32_t PE_i_can_has_debugger(uint32_t * pe_debug)
{
    if (pe_debug) {
        if (debug_enabled)
            *pe_debug = 1;
        else
            *pe_debug = 0;
    }
    return debug_enabled;
}

uint32_t PE_get_security_epoch(void)
{
    return 0;
}
