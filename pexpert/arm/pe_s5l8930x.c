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
 * Platform Expert for Samsung S5L8930X devices.
 *
 * Now includes S5L8920X and S5L8922X!
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
#if defined(BOARD_CONFIG_S5L8930X) || defined(BOARD_CONFIG_S5L8920X) || defined(BOARD_CONFIG_S5L8922X)
#define KPRINTF_PREFIX  "PE_SamsungS5L: "

#include "s5l8930x.h"

#define HwReg(x) *((volatile unsigned long*)(x))

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

#define uart_base   gS5L8930XUartBase
vm_offset_t gS5L8930XUartBase;
vm_offset_t gS5L8930XClockGateBase;

vm_offset_t gS5L8930XPmgrBase;

/* The 8930 has 4 PL192 compatible VICs. */
vm_offset_t gS5L8930XVic0Base;
vm_offset_t gS5L8930XVic1Base;
vm_offset_t gS5L8930XVic2Base;
vm_offset_t gS5L8930XVic3Base;

vm_offset_t gS5L8930XTimerBase;

#ifdef BOARD_CONFIG_S5L8930X
static boolean_t avoid_uarts = FALSE;
#else
/* Busted... */
static boolean_t avoid_uarts = TRUE;
#endif

static uint64_t clock_decrementer = 0;
static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_absolute_time = 0;

static void s5l8930x_clock_gate_switch(int gate, int state)
{
    uint32_t __register;

    assert(gS5L8930XClockGateBase);

    if (gate > 0x3f)
        return;

    __register = CLK_REG_OFF + (gate << 2);

#if defined(BOARD_CONFIG_S5L8920X) || defined(BOARD_CONFIG_S5L8922X)
    __register -= CLK_REG_OFF;
    __register += 0x78;
#endif

    if (state) {
        HwReg(gS5L8930XClockGateBase + __register) = HwReg(gS5L8930XClockGateBase + __register) | 0xF;
    } else {
        HwReg(gS5L8930XClockGateBase + __register) = HwReg(gS5L8930XClockGateBase + __register) & ~0xF;
    }

    /*
     * Wait for the state change to take effect. 
     */
    while ((HwReg(gS5L8930XClockGateBase + __register) & 0xF) != ((HwReg(gS5L8930XClockGateBase + __register) >> 4) & 0xF))
        barrier();

    return;
}

static void timer_configure(void)
{
    /*
     * DUMMY 
     */
    uint64_t hz = 24000000;
    gPEClockFrequencyInfo.timebase_frequency_hz = hz;

    clock_decrementer = 10000;
    kprintf(KPRINTF_PREFIX "decrementer frequency = %llu\n", clock_decrementer);

    rtc_configure(hz);
    return;
}

void S5L8930X_putc(int c)
{
    if(c == '\n') S5L8930X_putc('\r');

    /*
     * Wait for FIFO queue to empty. 
     */
    while (HwReg(gS5L8930XUartBase + UFSTAT) & UART_UFSTAT_TXFIFO_FULL)
        barrier();

    HwReg(gS5L8930XUartBase + UTXH) = c;
    return;
}

int S5L8930X_getc(void)
{
    /*
     * Wait for a character. 
     */
    int i = 0x80;
    uint32_t ufstat = HwReg(gS5L8930XUartBase + UFSTAT);
    boolean_t can_read = FALSE;

    can_read = (ufstat & UART_UFSTAT_RXFIFO_FULL) | (ufstat & 0xF);
    if(can_read)
        return HwReg(gS5L8930XUartBase + URXH);
    else
        return -1;

    return -1;
}

