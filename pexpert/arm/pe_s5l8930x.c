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
#ifdef BOARD_CONFIG_S5L8930X
#define KPRINTF_PREFIX  "PE_S5L8930X: "

#include "s5l8930x.h"

#define HwReg(x) *((volatile unsigned long*)(x))

extern void rtclock_intr(arm_saved_state_t* regs);
extern void rtc_configure(uint64_t hz);

#define uart_base   gS5L8930XUartBase
vm_offset_t     gS5L8930XUartBase;
vm_offset_t     gS5L8930XClockGateBase;

/* The 8930 has 4 PL192 compatible VICs. */
vm_offset_t     gS5L8930XVic0Base;
vm_offset_t     gS5L8930XVic1Base;
vm_offset_t     gS5L8930XVic2Base;
vm_offset_t     gS5L8930XVic3Base;

vm_offset_t     gS5L8930XTimerBase;

static uint64_t     clock_decrementer = 0;
static boolean_t    clock_initialized = FALSE;
static boolean_t    clock_had_irq = FALSE;
static uint64_t     clock_absolute_time = 0;

static void s5l8930x_clock_gate_switch(int gate, int state)
{
    uint32_t __register;

    assert(gS5L8930XClockGateBase);

    if(gate > 0x3f)
        return;

    __register = CLK_REG_OFF + (gate << 2);
    if(state) {
        HwReg(gS5L8930XClockGateBase + __register) = HwReg(gS5L8930XClockGateBase + __register) | 0xF;
    } else {
        HwReg(gS5L8930XClockGateBase + __register) = HwReg(gS5L8930XClockGateBase + __register) & ~0xF;
    }

    /* Wait for the state change to take effect. */
    while((HwReg(gS5L8930XClockGateBase + __register) & 0xF) != 
         ((HwReg(gS5L8930XClockGateBase + __register) >> 4) & 0xF))
        barrier();

    return;
}

static void timer_configure(void)
{
    /* DUMMY */
    uint64_t hz = 32768;
    gPEClockFrequencyInfo.timebase_frequency_hz = hz;

    clock_decrementer = 1000;
    kprintf(KPRINTF_PREFIX "decrementer frequency = %llu\n", clock_decrementer);    

    rtc_configure(hz);
    return;
}

void S5L8930X_putc(int c)
{
    /* Wait for FIFO queue to empty. */
    while(HwReg(gS5L8930XUartBase + UFSTAT) & UART_UFSTAT_TXFIFO_FULL)
        barrier();

    HwReg(gS5L8930XUartBase + UTXH) = c;    
    return;
}

int S5L8930X_getc(void)
{
    return 'A';
}

void S5L8930X_uart_init(void)
{
    uint32_t divisorValue;
    /* XXX: The UART init routine is also the Core Platform mapping routine... */
    gS5L8930XVic0Base = ml_io_map(VIC(0), PAGE_SIZE);
    gS5L8930XVic1Base = ml_io_map(VIC(1), PAGE_SIZE);
    gS5L8930XVic2Base = ml_io_map(VIC(2), PAGE_SIZE);
    gS5L8930XVic3Base = ml_io_map(VIC(3), PAGE_SIZE);
    assert(gS5L8930XVic0Base && gS5L8930XVic1Base &&
           gS5L8930XVic2Base && gS5L8930XVic3Base);

    /* Clocks. */
    gS5L8930XTimerBase = ml_io_map(TIMER0_BASE, PAGE_SIZE);
    assert(gS5L8930XTimerBase);

    /* Map the UARTs... */
    gS5L8930XUartBase = ml_io_map(UART0_BASE, PAGE_SIZE);

    /* Also map the ClockGate Base */
    gS5L8930XClockGateBase = ml_io_map(CLOCK_GATE_BASE, PAGE_SIZE);

    assert(gS5L8930XUartBase && gS5L8930XClockGateBase);

    /* Enable clock gate. */
    s5l8930x_clock_gate_switch(UART_CLOCKGATE, TRUE);

    /* Set 8-bit frames. */
    HwReg(gS5L8930XUartBase + ULCON) = UART_8BITS;

    /* Use polling for RX/TX. */
    HwReg(gS5L8930XUartBase + UCON) = 
        ((UART_UCON_MODE_IRQORPOLL << UART_UCON_RXMODE_SHIFT) |
        (UART_UCON_MODE_IRQORPOLL << UART_UCON_TXMODE_SHIFT));

    /* Set clock. */
    HwReg(gS5L8930XUartBase + UCON) = (HwReg(gS5L8930XUartBase + UCON) &
        (~UART_CLOCK_SELECTION_MASK)) | (1 << UART_CLOCK_SELECTION_SHIFT);

    /* Set baud to 115200. */
    divisorValue = CLOCK_HZ / (115200 * 16) - 1;

    HwReg(gS5L8930XUartBase + UBRDIV) = (HwReg(gS5L8930XUartBase + UBRDIV) &
        (~UART_DIVVAL_MASK)) | divisorValue;

    /* Reset FIFO */
    HwReg(gS5L8930XUartBase + UFCON) = UART_FIFO_RESET_RX | UART_FIFO_RESET_TX;

    /* Enable FIFO */
    HwReg(gS5L8930XUartBase + UFCON) = UART_FIFO_ENABLE;

    PE_kputc = S5L8930X_putc;

    kprintf(KPRINTF_PREFIX "serial is up\n");

    return;
}

