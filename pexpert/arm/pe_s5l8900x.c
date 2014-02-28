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
 * Platform Expert for Samsung S5L8900X devices.
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
#if defined(BOARD_CONFIG_S5L8900XRB)
#define KPRINTF_PREFIX  "PE_SamsungS5L: "

#define HwReg(x) *((volatile unsigned long*)(x))

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

#define uart_base   gS5L8900XUartBase
vm_offset_t gS5L8900XUartBase;
vm_offset_t gS5L8900XClockGateBase;

vm_offset_t gS5L8900XPmgrBase;

/* The 8900 has 2 PL192 compatible VICs. */
vm_offset_t gS5L8900XVic0Base;
vm_offset_t gS5L8900XVic1Base;
vm_offset_t gS5L8900XEdgeICBase;

vm_offset_t gS5L8900XTimerBase;

static boolean_t avoid_uarts = FALSE;

static uint64_t clock_decrementer = 0;
static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_absolute_time = 0;

#define barrier()               __asm__ __volatile__("": : :"memory");

#define EDGEIC 0x38E02000
#define EDGEICCONFIG0 0x0
#define EDGEICCONFIG1 0x4
#define EDGEICLOWSTATUS 0x8
#define EDGEICHIGHSTATUS 0xC

#define VIC0 0x38E00000
#define VIC1 0x38E01000
#define VIC_INTSEP  0x20
#define VICIRQSTATUS 0x0
#define VICRAWINTR 0x8
#define VICINTSELECT 0xC
#define VICINTENABLE 0x10
#define VICINTENCLEAR 0x14
#define VICSWPRIORITYMASK 0x24
#define VICVECTADDRS 0x100
#define VICADDRESS 0xF00
#define VICPERIPHID0 0xFE0
#define VICPERIPHID1 0xFE4
#define VICPERIPHID2 0xFE8
#define VICPERIPHID3 0xFEC

#define TIMER_BASE      0x3E200000

#define TM64_COUNTHIGH  0x80
#define TM64_COUNTLOW   0x84
#define TM64_CONTROL    0x88
#define TM64_DATA0_H    0x8C
#define TM64_DATA0_L    0x90
#define TM64_DATA1_H    0x94
#define TM64_DATA1_L    0x98

#define TIMER_CONTROL   0xA0 + 0x0
#define TIMER_COMMAND   0xA0 + 0x4
#define TIMER_DATA0     0xA0 + 0x8
#define TIMER_DATA1     0xA0 + 0xC
#define TIMER_PRESCALER 0xA0 + 0x10
#define TIMER_COUNTER   0xA0 + 0x14

#define TIMER_IRQLATCH  0xF8

#define TIMER_STATE_START  1
#define TIMER_STATE_STOP   0

static void timer_configure(void)
{
    /*
     * DUMMY 
     */
    uint64_t hz = 12000000;
    gPEClockFrequencyInfo.timebase_frequency_hz = hz;

    clock_decrementer = 5000;
    kprintf(KPRINTF_PREFIX "decrementer frequency = %llu\n", clock_decrementer);

    rtc_configure(hz);
    return;
}

void S5L8900X_putc(int c)
{
    return;
}

int S5L8900X_getc(void)
{
    return 'A';
}

void S5L8900X_uart_init(void)
{
    /* Map the VICs. */
    gS5L8900XVic0Base = ml_io_map(VIC0, PAGE_SIZE);
    gS5L8900XVic1Base = ml_io_map(VIC1, PAGE_SIZE);
    gS5L8900XEdgeICBase = ml_io_map(EDGEIC, PAGE_SIZE);
    gS5L8900XTimerBase = ml_io_map(TIMER_BASE, PAGE_SIZE);
    return;
}

void S5L8900X_interrupt_init(void)
{
    assert(gS5L8900XVic0Base && gS5L8900XVic1Base && gS5L8900XEdgeICBase);

    /* 
     * Reset the EdgeIC.
     */
    HwReg(gS5L8900XEdgeICBase + EDGEICCONFIG0) = 0;
    HwReg(gS5L8900XEdgeICBase + EDGEICCONFIG1) = 0;

    /*
     * Disable all interrupts. 
     */
    HwReg(gS5L8900XVic0Base + VICINTENCLEAR) = 0xFFFFFFFF;
    HwReg(gS5L8900XVic1Base + VICINTENCLEAR) = 0xFFFFFFFF;

    HwReg(gS5L8900XVic0Base + VICINTENABLE) = 0;
    HwReg(gS5L8900XVic1Base + VICINTENABLE) = 0;

    /*
     * Please use IRQs. I don't want to implement a FIQ based timer decrementer handler. 
     */
    HwReg(gS5L8900XVic0Base + VICINTSELECT) = 0;
    HwReg(gS5L8900XVic1Base + VICINTSELECT) = 0;

    /*
     * Unmask all interrupt levels. 
     */
    HwReg(gS5L8900XVic0Base + VICSWPRIORITYMASK) = 0xFFFF;
    HwReg(gS5L8900XVic1Base + VICSWPRIORITYMASK) = 0xFFFF;

    /*
     * Set vector addresses to interrupt numbers. 
     */
    int i;
    for (i = 0; i < 0x20; i++) {
        HwReg(gS5L8900XVic0Base + VICVECTADDRS + (i * 4)) = (0x20 * 0) + i;
        HwReg(gS5L8900XVic1Base + VICVECTADDRS + (i * 4)) = (0x20 * 1) + i;
    }

    return;
}

