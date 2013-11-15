/*
 * Copyright 2013, winocm. <winocm@icloud.com>
 * Copyright 2013, furkanmustafa.
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
 * Platform Expert for OMAP335X
 */

#include <sys/types.h>
#include <mach/vm_param.h>
#include <machine/machine_routines.h>
#include <pexpert/device_tree.h>
#include <pexpert/protos.h>
#include <pexpert/pexpert.h>
#include <kern/debug.h>
#include <kern/simple_lock.h>
#include <machine/machine_routines.h>
#include <vm/pmap.h>
#include <arm/pmap.h>

extern int disableConsoleOutput, serialmode;

#ifdef BOARD_CONFIG_OMAP335X

void Omap3_timer_enabled(int enable);
uint64_t Omap3_timer_value(void);
uint64_t Omap3_get_timebase(void);

#include "omap335x.h"

#define mmio_read(a)    (*(volatile uint32_t *)(a))
#define mmio_write(a,v) (*(volatile uint32_t *)(a) = (v))
#define mmio_set(a,v)   mmio_write((a), mmio_read((a)) | (v))
#define mmio_clear(a,v) mmio_write((a), mmio_read((a)) & ~(v))

#define HwReg8(x) *((volatile uint8_t*)(x))
#define HwReg16(x) *((volatile uint16_t*)(x))
#define HwReg32(x) *((volatile uint32_t*)(x))

#define KPRINTF_PREFIX  "PE_omap335x: "

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

vm_offset_t gOmapSerialUartBase = 0x0;
vm_offset_t gOmapInterruptControllerBase = 0x0;
vm_offset_t gOmapTimerBase = 0x0;
vm_offset_t gOmapDisplayControllerBase = 0x0;
vm_offset_t gOmapPrcmBase = 0x0;

static uint64_t clock_decrementer = 0;
static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_absolute_time = 0;

#define FLD_VAL(val, start, end) (((val) << (end)) & FLD_MASK(start, end))
#define FLD_MASK(start, end)    (((1 << ((start) - (end) + 1)) - 1) << (end))
#define FLD_VAL(val, start, end) (((val) << (end)) & FLD_MASK(start, end))

static void timer_configure(void)
{
    /*
     * xxx hack for etimer since it does not know time yet 
     */
    uint64_t hz = 3276800;
    gPEClockFrequencyInfo.timebase_frequency_hz = hz;

    clock_decrementer = 1000;
    kprintf(KPRINTF_PREFIX "decrementer frequency = %llu\n", clock_decrementer);

    rtc_configure(hz);
    return;
}

void Omap3_early_putc(int c)
{
    if (c == '\n')
        Omap3_early_putc('\r');

    while ((HwReg32(OMAP3_UART_BASE + SSR) & SSR_TXFIFOFULL))
        barrier();

    HwReg32(OMAP3_UART_BASE + THR) = c;
}

void Omap3_putc(int c)
{
    if (!gOmapSerialUartBase)
        return;

    if (c == '\n')
        Omap3_putc('\r');

    while ((HwReg32(gOmapSerialUartBase + SSR) & SSR_TXFIFOFULL))
        barrier();

    HwReg32(gOmapSerialUartBase + THR) = c;
}

int Omap3_getc(void)
{
    int i = 0x80000;
    while (!(HwReg32(gOmapSerialUartBase + LSR) & LSR_DR)) {
        i--; if(!i) return -1;
    }

    return (HwReg32(gOmapSerialUartBase + RBR));
}

void Omap3_uart_init(void)
{
    int baudDivisor;

    assert(OMAP3_UART_BAUDRATE != 0);
    baudDivisor = (OMAP3_UART_CLOCK / 16 / OMAP3_UART_BAUDRATE);

    HwReg32(gOmapSerialUartBase + IER) = 0x00;
    HwReg32(gOmapSerialUartBase + LCR) = LCR_BKSE | LCRVAL;
    HwReg32(gOmapSerialUartBase + DLL) = baudDivisor & 0xFF;
    HwReg32(gOmapSerialUartBase + DLM) = (baudDivisor >> 8) & 0xFF;
    HwReg32(gOmapSerialUartBase + LCR) = LCRVAL;
    HwReg32(gOmapSerialUartBase + MCR) = MCRVAL;
    HwReg32(gOmapSerialUartBase + FCR) = FCRVAL;
}