void S5L8930X_interrupt_init(void)
{
    /* Disable interrupts */
    ml_set_interrupts_enabled(FALSE);

    /* Goddamn am I paranoid. */
    assert(gS5L8930XVic0Base && gS5L8930XVic1Base &&
           gS5L8930XVic2Base && gS5L8930XVic3Base);

    /* Disable all interrupts. */
    HwReg(gS5L8930XVic0Base + VICINTENCLEAR) = 0xFFFFFFFF;
    HwReg(gS5L8930XVic1Base + VICINTENCLEAR) = 0xFFFFFFFF;
    HwReg(gS5L8930XVic2Base + VICINTENCLEAR) = 0xFFFFFFFF;
    HwReg(gS5L8930XVic3Base + VICINTENCLEAR) = 0xFFFFFFFF;

    HwReg(gS5L8930XVic0Base + VICINTENABLE) = 0;
    HwReg(gS5L8930XVic1Base + VICINTENABLE) = 0;
    HwReg(gS5L8930XVic2Base + VICINTENABLE) = 0;
    HwReg(gS5L8930XVic3Base + VICINTENABLE) = 0;

    /* Please use IRQs. I don't want to implement a FIQ based timer decrementer handler. */
    HwReg(gS5L8930XVic0Base + VICINTSELECT) = 0;
    HwReg(gS5L8930XVic1Base + VICINTSELECT) = 0;
    HwReg(gS5L8930XVic2Base + VICINTSELECT) = 0;
    HwReg(gS5L8930XVic3Base + VICINTSELECT) = 0;

    /* Unmask all interrupt levels. */
    HwReg(gS5L8930XVic0Base + VICSWPRIORITYMASK) = 0xFFFF;
    HwReg(gS5L8930XVic1Base + VICSWPRIORITYMASK) = 0xFFFF;
    HwReg(gS5L8930XVic2Base + VICSWPRIORITYMASK) = 0xFFFF;
    HwReg(gS5L8930XVic3Base + VICSWPRIORITYMASK) = 0xFFFF;

    /* Set vector addresses to interrupt numbers. */
    int i;
    for(i = 0; i < 0x20; i++) {
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

    /* Set rtclock stuff */
    timer_configure();

    /* Disable the timer. */
    S5L8930X_timer_enabled(FALSE);

    /* Enable the interrupt. */
    HwReg(gS5L8930XVic0Base + VICINTENABLE) = 
        HwReg(gS5L8930XVic0Base + VICINTENABLE) | (1 << 5) | (1 << 6);

    /* Enable interrupts. */
    ml_set_interrupts_enabled(TRUE);

    /* Wait for it. */
    kprintf(KPRINTF_PREFIX "waiting for system timer to come up...\n");
    S5L8930X_timer_enabled(TRUE);

    clock_initialized = TRUE;
    
    while(!clock_had_irq)
        barrier();

    return;
}

void S5L8930X_handle_interrupt(void* context)
{
    /* Timer IRQs are handeled by us. */
    if(HwReg(gS5L8930XVic0Base + VICADDRESS) == 6) {
        /* Disable timer */
        S5L8930X_timer_enabled(FALSE);

        /* Update absolute time */
        clock_absolute_time += (clock_decrementer - S5L8930X_timer_value());
    
        /* EOI. */
        HwReg(gS5L8930XVic0Base + VICADDRESS) = 0;

        /* Enable timer. */
        S5L8930X_timer_enabled(TRUE);

        /* We had an IRQ. */
        clock_had_irq = TRUE;
    }
    return;
}

uint64_t S5L8930X_get_timebase(void)
{
    uint32_t timestamp;
    
    if(!clock_initialized)
        return 0;
    
    timestamp = S5L8930X_timer_value();

    if(timestamp) {
        uint64_t v = clock_absolute_time;
        v += (uint64_t)(((uint64_t)clock_decrementer) - (uint64_t)(timestamp));
        return v;
    } else {
        clock_absolute_time += clock_decrementer;
        return clock_absolute_time;
    }
}

uint64_t S5L8930X_timer_value(void)
{
    return HwReg(gS5L8930XTimerBase + TIMER0_VAL);
}

void S5L8930X_timer_enabled(int enable)
{
    /* How do you disable this timer? */
    if(!enable) {
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

static void _fb_putc(int c) {
    if(c == '\n') {
        vcputc(0, 0, '\r');
    }
    vcputc(0, 0, c);
    S5L8930X_putc(c);
}

void S5L8930X_InitCaches(void)
{
    kprintf(KPRINTF_PREFIX "initializing i+dcache\n");
    cache_initialize();
    kprintf(KPRINTF_PREFIX "done\n");
}

void S5L8930X_framebuffer_init(void)
{
    /* Technically, iBoot should initialize this.. */
    PE_state.video.v_depth = 4 * (8);   // 32bpp
    
    kprintf(KPRINTF_PREFIX "framebuffer initialized\n");

    initialize_screen((void*)&PE_state.video, kPEAcquireScreen);
    initialize_screen((void*)&PE_state.video, kPEEnableScreen);
    return;
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
    
    if(PE_parse_boot_argn("-no-cache", tempbuf, sizeof(tempbuf))) {
        kprintf(KPRINTF_PREFIX "No caching enabled (I+D).\n");
    } else {
        S5L8930X_InitCaches();
    }

    S5L8930X_framebuffer_init();
    S5L8930X_uart_init();

    PE_kputc = _fb_putc; //gPESocDispatch.uart_putc;
}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for S5L8930X\n");
    PE_init_SocSupport_S5L8930X();
}

#endif
