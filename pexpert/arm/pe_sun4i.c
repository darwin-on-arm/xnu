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
 * Platform Expert for Allwinner A10 devices (Mele A2000/A1000)
 */

#include <mach/mach_types.h>

#include <pexpert/pexpert.h>
#include <pexpert/arm/protos.h>
#include <pexpert/arm/boot.h>

#include <machine/machine_routines.h>

#include <vm/pmap.h>
#include <arm/pmap.h>

#include "sun4i.h"

/*
 * This is board specific stuff.
 */
#ifdef BOARD_CONFIG_SUN4I
#define KPRINTF_PREFIX  "PE_sun4i: "

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

#define uart_base   gSun4iUartBase
vm_offset_t gSun4iUartBase;

static uint64_t clock_decrementer = 0;
static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_absolute_time = 0;

static void timer_configure(void)
{
    return;
}

void Sun4i_putc(int c)
{
    if (!gSun4iUartBase)
        return;

    while (!TX_READY)
        barrier();
    writel(c, UART_THR(UART));
}

int Sun4i_getc(void)
{
    return 'A';
}

void Sun4i_uart_init(void)
{
    gSun4iUartBase = ml_io_map(UART_BASE, PAGE_SIZE);
}

void Sun4i_interrupt_init(void)
{
    return;
}

void Sun4i_timebase_init(void)
{
    return;
}

void Sun4i_handle_interrupt(void *context)
{
    return;
}

uint64_t Sun4i_get_timebase(void)
{
    return 0;
}

uint64_t Sun4i_timer_value(void)
{
    return 0;
}

void Sun4i_timer_enabled(int enable)
{
    return;
}

/*
 * Stub for printing out to framebuffer.
 */
void vcputc(__unused int l, __unused int u, int c);

static void _fb_putc(int c)
{
    Sun4i_putc(c);
}

void Sun4i_framebuffer_init(void)
{
    return;
}

void PE_init_SocSupport_sun4i(void)
{
    gPESocDispatch.uart_getc = Sun4i_getc;
    gPESocDispatch.uart_putc = Sun4i_putc;
    gPESocDispatch.uart_init = Sun4i_uart_init;

    gPESocDispatch.interrupt_init = Sun4i_interrupt_init;
    gPESocDispatch.timebase_init = Sun4i_timebase_init;

    gPESocDispatch.get_timebase = Sun4i_get_timebase;

    gPESocDispatch.handle_interrupt = Sun4i_handle_interrupt;

    gPESocDispatch.timer_value = Sun4i_timer_value;
    gPESocDispatch.timer_enabled = Sun4i_timer_enabled;

    gPESocDispatch.framebuffer_init = Sun4i_framebuffer_init;

    Sun4i_framebuffer_init();
    Sun4i_uart_init();

    PE_kputc = _fb_putc;        //gPESocDispatch.uart_putc;

}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for SUN4I\n");
    PE_init_SocSupport_sun4i();
}

#endif