void Omap3_interrupt_init(void)
{
    int i;

    /*
     * Disable interrupts 
     */
    ml_set_interrupts_enabled(FALSE);

    /*
     * Set MIR bits to enable all interrupts 
     */
    HwReg32(INTCPS_MIR(0)) = 0xffffffff;
    HwReg32(INTCPS_MIR(1)) = 0xffffffff;
    HwReg32(INTCPS_MIR(2)) = 0xffffffff;

    /*
     * Set the true bits (for interrupts we're interested in)
     */
    mmio_write(INTCPS_MIR_CLEAR(OMAP335X_SCH_TIMER_IRQ >> 5), 1 << (OMAP335X_SCH_TIMER_IRQ & 0x1f));

    /*
     * Set enable new IRQs/FIQs 
     */
    HwReg32(INTCPS_CONTROL) = (1 << 0);

    barrier();
    return;
}

void Omap3_timebase_init(void)
{
    /*
     * Stop the timer. 
     */
    Omap3_timer_enabled(FALSE);

    /*
     * Enable interrupts 
     */
    ml_set_interrupts_enabled(TRUE);

    /*
     * Set rtclock stuff 
     */
    timer_configure();

    /*
     * Set timer decrementer defaults 
     */
    HwReg32(gOmapTimerBase + TLDR) = 0xffffffe0;
    HwReg32(gOmapTimerBase + TCRR) = 0xffffffe0;

    HwReg32(gOmapTimerBase + TPIR) = 232000;
    HwReg32(gOmapTimerBase + TNIR) = -768000;

    HwReg32(gOmapTimerBase + TOCR) = 0;
    HwReg32(gOmapTimerBase + TOWR) = 100;

    HwReg32(gOmapTimerBase + TCLR) = (1 << 6);

    /*
     * !!! SET INTERRUPTS ENABLED ON OVERFLOW 
     */
    HwReg32(gOmapTimerBase + TISR) = 0x7;   // 0x7; //0x2;
    HwReg32(gOmapTimerBase + TIER) = 0x7;   // 0x7; //0x2;

    kprintf(KPRINTF_PREFIX "starting timer..\n");

    // /*
    //  * Set to 32KHz 
    //  */
    // mmio_set(gOmapPrcmBase + 0xc40, 0x40);           // CLKSEL_TIMER2_CLK = 32K_FCLK ????
    // HwReg32(gOmapPrcmBase + 0x08) &= 0x02;
    // mmio_set(gOmapPrcmBase + 0x400 + 0xc4, 0x02);            // CLKSEL_TIMER2_CLK = 32K_FCLK

    /*
     * Arm the timer 
     */
    HwReg32(gOmapTimerBase + TCLR) = (1 << 0) | (1 << 1) | (2 << 10);

    /*
     * Wait for it. 
     */
    clock_initialized = TRUE;

    while (!clock_had_irq)
        barrier();

    kprintf(KPRINTF_PREFIX "timer is now up, ticks %llu\n", Omap3_timer_value());

    return;
}

void Omap3_handle_interrupt(void *context)
{
    uint32_t irq_number = (HwReg32(INTCPS_SIR_IRQ)) & 0x7F;

    if (irq_number == OMAP335X_SCH_TIMER_IRQ) {
        /*
         * Stop the timer 
         */
        Omap3_timer_enabled(FALSE);

        /*
         * Clear interrupt status 
         */
        HwReg32(gOmapTimerBase + TISR) = 0x7;   // 0x2; wrong?

        /*
         * FFFFF 
         */
        rtclock_intr((arm_saved_state_t *) context);

        /*
         * Set new IRQ generation 
         */
        HwReg32(INTCPS_CONTROL) = 0x1;

        /*
         * ARM IT. 
         */
        Omap3_timer_enabled(TRUE);

        /*
         * Update absolute time 
         */
        clock_absolute_time += (clock_decrementer - Omap3_timer_value());

        clock_had_irq = 1;

        return;
    } else {
        irq_iokit_dispatch(irq_number);
    }

    return;
}