void S5L8930X_uart_init(void)
{
    uint32_t divisorValue;
    /*
     * xxx map pmgr 
     */
#ifdef BOARD_CONFIG_S5L8930X
    gS5L8930XPmgrBase = ml_io_map(0xBF102000, PAGE_SIZE);
    assert(gS5L8930XPmgrBase);
#elif defined(BOARD_CONFIG_S5L8922X) || defined(BOARD_CONFIG_S5L8920X)
    gS5L8930XPmgrBase = ml_io_map(0xBF100000, PAGE_SIZE);
    assert(gS5L8930XPmgrBase);
#endif

    /*
     * XXX: The UART init routine is also the Core Platform mapping routine... 
     */
    gS5L8930XVic0Base = ml_io_map(VIC(0), PAGE_SIZE);
    gS5L8930XVic1Base = ml_io_map(VIC(1), PAGE_SIZE);
    gS5L8930XVic2Base = ml_io_map(VIC(2), PAGE_SIZE);
    gS5L8930XVic3Base = ml_io_map(VIC(3), PAGE_SIZE);
    assert(gS5L8930XVic0Base && gS5L8930XVic1Base && gS5L8930XVic2Base && gS5L8930XVic3Base);

    /*
     * Clocks. 
     */
    gS5L8930XTimerBase = ml_io_map(TIMER0_BASE, PAGE_SIZE);
    assert(gS5L8930XTimerBase);

    /*
     * Map the UARTs... 
     */
    gS5L8930XUartBase = ml_io_map(UART0_BASE, PAGE_SIZE);

    /*
     * Also map the ClockGate Base 
     */
    gS5L8930XClockGateBase = ml_io_map(CLOCK_GATE_BASE, PAGE_SIZE);

    assert(gS5L8930XUartBase && gS5L8930XClockGateBase);

    if (avoid_uarts)
        return;

    /*
     * Enable clock gate. 
     */
    s5l8930x_clock_gate_switch(UART_CLOCKGATE, TRUE);

    /*
     * Set 8-bit frames. 
     */
    HwReg(gS5L8930XUartBase + ULCON) = UART_8BITS;

    /*
     * Use polling for RX/TX. 
     */
    HwReg(gS5L8930XUartBase + UCON) = ((UART_UCON_MODE_IRQORPOLL << UART_UCON_RXMODE_SHIFT) | (UART_UCON_MODE_IRQORPOLL << UART_UCON_TXMODE_SHIFT));

    /*
     * Set clock. 
     */
    HwReg(gS5L8930XUartBase + UCON) = (HwReg(gS5L8930XUartBase + UCON) & (~UART_CLOCK_SELECTION_MASK)) | (1 << UART_CLOCK_SELECTION_SHIFT);

    /*
     * Set baud to 115200. 
     */
    divisorValue = CLOCK_HZ / (115200 * 16) - 1;

    HwReg(gS5L8930XUartBase + UBRDIV) = (HwReg(gS5L8930XUartBase + UBRDIV) & (~UART_DIVVAL_MASK)) | divisorValue;

    /*
     * Reset FIFO 
     */
    HwReg(gS5L8930XUartBase + UFCON) = UART_FIFO_RESET_RX | UART_FIFO_RESET_TX;

    /*
     * Enable FIFO 
     */
    HwReg(gS5L8930XUartBase + UFCON) = UART_FIFO_ENABLE;

    PE_kputc = S5L8930X_putc;

    kprintf(KPRINTF_PREFIX "serial is up\n");

    return;
}

