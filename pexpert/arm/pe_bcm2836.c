/*
 * Copyright 2015, Brian McKenzie <mckenzba@gmail.com>
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
 * Platform expert for BCM2836.
 */

#if defined(BOARD_CONFIG_RASPBERRYPI2)

#include <vm/pmap.h>
#include <arm/pmap.h>

#include <mach/mach_types.h>

#include <pexpert/pexpert.h>
#include <pexpert/arm/protos.h>
#include <pexpert/arm/boot.h>

#include <machine/machine_routines.h>

#include "pe_bcm2836.h"

#define KPRINTF_PREFIX  "PE_BCM2836: "

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_decrementer = 0;
static uint64_t clock_absolute_time = 0;

static void timer_configure(void)
{
}

void bcm2836_putc(int c)
{
    return;
}

int bcm2836_getc(void)
{
    int c;
    return c;
}

void bcm2836_uart_init(void)
{
    return;
}

void bcm2836_interrupt_init(void)
{
    return;
}

void bcm2836_timebase_init(void)
{
    return;
}

void bcm2836_handle_interrupt(void *context)
{
    return;
}

uint64_t bcm2836_get_timebase(void)
{
    return;
}

uint64_t bcm2836_timer_value(void)
{
    return;
}

void bcm2836_timer_enabled(int enable)
{
    return;
}

/*
 * Stub for printing out to framebuffer.
 */
void vcputc(__unused int l, __unused int u, int c);

static void _fb_putc(int c)
{
    if (c == '\n') {
        _fb_putc('\r');
    }
    vcputc(0, 0, c);
    bcm2836_putc(c);
}

void bcm2836_framebuffer_init(void)
{
    return;
}


/*
 * Setup the BCM2836 SoC dispatch table
 */
void PE_init_SocSupport_bcm2836(void)
{
    gPESocDispatch.uart_getc = bcm2836_getc;
    gPESocDispatch.uart_putc = bcm2836_putc;
    gPESocDispatch.uart_init = bcm2836_uart_init;

    gPESocDispatch.interrupt_init = bcm2836_interrupt_init;
    gPESocDispatch.timebase_init = bcm2836_timebase_init;

    gPESocDispatch.get_timebase = bcm2836_get_timebase;

    gPESocDispatch.handle_interrupt = bcm2836_handle_interrupt;

    gPESocDispatch.timer_value = bcm2836_timer_value;
    gPESocDispatch.timer_enabled = bcm2836_timer_enabled;

    gPESocDispatch.framebuffer_init = bcm2836_framebuffer_init;

    bcm2836_uart_init();
    bcm2836_framebuffer_init();

}

/*
 * Initialize SoC support for BCM2836.
 */
void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for Broadcom BCM2836\n");
    PE_init_SocSupport_bcm2836();
}

#endif /* !BOARD_CONFIG_RASPBERRYPI2 */
