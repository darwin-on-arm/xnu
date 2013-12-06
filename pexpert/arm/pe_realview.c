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
 * RealView init.
 */

#include <mach/mach_types.h>

#include <pexpert/pexpert.h>
#include <pexpert/arm/protos.h>
#include <pexpert/arm/boot.h>

#include <machine/machine_routines.h>

#include <vm/pmap.h>
#include <arm/pmap.h>

#include "realview.h"

/*
 * This is board specific stuff.
 */
#ifdef BOARD_CONFIG_ARMPBA8

#define KPRINTF_PREFIX      "PE_RealView: "

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

vm_offset_t gRealviewUartBase;
vm_offset_t gRealviewPicBase;
vm_offset_t gRealviewTimerBase;
vm_offset_t gRealviewSysControllerBase;

vm_offset_t gRealviewPicDistribBase;

vm_offset_t gRealviewPl111Base;

static uint64_t clock_decrementer = 0;
static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_absolute_time = 0;

static void timer_configure(void)
{
    uint64_t hz = 320000;
    clock_decrementer = (hz / 7);   // For 500Hz.

    gPEClockFrequencyInfo.timebase_frequency_hz = hz;

    kprintf(KPRINTF_PREFIX "decrementer frequency = %llu\n", clock_decrementer);

    rtc_configure(hz);
}

void RealView_putc(int c)
{
    if (!gRealviewUartBase)
        return;

    while (AMBA_UART_FR(gRealviewUartBase) & (1 << 5))
        barrier();

    if(c == '\n')
        RealView_putc('\r');

    AMBA_UART_DR(gRealviewUartBase) = c;
}

int RealView_getc(void)
{
    unsigned char c;
    if (!gRealviewUartBase)
        return -1;

    int i = 0x80000;
    while (AMBA_UART_FR(gRealviewUartBase) & (1 << 4)) {
        i--; if(!i) return -1;
    }

    c = AMBA_UART_DR(gRealviewUartBase);
    return c;
}

void RealView_uart_init(void)
{
    char temp_buf[16];

    gRealviewUartBase = ml_io_map(REALVIEW_UART0_BASE, PAGE_SIZE);

    if (PE_parse_boot_argn("-use_realview_eb_pic", temp_buf, sizeof(temp_buf))) {
        gRealviewPicBase = ml_io_map(REALVIEW_EB_PIC0_BASE, PAGE_SIZE);
        gRealviewPicDistribBase = ml_io_map(REALVIEW_EB_PIC0_BASE + PAGE_SIZE, PAGE_SIZE);
    } else {
        gRealviewPicBase = ml_io_map(REALVIEW_PIC0_BASE, PAGE_SIZE);
        gRealviewPicDistribBase = ml_io_map(REALVIEW_PIC0_BASE + PAGE_SIZE, PAGE_SIZE);
    }

    gRealviewSysControllerBase = ml_io_map(REALVIEW_SYSCTL_BASE, PAGE_SIZE);
    gRealviewTimerBase = ml_io_map(REALVIEW_TIMER0_BASE, PAGE_SIZE);
}

void RealView_interrupt_init(void)
{
    char temp_buf[16];

    kprintf(KPRINTF_PREFIX "pic at 0x%08x, distribution at 0x%08x\n", gRealviewPicBase, gRealviewPicDistribBase);

    /*
     * Stuff. 
     */
    barrier();

    /*
     * do interrupt controller initialization 
     */
    HARDWARE_REGISTER(gRealviewPicBase) |= PIC_ENABLE;
    HARDWARE_REGISTER(gRealviewPicBase + PICPRIOMASK) |= PIC_ALLOW_INTR;

    /*
     * enable distribution 
     */
    HARDWARE_REGISTER(gRealviewPicDistribBase) |= PIC_ENABLE;

    /*
     * allow all interrupts 
     */
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x104) = -1;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x108) = -1;

    /*
     * set priorities and things. 
     */
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x420) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x424) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x428) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x42C) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x430) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x434) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x438) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x43C) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x440) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x444) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x448) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x44C) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x450) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x454) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x458) |= PIC_INTPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x45C) |= PIC_INTPRIO;

    /*
     * Set CPU targets 
     */
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x820) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x824) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x828) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x82C) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x830) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x834) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x838) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x83C) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x840) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x844) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x848) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x84C) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x850) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x854) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x858) |= PIC_CPUPRIO;
    HARDWARE_REGISTER(gRealviewPicDistribBase + 0x85C) |= PIC_CPUPRIO;

    return;
}

void RealView_timebase_init(void)
{
    assert(gRealviewTimerBase);

    timer_configure();

    /*
     * disable timer 
     */
    RealView_timer_enabled(FALSE);

    /*
     * set timer values and initialize decrementer 
     */
    HARDWARE_REGISTER(gRealviewTimerBase + TIMER_CONTROL) |= TIMER_MODE_FREE_RUNNING;
    HARDWARE_REGISTER(gRealviewTimerBase + TIMER_CONTROL) |= TIMER_SIZE_32_BIT;
    HARDWARE_REGISTER(gRealviewTimerBase + TIMER_CONTROL) |= TIMER_ENABLE;
    HARDWARE_REGISTER(gRealviewTimerBase) = clock_decrementer;

    /*
     * enable irqs so we can get ahold of the timer when it decrements 
     */
    ml_set_interrupts_enabled(TRUE);

    /*
     * re-enable timer 
     */
    RealView_timer_enabled(TRUE);

    clock_initialized = TRUE;

    while (!clock_had_irq)
        barrier();

    kprintf(KPRINTF_PREFIX "RealView Timer initialized, Timer value %llu\n", RealView_timer_value());

    return;
}