void S5L8930X_interrupt_init(void)
{
    /*
     * Disable interrupts 
     */
    ml_set_interrupts_enabled(FALSE);

    /*
     * Goddamn am I paranoid. 
     */
    assert(gS5L8930XVic0Base && gS5L8930XVic1Base && gS5L8930XVic2Base && gS5L8930XVic3Base);

    /*
     * Disable all interrupts. 
     */
    HwReg(gS5L8930XVic0Base + VICINTENCLEAR) = 0xFFFFFFFF;
    HwReg(gS5L8930XVic1Base + VICINTENCLEAR) = 0xFFFFFFFF;
    HwReg(gS5L8930XVic2Base + VICINTENCLEAR) = 0xFFFFFFFF;
    HwReg(gS5L8930XVic3Base + VICINTENCLEAR) = 0xFFFFFFFF;

    HwReg(gS5L8930XVic0Base + VICINTENABLE) = 0;
    HwReg(gS5L8930XVic1Base + VICINTENABLE) = 0;
    HwReg(gS5L8930XVic2Base + VICINTENABLE) = 0;
    HwReg(gS5L8930XVic3Base + VICINTENABLE) = 0;

    /*
     * Please use IRQs. I don't want to implement a FIQ based timer decrementer handler. 
     */
    HwReg(gS5L8930XVic0Base + VICINTSELECT) = 0;
    HwReg(gS5L8930XVic1Base + VICINTSELECT) = 0;
    HwReg(gS5L8930XVic2Base + VICINTSELECT) = 0;
    HwReg(gS5L8930XVic3Base + VICINTSELECT) = 0;

    /*
     * Unmask all interrupt levels. 
     */
    HwReg(gS5L8930XVic0Base + VICSWPRIORITYMASK) = 0xFFFF;
    HwReg(gS5L8930XVic1Base + VICSWPRIORITYMASK) = 0xFFFF;
    HwReg(gS5L8930XVic2Base + VICSWPRIORITYMASK) = 0xFFFF;
    HwReg(gS5L8930XVic3Base + VICSWPRIORITYMASK) = 0xFFFF;

    /*
     * Set vector addresses to interrupt numbers. 
     */
    int i;
    for (i = 0; i < 0x20; i++) {
        HwReg(gS5L8930XVic0Base + VICVECTADDRS + (i * 4)) = (0x20 * 0) + i;
        HwReg(gS5L8930XVic1Base + VICVECTADDRS + (i * 4)) = (0x20 * 1) + i;
        HwReg(gS5L8930XVic2Base + VICVECTADDRS + (i * 4)) = (0x20 * 2) + i;
        HwReg(gS5L8930XVic3Base + VICVECTADDRS + (i * 4)) = (0x20 * 3) + i;
    }

    return;
}

uint64_t S5L8930X_timer_value(void);
void S5L8930X_timer_enabled(int enable);

void S5L8930X_timebase_init(void)
{
    assert(gS5L8930XTimerBase);

    /*
     * Set rtclock stuff 
     */
    timer_configure();

    /*
     * Disable the timer. 
     */
    S5L8930X_timer_enabled(FALSE);

    /*
     * Enable the interrupt. 
     */
    HwReg(gS5L8930XVic0Base + VICINTENABLE) = HwReg(gS5L8930XVic0Base + VICINTENABLE) | (1 << 5) | (1 << 6);

    /*
     * Enable interrupts. 
     */
    ml_set_interrupts_enabled(TRUE);

    /*
     * Wait for it. 
     */
    kprintf(KPRINTF_PREFIX "waiting for system timer to come up...\n");
    S5L8930X_timer_enabled(TRUE);

    clock_initialized = TRUE;

    while (!clock_had_irq)
        barrier();

    return;
}

void S5L8930X_handle_interrupt(void *context)
{
    uint32_t current_irq = HwReg(gS5L8930XVic0Base + VICADDRESS);
    /*
     * Timer IRQs are handeled by us. 
     */
    if (current_irq == 6) {
        /*
         * Disable timer 
         */
        S5L8930X_timer_enabled(FALSE);

        /*
         * Update absolute time 
         */
        clock_absolute_time += (clock_decrementer - (int64_t) S5L8930X_timer_value());

        /*
         * Resynchronize deadlines. 
         */
        rtclock_intr((arm_saved_state_t *) context);

        /*
         * EOI. 
         */
        HwReg(gS5L8930XVic0Base + VICADDRESS) = 0;

        /*
         * Enable timer. 
         */
        S5L8930X_timer_enabled(TRUE);

        /*
         * We had an IRQ. 
         */
        clock_had_irq = TRUE;
    } else {
        irq_iokit_dispatch(current_irq);
    }

    return;
}

uint64_t S5L8930X_get_timebase(void)
{
    uint32_t timestamp;

    if (!clock_initialized)
        return 0;

    timestamp = S5L8930X_timer_value();

    if (timestamp) {
        uint64_t v = clock_absolute_time;
        v += (uint64_t) (((uint64_t) clock_decrementer) - (uint64_t) (timestamp));
        return v;
    } else {
        clock_absolute_time += clock_decrementer;
        return clock_absolute_time;
    }
}

