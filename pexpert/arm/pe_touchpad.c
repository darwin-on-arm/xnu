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
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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

#include "touchpad_msm.h"

#define KPRINTF_PREFIX  "PE_TouchPad: "

#define HwReg(x) *((volatile unsigned long*)(x))

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

#define uart_base   gTouchPadUartBase
vm_offset_t gTouchPadUartBase;
vm_offset_t gTouchPadClockGateBase;
vm_offset_t gTouchPadPmgrBase;
vm_offset_t gTouchpadqGICCPUBase;
vm_offset_t gTouchPadqGICDistributerBase;
vm_offset_t gTouchPadTimerBase;

static uint64_t clock_decrementer = 0;
static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_absolute_time = 0;

static void timer_configure(void)
{
    /*
     * DUMMY DUMMY
     */
    uint64_t hz = 6750000;
    gPEClockFrequencyInfo.timebase_frequency_hz = hz;

    clock_decrementer = 5000;
    kprintf(KPRINTF_PREFIX "decrementer frequency = %llu\n", clock_decrementer);

    rtc_configure(hz);
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
    gTouchPadqGICDistributerBase = ml_io_map(MSM_GIC_DIST_BASE, PAGE_SIZE);
    gTouchpadqGICCPUBase = ml_io_map(MSM_GIC_CPU_BASE, PAGE_SIZE);

    gTouchPadTimerBase = ml_io_map(MSM_TMR_BASE, PAGE_SIZE);
    return;
}


/* Intialize distributor */
static void qgic_dist_init(void)
{
    uint32_t i;
    uint32_t num_irq = 0;
    uint32_t cpumask = 1;

    cpumask |= cpumask << 8;
    cpumask |= cpumask << 16;

    /* Disabling GIC */
    HwReg(gTouchPadqGICDistributerBase + GIC_DIST_CTRL) = 0;

    /*
     * Find out how many interrupts are supported.
     */
    num_irq = HwReg(gTouchPadqGICDistributerBase + GIC_DIST_CTR) & 0x1f;
    num_irq = (num_irq + 1) * 32;

    /* Set each interrupt line to use N-N software model
     * and edge sensitive, active high
     */
    for (i=32; i < num_irq; i += 16)
        HwReg(gTouchPadqGICDistributerBase + GIC_DIST_CONFIG + i * 4/16) = 0xffffffff;

    HwReg(gTouchPadqGICDistributerBase + GIC_DIST_CONFIG + 4) = 0xffffffff;

    /* Set up interrupts for this CPU */
    for (i = 32; i < num_irq; i += 4)
        HwReg(gTouchPadqGICDistributerBase + GIC_DIST_TARGET + i * 4 / 4) = cpumask;

    /* Set priority of all interrupts*/

    /*
     * In bootloader we dont care about priority so
     * setting up equal priorities for all
     */
    for (i=0; i < num_irq; i += 4)
        HwReg(gTouchPadqGICDistributerBase + GIC_DIST_PRI) = 0xa0a0a0a0;

    /* Disabling interrupts*/
    for (i=0; i < num_irq; i += 32)
        HwReg(gTouchPadqGICDistributerBase + GIC_DIST_ENABLE_CLEAR + i * 4/32) = 0xffffffff;

    HwReg(gTouchPadqGICDistributerBase + GIC_DIST_ENABLE_SET) = 0xffff;

    /*Enabling GIC*/
    HwReg(gTouchPadqGICDistributerBase + GIC_DIST_CTRL) = 0x1;
}

/* Intialize cpu specific controller */
static void qgic_cpu_init(void)
{
    HwReg(gTouchpadqGICCPUBase + GIC_CPU_PRIMASK) = 0xf0;
    HwReg(gTouchpadqGICCPUBase + GIC_CPU_CTRL) = 0x1;
}

void TouchPad_interrupt_init(void)
{
    assert(gTouchPadqGICDistributerBase && gTouchpadqGICCPUBase);

    /* Initialize qGIC. */
    qgic_dist_init();
    qgic_cpu_init();

    return;
}

uint64_t TouchPad_timer_value(void);
void TouchPad_timer_enabled(int enable);

void TouchPad_timebase_init(void)
{
    /* Set Rtclock stuff. */
    timer_configure();

    /* Disable timer. */
    TouchPad_timer_enabled(FALSE);

    /* Unmask interrupt. */
    uint32_t reg = GIC_DIST_ENABLE_SET + (INT_DEBUG_TIMER_EXP/32)*4;
    uint32_t bit = 1 << (INT_DEBUG_TIMER_EXP & 31);
    HwReg(gTouchPadqGICDistributerBase + reg) = (bit);

    /* Enable interrupts. */
    ml_set_interrupts_enabled(TRUE);

    /* ARM timer. */
    TouchPad_timer_enabled(TRUE);

    /* Wait for pong. */
    clock_initialized = TRUE;
    while(!clock_had_irq)
        barrier();

    return;
}

void TouchPad_handle_interrupt(void *context)
{
    uint32_t irq_no = HwReg(gTouchpadqGICCPUBase + GIC_CPU_INTACK);
    if(irq_no > NR_IRQS) {
        kprintf(KPRINTF_PREFIX "Got a bogus IRQ?");
        return;
    }

    /* Timer interrupt? */
    if(irq_no == INT_DEBUG_TIMER_EXP) {
        TouchPad_timer_enabled(FALSE);
        clock_absolute_time += (clock_decrementer - (int64_t) TouchPad_timer_value());
        rtclock_intr((arm_saved_state_t *) context);
        TouchPad_timer_enabled(TRUE);
        clock_had_irq = TRUE;
    } else {
        irq_iokit_dispatch(irq_no);
    }

    /* EOI. */
    HwReg(gTouchpadqGICCPUBase + GIC_CPU_EOI) = irq_no;
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
    /* Don't bother. HwReg(gTouchPadTimerBase + DGT_COUNT_VAL).. */
    uint64_t ret = 0;
    return ret;
}

void TouchPad_timer_enabled(int enable)
{
    if(enable) {
        HwReg(gTouchPadTimerBase + DGT_MATCH_VAL) = clock_decrementer;
        HwReg(gTouchPadTimerBase + DGT_CLEAR) = 0;
        HwReg(gTouchPadTimerBase + DGT_ENABLE) = DGT_ENABLE_EN | DGT_ENABLE_CLR_ON_MATCH_EN;
    } else {
        HwReg(gTouchPadTimerBase + DGT_ENABLE) = 0;
        barrier();
        HwReg(gTouchPadTimerBase + DGT_CLEAR) = 0;
        barrier();
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