void RealView_handle_interrupt(void *context)
{
    arm_saved_state_t *regs = (arm_saved_state_t *) context;
    uint32_t ack;

    /*
     * Acknowledge interrupt 
     */
    ack = HARDWARE_REGISTER(gRealviewPicBase + 0xC);

    /*
     * Update absolute time 
     */
    RealView_timer_enabled(FALSE);
    clock_absolute_time += (clock_decrementer - RealView_timer_value());

    /*
     * Kill the timer 
     */
    HARDWARE_REGISTER(gRealviewTimerBase + TIMER_INTCLR) = 1;
    rtclock_intr((arm_saved_state_t *) context);

    /*
     * Restart timer. 
     */
    HARDWARE_REGISTER(gRealviewTimerBase) = clock_decrementer;
    RealView_timer_enabled(TRUE);

    clock_had_irq = TRUE;

    /*
     * EOI. 
     */
    HARDWARE_REGISTER(gRealviewPicBase + 0x10) = ack;

    return;
}

uint64_t RealView_get_timebase(void)
{
    uint32_t timestamp;

    if (!clock_initialized)
        return 0;

    timestamp = RealView_timer_value();
    if (timestamp) {
        uint64_t v = clock_absolute_time;
        v += (uint64_t) (((uint64_t) clock_decrementer) - (uint64_t) (timestamp));
        return v;
    } else {
        HARDWARE_REGISTER(gRealviewTimerBase) = clock_decrementer;
        RealView_timer_enabled(TRUE);
        clock_absolute_time += clock_decrementer;

        return clock_absolute_time;
    }
}

uint64_t RealView_timer_value(void)
{
    return (HARDWARE_REGISTER(gRealviewTimerBase + TIMER_VALUE));
}

void RealView_timer_enabled(int enable)
{
    if (enable)
        HARDWARE_REGISTER(gRealviewTimerBase + TIMER_CONTROL) |= TIMER_SET_ENABLE;
    else
        HARDWARE_REGISTER(gRealviewTimerBase + TIMER_CONTROL) &= ~TIMER_SET_ENABLE;
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
    RealView_putc(c);
}

void RealView_framebuffer_init(void)
{
    gRealviewPl111Base = ml_io_map(REALVIEW_PL111_BASE, PAGE_SIZE);

    /*
     * The hardware demands a framebuffer, but the framebuffer has to be given
     * in a hardware address.
     */
    void *framebuffer = pmap_steal_memory(1024 * 768 * 4);
    void *framebuffer_phys = pmap_extract(kernel_pmap, framebuffer);

    uint32_t depth = 2;
    uint32_t width = 1024;
    uint32_t height = 768;

    uint32_t pitch = (width * depth);
    uint32_t fb_length = (pitch * width);

    uint32_t timingRegister, controlRegister;

    /*
     * Set framebuffer address 
     */
    HARDWARE_REGISTER(gRealviewPl111Base + PL111_UPPER_FB) = framebuffer_phys;
    HARDWARE_REGISTER(gRealviewPl111Base + PL111_LOWER_FB) = framebuffer_phys;

    /*
     * Initialize timings to 1024x768x16 
     */
    HARDWARE_REGISTER(gRealviewPl111Base + PL111_TIMINGS_0) = LCDTIMING0_PPL(width);
    HARDWARE_REGISTER(gRealviewPl111Base + PL111_TIMINGS_1) = LCDTIMING1_LPP(height);

    /*
     * Enable the TFT/LCD Display 
     */
    HARDWARE_REGISTER(gRealviewPl111Base + PL111_CONTROL) = LCDCONTROL_LCDEN | LCDCONTROL_LCDTFT | LCDCONTROL_LCDPWR | LCDCONTROL_LCDBPP(5);

    PE_state.video.v_baseAddr = (unsigned long) framebuffer_phys;
    PE_state.video.v_rowBytes = width * 4;
    PE_state.video.v_width = width;
    PE_state.video.v_height = height;
    PE_state.video.v_depth = 4 * (8);   // 16bpp

    kprintf(KPRINTF_PREFIX "framebuffer initialized\n");
    bzero(framebuffer, (pitch * height));

    char tempbuf[16];
    
	if (PE_parse_boot_argn("-graphics-mode", tempbuf, sizeof(tempbuf))) {
        /*
         * BootX like framebuffer. 
         */
        memset(framebuffer, 0xb9, PE_state.video.v_rowBytes * PE_state.video.v_height);
        initialize_screen((void *) &PE_state.video, kPEGraphicsMode);
    } else {
		initialize_screen((void *) &PE_state.video, kPETextMode);
	}
}

void PE_init_SocSupport_realview(void)
{
    gPESocDispatch.uart_getc = RealView_getc;
    gPESocDispatch.uart_putc = RealView_putc;
    gPESocDispatch.uart_init = RealView_uart_init;

    gPESocDispatch.interrupt_init = RealView_interrupt_init;
    gPESocDispatch.timebase_init = RealView_timebase_init;

    gPESocDispatch.get_timebase = RealView_get_timebase;

    gPESocDispatch.handle_interrupt = RealView_handle_interrupt;

    gPESocDispatch.timer_value = RealView_timer_value;
    gPESocDispatch.timer_enabled = RealView_timer_enabled;

    gPESocDispatch.framebuffer_init = RealView_framebuffer_init;

    RealView_uart_init();
    RealView_framebuffer_init();

}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for ARM RealView PB-A8\n");
    PE_init_SocSupport_realview();
}

#endif                          // BOARD_CONFIG_ARMPBA8