uint64_t Omap3_get_timebase(void)
{
    uint32_t timestamp;

    if (!clock_initialized)
        return 0;

    timestamp = Omap3_timer_value();

    if (timestamp) {
        uint64_t v = clock_absolute_time;
        v += (uint64_t) (((uint64_t) clock_decrementer) - (uint64_t) (timestamp));
        return v;
    } else {
        clock_absolute_time += clock_decrementer;
        return clock_absolute_time;
    }
}

uint64_t Omap3_timer_value(void)
{
    /*
     * Return overflow value minus the counter 
     */
    return 0xffffffff - (HwReg32(gOmapTimerBase + TCRR));
}

void Omap3_timer_enabled(int enable)
{

    /*
     * Clear the TCLR [ST] bit 
     */
    if (enable)
        mmio_set(gOmapTimerBase + TCLR, 0x1);
    else
        mmio_clear(gOmapTimerBase + TCLR, 0x1);

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
    Omap3_putc(c);
}

extern int serialmode;

void Omap3_framebuffer_init(void)
{
    /*
     * This is an emulated framebuffer. 
     */
    void *framebuffer = pmap_steal_memory(1024 * 768 * 4);
    void *framebuffer_phys = pmap_extract(kernel_pmap, framebuffer);

    uint32_t depth = 4;
    uint32_t width = 1024;
    uint32_t height = 768;

    uint32_t pitch = (width * depth);
    uint32_t fb_length = (pitch * width);

    PE_state.video.v_baseAddr = (unsigned long) framebuffer_phys;
    PE_state.video.v_rowBytes = width * 2;
    PE_state.video.v_width = width;
    PE_state.video.v_height = height;
    PE_state.video.v_depth = 2 * (8);   // 16bpp

    kprintf(KPRINTF_PREFIX "fake framebuffer initialized\n");
    bzero(framebuffer, (pitch * height));

    char tempbuf[16];
    initialize_screen((void *) &PE_state.video, kPETextMode);

    // TEMPORARY, because beaglebone (usually) doesn't have a display
    serialmode = 3;
    switch_to_serial_console();
    disableConsoleOutput = FALSE;

    return;
}

void Omap3_InitCaches(void)
{
    kprintf(KPRINTF_PREFIX "initializing i+dcache\n");
    cache_initialize();
    kprintf(KPRINTF_PREFIX "done\n");
}

void PE_init_SocSupport_omap3(void)
{
    gPESocDispatch.uart_getc = Omap3_getc;
    gPESocDispatch.uart_putc = Omap3_putc;
    gPESocDispatch.uart_init = Omap3_uart_init;

    gPESocDispatch.interrupt_init = Omap3_interrupt_init;
    gPESocDispatch.timebase_init = Omap3_timebase_init;

    gPESocDispatch.get_timebase = Omap3_get_timebase;

    gPESocDispatch.handle_interrupt = Omap3_handle_interrupt;

    gPESocDispatch.timer_value = Omap3_timer_value;
    gPESocDispatch.timer_enabled = Omap3_timer_enabled;

    gPESocDispatch.framebuffer_init = Omap3_framebuffer_init;

    // init device base addresses
    gOmapSerialUartBase = ml_io_map(OMAP3_UART_BASE, PAGE_SIZE);
    gOmapTimerBase = ml_io_map(OMAP335X_SCH_TIMER_BASE, PAGE_SIZE);
    gOmapInterruptControllerBase = ml_io_map(OMAP3_GIC_BASE, PAGE_SIZE);
    // gOmapDisplayControllerBase = ml_io_map(OMAP3_DSS_BASE - 0x40, PAGE_SIZE); // doesn't apply for omap335x yet

    gOmapPrcmBase = ml_io_map(0x44E00000, PAGE_SIZE);   // 0x48004000 (L4 Core / Clock Manager)

    Omap3_uart_init();
    PE_kputc = gPESocDispatch.uart_putc;

    Omap3_framebuffer_init();

    mmio_set(gOmapPrcmBase + 0x404, 0x2);   // Enable Timer1 Clock
    mmio_set(gOmapPrcmBase + 0x4C4, 0x2);   // Enable Timer1 Clock
}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for OMAP335x\n");
    PE_init_SocSupport_omap3();
}

#endif
