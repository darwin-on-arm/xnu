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

#include <mach/mach_types.h>

#include <IOKit/IOPlatformExpert.h>

#include <pexpert/pexpert.h>
#include <pexpert/arm/protos.h>
#include <pexpert/arm/boot.h>

#include <machine/machine_routines.h>

#include <vm/pmap.h>
#include <arm/pmap.h>

/*
 * This is board specific stuff.
 */
#if defined(BOARD_CONFIG_MSM8960_TOUCHPAD)
#define KPRINTF_PREFIX  "PE_TouchPad: "

#define HwReg(x) *((volatile unsigned long*)(x))

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

#define uart_base   gTouchPadUartBase
vm_offset_t gTouchPadUartBase;
vm_offset_t gTouchPadClockGateBase;

vm_offset_t gTouchPadPmgrBase;

/* The 8720 has 4 PL192 compatible VICs. */
vm_offset_t gTouchPadVic0Base;
vm_offset_t gTouchPadVic1Base;
vm_offset_t gTouchPadVic2Base;
vm_offset_t gTouchPadVic3Base;

vm_offset_t gTouchPadTimerBase;

#ifdef BOARD_CONFIG_TouchPad
static boolean_t avoid_uarts = FALSE;
#else
/* Busted... */
static boolean_t avoid_uarts = TRUE;
#endif

static uint64_t clock_decrementer = 0;
static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_absolute_time = 0;

static void timer_configure(void)
{
    return;
}

void TouchPad_putc(int c)
{
    return;
}

int TouchPad_getc(void)
{
    return 'A';
}

void TouchPad_uart_init(void)
{
    return;
}

void TouchPad_interrupt_init(void)
{
    return;
}

uint64_t TouchPad_timer_value(void);
void TouchPad_timer_enabled(int enable);

void TouchPad_timebase_init(void)
{
    return;
}

void TouchPad_handle_interrupt(void *context)
{
    return;
}

uint64_t TouchPad_get_timebase(void)
{
    uint32_t timestamp;

    if (!clock_initialized)
        return 0;

    timestamp = TouchPad_timer_value();

    if (timestamp) {
        uint64_t v = clock_absolute_time;
        v += (uint64_t) (((uint64_t) clock_decrementer) - (uint64_t) (timestamp));
        return v;
    } else {
        clock_absolute_time += clock_decrementer;
        return clock_absolute_time;
    }
}

uint64_t TouchPad_timer_value(void)
{
    uint64_t ret = 0;
    return ret;
}

void TouchPad_timer_enabled(int enable)
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
        vcputc(0, 0, '\r');
    }
    vcputc(0, 0, c);
}

void TouchPad_framebuffer_init(void)
{
    char tempbuf[16]; 

    uint32_t lcd_width = 1024;
    uint32_t lcd_height = 768;

    PE_state.video.v_baseAddr = (unsigned long) 0x7f600000;
    PE_state.video.v_rowBytes = lcd_width * 4;
    PE_state.video.v_width = lcd_width;
    PE_state.video.v_height = lcd_height;
    PE_state.video.v_depth = 4 * (8);   // 32bpp
    kprintf(KPRINTF_PREFIX "framebuffer initialized\n");

    /*
     * Enable early framebuffer.
     */

    initialize_screen((void *) &PE_state.video, kPEAcquireScreen);
    initialize_screen((void *) &PE_state.video, kPETextMode);

    return;
}

int TouchPad_halt_restart(int type)
{
    return 0;
}

void PE_init_SocSupport_TouchPad(void)
{
    gPESocDispatch.uart_getc = TouchPad_getc;
    gPESocDispatch.uart_putc = TouchPad_putc;
    gPESocDispatch.uart_init = TouchPad_uart_init;

    gPESocDispatch.interrupt_init = TouchPad_interrupt_init;
    gPESocDispatch.timebase_init = TouchPad_timebase_init;

    gPESocDispatch.get_timebase = TouchPad_get_timebase;

    gPESocDispatch.handle_interrupt = TouchPad_handle_interrupt;

    gPESocDispatch.timer_value = TouchPad_timer_value;
    gPESocDispatch.timer_enabled = TouchPad_timer_enabled;

    gPESocDispatch.framebuffer_init = TouchPad_framebuffer_init;

    char tempbuf[16];
    if (PE_parse_boot_argn("-avoid-uarts", tempbuf, sizeof(tempbuf))) {
        avoid_uarts = 1;
    }

    if (PE_parse_boot_argn("-force-uarts", tempbuf, sizeof(tempbuf))) {
        avoid_uarts = 0;
    }

    TouchPad_framebuffer_init();
    TouchPad_uart_init();

    PE_halt_restart = TouchPad_halt_restart;
}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for TouchPad\n");
    PE_init_SocSupport_TouchPad();
}

#endif