uint64_t S5L8900X_timer_value(void);
void S5L8900X_timer_enabled(int enable);

void S5L8900X_timebase_init(void)
{
    assert(gS5L8900XTimerBase);

    /*
     * Set rtclock stuff 
     */
    timer_configure();

    /*
     * Disable the timer. 
     */
    S5L8900X_timer_enabled(FALSE);

    /*
     * Enable the interrupt. 
     */
    HwReg(gS5L8900XVic0Base + VICINTENABLE) = HwReg(gS5L8900XVic0Base + VICINTENABLE) | (1 << 7) | (1 << 6) | (1 << 5);

    /*
     * Enable interrupts. 
     */
    ml_set_interrupts_enabled(TRUE);

    /*
     * Wait for it. 
     */
    kprintf(KPRINTF_PREFIX "waiting for system timer to come up...\n");
    S5L8900X_timer_enabled(TRUE);

    clock_initialized = TRUE;

    while (!clock_had_irq)
        barrier();

    return;
}

void S5L8900X_handle_interrupt(void *context)
{
    uint32_t current_irq = HwReg(gS5L8900XVic0Base + VICADDRESS);

    /*
     * Timer IRQs are handled by us. 
     */
    if (current_irq == 0x5) {
        /*
         * Disable timer 
         */
        S5L8900X_timer_enabled(FALSE);

        /*
         * Update absolute time 
         */
        clock_absolute_time += clock_decrementer;

        /*
         * Resynchronize deadlines. 
         */
        rtclock_intr((arm_saved_state_t *) context);

        /*
         * EOI. 
         */
        HwReg(gS5L8900XVic0Base + VICADDRESS) = 0;

        /*
         * Enable timer. 
         */
        S5L8900X_timer_enabled(TRUE);

        /*
         * We had an IRQ. 
         */
        clock_had_irq = TRUE;
    } else {
        irq_iokit_dispatch(current_irq);
    }

    return;
}

uint64_t S5L8900X_get_timebase(void)
{
    uint32_t timestamp;

    if (!clock_initialized)
        return 0;

    timestamp = S5L8900X_timer_value();

    if (timestamp) {
        uint64_t v = clock_absolute_time;
        v += (uint64_t) (((uint64_t) clock_decrementer) - (uint64_t) (timestamp));
        return v;
    } else {
        clock_absolute_time += clock_decrementer;
        return clock_absolute_time;
    }
}

uint64_t S5L8900X_timer_value(void)
{
    /* The 8900 does not have a true overflow timer, like the other platforms...? */
    uint64_t ret = 0;
    return ret;
}

void S5L8900X_timer_enabled(int enable)
{
    if(enable) {
        HwReg(gS5L8900XTimerBase + TIMER_COUNTER) = TIMER_STATE_START;
        HwReg(gS5L8900XTimerBase + TIMER_CONTROL) = 0x7000 | (1 << 6); /* IRQ enable. */
        HwReg(gS5L8900XTimerBase + TIMER_DATA0) = clock_decrementer; /* Decrementer. */
    } else {
        HwReg(gS5L8900XTimerBase + TIMER_COUNTER) = TIMER_STATE_STOP;
    }

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

void S5L8900X_framebuffer_init(void)
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

    //if (PE_parse_boot_argn("-early-fb-debug", tempbuf, sizeof(tempbuf))) {
        initialize_screen((void *) &PE_state.video, kPEAcquireScreen);
    //}

    if (PE_parse_boot_argn("-graphics-mode", tempbuf, sizeof(tempbuf))) {
        initialize_screen((void *) &PE_state.video, kPEGraphicsMode);
    } else {
        initialize_screen((void *) &PE_state.video, kPETextMode);
    }
    return;
}

int S5L8900X_halt_restart(int type)
{
    return 0;
}

void PE_init_SocSupport_S5L8900X(void)
{
    gPESocDispatch.uart_getc = S5L8900X_getc;
    gPESocDispatch.uart_putc = S5L8900X_putc;
    gPESocDispatch.uart_init = S5L8900X_uart_init;

    gPESocDispatch.interrupt_init = S5L8900X_interrupt_init;
    gPESocDispatch.timebase_init = S5L8900X_timebase_init;

    gPESocDispatch.get_timebase = S5L8900X_get_timebase;

    gPESocDispatch.handle_interrupt = S5L8900X_handle_interrupt;

    gPESocDispatch.timer_value = S5L8900X_timer_value;
    gPESocDispatch.timer_enabled = S5L8900X_timer_enabled;

    gPESocDispatch.framebuffer_init = S5L8900X_framebuffer_init;

    char tempbuf[16];
    if (PE_parse_boot_argn("-avoid-uarts", tempbuf, sizeof(tempbuf))) {
        avoid_uarts = 1;
    }

    if (PE_parse_boot_argn("-force-uarts", tempbuf, sizeof(tempbuf))) {
        avoid_uarts = 0;
    }

    S5L8900X_framebuffer_init();
    S5L8900X_uart_init();

    PE_halt_restart = S5L8900X_halt_restart;
}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for S5L8900X\n");
    PE_init_SocSupport_S5L8900X();
}

#endif
