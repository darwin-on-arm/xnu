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
 * Platform Expert for Raspberry Pi.
 */

#include <mach/mach_types.h>

#include <pexpert/pexpert.h>
#include <pexpert/arm/protos.h>
#include <pexpert/arm/boot.h>

#include <machine/machine_routines.h>

#include <vm/pmap.h>
#include <arm/pmap.h>

/*
 * This is board specific stuff.
 */
#ifdef BOARD_CONFIG_RASPBERRYPI
#define KPRINTF_PREFIX  "PE_RaspberryPi: "

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

#define uart_base   gRaspberryPiUartBase
vm_offset_t gRaspberryPiUartBase;

static uint64_t clock_decrementer = 0;
static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_absolute_time = 0;

static void timer_configure(void)
{
    return;
}

void RaspberryPi_putc(int c)
{
    return;
}

int RaspberryPi_getc(void)
{
    return 'A';
}

void RaspberryPi_uart_init(void)
{
    return;
}

void RaspberryPi_interrupt_init(void)
{
    return;
}

void RaspberryPi_timebase_init(void)
{
    return;
}

void RaspberryPi_handle_interrupt(void *context)
{
    return;
}

uint64_t RaspberryPi_get_timebase(void)
{
    return 0;
}

uint64_t RaspberryPi_timer_value(void)
{
    return 0;
}

void RaspberryPi_timer_enabled(int enable)
{
    return;
}

/*
 * Stub for printing out to framebuffer.
 */
void vcputc(__unused int l, __unused int u, int c);

static void _fb_putc(int c)
{
    RaspberryPi_putc(c);
}

void RaspberryPi_framebuffer_init(void)
{
    return;
}

void PE_init_SocSupport_raspberrypi(void)
{
    gPESocDispatch.uart_getc = RaspberryPi_getc;
    gPESocDispatch.uart_putc = RaspberryPi_putc;
    gPESocDispatch.uart_init = RaspberryPi_uart_init;

    gPESocDispatch.interrupt_init = RaspberryPi_interrupt_init;
    gPESocDispatch.timebase_init = RaspberryPi_timebase_init;

    gPESocDispatch.get_timebase = RaspberryPi_get_timebase;

    gPESocDispatch.handle_interrupt = RaspberryPi_handle_interrupt;

    gPESocDispatch.timer_value = RaspberryPi_timer_value;
    gPESocDispatch.timer_enabled = RaspberryPi_timer_enabled;

    gPESocDispatch.framebuffer_init = RaspberryPi_framebuffer_init;

    RaspberryPi_framebuffer_init();
    RaspberryPi_uart_init();

    PE_kputc = _fb_putc;        //gPESocDispatch.uart_putc;

}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for RASPBERRYPI\n");
    PE_init_SocSupport_raspberrypi();
}

#endif