uint64_t S5L8930X_timer_value(void)
{
    uint64_t ret = (uint64_t) ((uint32_t) 0xFFFFFFFF - (uint32_t) HwReg(gS5L8930XTimerBase + TIMER0_VAL));

    /*
     * HACK 
     */
    if (ret >= clock_decrementer)
        ret = 0;

    return ret;
}

void S5L8930X_timer_enabled(int enable)
{
    /*
     * How do you disable this timer? 
     */
    if (!enable) {
        HwReg(gS5L8930XTimerBase + TIMER0_CTRL) = 2;
        HwReg(gS5L8930XTimerBase + TIMER0_CTRL) = 0;
    } else {
        HwReg(gS5L8930XTimerBase + TIMER0_VAL) = 0xFFFFFFFF;
        HwReg(gS5L8930XTimerBase + TIMER0_CTRL) = 3;
        HwReg(gS5L8930XTimerBase + TIMER0_CTRL) = 1;
        HwReg(gS5L8930XTimerBase + TIMER0_VAL) = clock_decrementer;
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
    if (avoid_uarts)
        S5L8930X_putc(c);
}

void S5L8930X_framebuffer_init(void)
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

int S5L8930X_halt_restart(int type)
{
#ifdef BOARD_CONFIG_S5L8930X
    /*
     * Just reboot. 
     */
    assert(gS5L8930XPmgrBase);
    HwReg(gS5L8930XPmgrBase + 0x2C) = 0;
    HwReg(gS5L8930XPmgrBase + 0x24) = 1;
    HwReg(gS5L8930XPmgrBase + 0x20) = 0x80000000;
    HwReg(gS5L8930XPmgrBase + 0x2C) = 4;
    HwReg(gS5L8930XPmgrBase + 0x20) = 0;
#elif defined(BOARD_CONFIG_S5L8920X)
    assert(gS5L8930XPmgrBase);
    HwReg(gS5L8930XPmgrBase + 0x21C) = 0;
    HwReg(gS5L8930XPmgrBase + 0x214) = 1;
    HwReg(gS5L8930XPmgrBase + 0x210) = 0x80000000;
    HwReg(gS5L8930XPmgrBase + 0x21C) = 4;
    HwReg(gS5L8930XPmgrBase + 0x210) = 0;
#elif defined(BOARD_CONFIG_S5L8922X)
    assert(gS5L8930XPmgrBase);
    HwReg(gS5L8930XPmgrBase + 0x21C) = 0;
    HwReg(gS5L8930XPmgrBase + 0x214) = 1;
    HwReg(gS5L8930XPmgrBase + 0x210) = 0x80000000;
    HwReg(gS5L8930XPmgrBase + 0x21C) = 4;
    HwReg(gS5L8930XPmgrBase + 0x210) = 0;
#endif
    /*
     * xxx never reached 
     */
    return 0;
}

void PE_init_SocSupport_S5L8930X(void)
{
    gPESocDispatch.uart_getc = S5L8930X_getc;
    gPESocDispatch.uart_putc = S5L8930X_putc;
    gPESocDispatch.uart_init = S5L8930X_uart_init;

    gPESocDispatch.interrupt_init = S5L8930X_interrupt_init;
    gPESocDispatch.timebase_init = S5L8930X_timebase_init;

    gPESocDispatch.get_timebase = S5L8930X_get_timebase;

    gPESocDispatch.handle_interrupt = S5L8930X_handle_interrupt;

    gPESocDispatch.timer_value = S5L8930X_timer_value;
    gPESocDispatch.timer_enabled = S5L8930X_timer_enabled;

    gPESocDispatch.framebuffer_init = S5L8930X_framebuffer_init;

    char tempbuf[16];
    if (PE_parse_boot_argn("-avoid-uarts", tempbuf, sizeof(tempbuf))) {
        avoid_uarts = 1;
    }

    if (PE_parse_boot_argn("-force-uarts", tempbuf, sizeof(tempbuf))) {
        avoid_uarts = 0;
    }

    S5L8930X_framebuffer_init();
    S5L8930X_uart_init();

    PE_halt_restart = S5L8930X_halt_restart;
}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for S5L8930X\n");
    PE_init_SocSupport_S5L8930X();
}

#endif
