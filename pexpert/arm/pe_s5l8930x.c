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

#define UART0_BASE          0x82500000
#define GPIO_BASE           0xBFA00000
#define FRAMEBUFFER_BASE    0x5F700000

#define ULCON      0x0000 /* Line Control             */
#define UCON       0x0004 /* Control                  */
#define UFCON      0x0008 /* FIFO Control             */
#define UMCON      0x000C /* Modem Control            */
#define UTRSTAT    0x0010 /* Tx/Rx Status             */
#define UERSTAT    0x0014 /* UART Error Status        */
#define UFSTAT     0x0018 /* FIFO Status              */
#define UMSTAT     0x001C /* Modem Status             */
#define UTXH       0x0020 /* Transmit Buffer          */
#define URXH       0x0024 /* Receive Buffer           */
#define UBRDIV     0x0028 /* Baud Rate Divisor        */
#define UFRACVAL   0x002C /* Divisor Fractional Value */
#define UINTP      0x0030 /* Interrupt Pending        */
#define UINTSP     0x0034 /* Interrupt Source Pending */
#define UINTM      0x0038 /* Interrupt Mask           */

#define UART_UFSTAT_TXFIFO_FULL (0x1 << 9)
#define UART_UTRSTAT_TRANSMITTEREMPTY 0x4
#define UART_UMSTAT_CTS 0x1

#define barrier()               __asm__ __volatile__("": : :"memory");

#define HwReg(x) *((volatile unsigned long*)(x))

extern void rtclock_intr(arm_saved_state_t* regs);
extern void rtc_configure(uint64_t hz);

#define uart_base   gS5L8930XUartBase
vm_offset_t     gS5L8930XUartBase;

static uint64_t     clock_decrementer = 0;
static boolean_t    clock_initialized = FALSE;
static boolean_t    clock_had_irq = FALSE;
static uint64_t     clock_absolute_time = 0;

static void timer_configure(void)
{
    return;
}

void S5L8930X_putc(int c)
{
    /* Wait for FIFO queue to empty. */
    while(HwReg(gS5L8930XUartBase + UFSTAT) & UART_UFSTAT_TXFIFO_FULL)
        barrier();
    
    /* Spin while transmitter not empty */
    while(HwReg(gS5L8930XUartBase + UTRSTAT) & UART_UTRSTAT_TRANSMITTEREMPTY)
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
    gS5L8930XUartBase = ml_io_map(UART0_BASE, PAGE_SIZE);
    return;
}

void S5L8930X_interrupt_init(void)
{
    return;
}

void S5L8930X_timebase_init(void)
{
    return;
}

void S5L8930X_handle_interrupt(void* context)
{
    return;
}

uint64_t S5L8930X_get_timebase(void)
{
    return 0;
}

uint64_t S5L8930X_timer_value(void)
{
    return 0;
}

void S5L8930X_timer_enabled(int enable)
{
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
#if 0
    S5L8930X_putc(c);
#endif
}

void S5L8930X_framebuffer_init(void)
{
    /* Technically, iBoot should initialize this.. */
    PE_state.video.v_baseAddr = (unsigned long)FRAMEBUFFER_BASE;
    PE_state.video.v_rowBytes = 640 * 4;
    PE_state.video.v_width = 640;
    PE_state.video.v_height = 960;
    PE_state.video.v_depth = 4 * (8);   // 16bpp
    
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
