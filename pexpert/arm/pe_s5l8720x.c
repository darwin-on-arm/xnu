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
 * Platform Expert for Samsung S5L8720X devices.
 *
 * Now includes S5L8920X and S5L8922X!
 */

#if defined(BOARD_CONFIG_S5L8720X)

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
#define KPRINTF_PREFIX  "PE_SamsungS5L: "

#define HwReg(x) *((volatile unsigned long*)(x))

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

#define uart_base   gS5L8720XUartBase
vm_offset_t gS5L8720XUartBase;
vm_offset_t gS5L8720XClockGateBase;

vm_offset_t gS5L8720XPmgrBase;

/* The 8720 has 4 PL192 compatible VICs. */
vm_offset_t gS5L8720XVic0Base;
vm_offset_t gS5L8720XVic1Base;
vm_offset_t gS5L8720XVic2Base;
vm_offset_t gS5L8720XVic3Base;

vm_offset_t gS5L8720XTimerBase;

#ifdef BOARD_CONFIG_S5L8720X
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

void S5L8720X_putc(int c)
{
    return;
}

int S5L8720X_getc(void)
{
    return 'A';
}

void S5L8720X_uart_init(void)
{
    return;
}

void S5L8720X_interrupt_init(void)
{
    return;
}

uint64_t S5L8720X_timer_value(void);
void S5L8720X_timer_enabled(int enable);

void S5L8720X_timebase_init(void)
{
    return;
}

void S5L8720X_handle_interrupt(void *context)
{
    return;
}

uint64_t S5L8720X_get_timebase(void)
{
    uint32_t timestamp;

    if (!clock_initialized)
        return 0;

    timestamp = S5L8720X_timer_value();

    if (timestamp) {
        uint64_t v = clock_absolute_time;
        v += (uint64_t) (((uint64_t) clock_decrementer) - (uint64_t) (timestamp));
        return v;
    } else {
        clock_absolute_time += clock_decrementer;
        return clock_absolute_time;
    }
}

uint64_t S5L8720X_timer_value(void)
{
    uint64_t ret = 0;
    return ret;
}

void S5L8720X_timer_enabled(int enable)
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

void S5L8720X_framebuffer_init(void)
{
    char tempbuf[16];

    /*
     * Technically, iBoot should initialize this.. Haven't bothered
     * to reverse this part properly, if you're using a 16-bit panel, then use 
     * the 'rgb565' boot-argument if you care about a working framebuffer...
     */
    PE_state.video.v_depth = 4 * (8);   // 32bpp
    if (PE_parse_boot_argn("rgb565", tempbuf, sizeof(tempbuf))) {
        PE_state.video.v_depth = 2 * (8);   // 16bpp
    }

    kprintf(KPRINTF_PREFIX "framebuffer initialized\n");

    /*
     * Enable early framebuffer.
     */

    if (PE_parse_boot_argn("-early-fb-debug", tempbuf, sizeof(tempbuf))) {
        initialize_screen((void *) &PE_state.video, kPEAcquireScreen);
    }

    if (PE_parse_boot_argn("-graphics-mode", tempbuf, sizeof(tempbuf))) {
        initialize_screen((void *) &PE_state.video, kPEGraphicsMode);
    } else {
        initialize_screen((void *) &PE_state.video, kPETextMode);
    }
    return;
}

int S5L8720X_halt_restart(int type)
{
    return 0;
}

void PE_init_SocSupport_S5L8720X(void)
{
    gPESocDispatch.uart_getc = S5L8720X_getc;
    gPESocDispatch.uart_putc = S5L8720X_putc;
    gPESocDispatch.uart_init = S5L8720X_uart_init;

    gPESocDispatch.interrupt_init = S5L8720X_interrupt_init;
    gPESocDispatch.timebase_init = S5L8720X_timebase_init;

    gPESocDispatch.get_timebase = S5L8720X_get_timebase;

    gPESocDispatch.handle_interrupt = S5L8720X_handle_interrupt;

    gPESocDispatch.timer_value = S5L8720X_timer_value;
    gPESocDispatch.timer_enabled = S5L8720X_timer_enabled;

    gPESocDispatch.framebuffer_init = S5L8720X_framebuffer_init;

    char tempbuf[16];
    if (PE_parse_boot_argn("-avoid-uarts", tempbuf, sizeof(tempbuf))) {
        avoid_uarts = 1;
    }

    if (PE_parse_boot_argn("-force-uarts", tempbuf, sizeof(tempbuf))) {
        avoid_uarts = 0;
    }

    S5L8720X_framebuffer_init();
    S5L8720X_uart_init();

    PE_halt_restart = S5L8720X_halt_restart;
}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for S5L8720X\n");
    PE_init_SocSupport_S5L8720X();
}

#endif /* !BOARD_CONFIG_S5L8720X */
