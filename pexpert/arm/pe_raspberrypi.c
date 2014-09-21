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

#include "raspberrypi.h"

/*
 * This is board specific stuff.
 */
#ifdef BOARD_CONFIG_RASPBERRYPI
#define KPRINTF_PREFIX  "PE_RaspberryPi: "

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

#define gpio_base   gRaspberryPiGPIOBase
vm_offset_t     gRaspberryPiGPIOBase;
#define uart_base   gRaspberryPiUartBase
vm_offset_t     gRaspberryPiUartBase;
#define systimer_base   gRaspberryPiSystimerBase
vm_offset_t     gRaspberryPiSystimerBase;
#define mb_base   gRaspberryPiMailboxBase
vm_offset_t       gRaspberryPiMailboxBase;

struct fb_info __attribute__((aligned(16))) gFBInfo;

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
    while(!GET32(uart_base + AUX_MU_LSR_REG) & 0x20)
        barrier();
    PUT32(uart_base + AUX_MU_IO_REG, c);
}

int RaspberryPi_getc(void)
{
    while(!GET32(uart_base + AUX_MU_LSR_REG) & 0x01)
        barrier();
    return GET32(uart_base + AUX_MU_IO_REG);
}

void RaspberryPi_uart_init(void)
{
    unsigned int i;
	gRaspberryPiUartBase = ml_io_map(AUX_BASE, PAGE_SIZE);
    gRaspberryPiGPIOBase = ml_io_map(GP_BASE, PAGE_SIZE);

    PUT32(uart_base + AUX_ENABLES,       1);
    PUT32(uart_base + AUX_MU_IER_REG,    0);
    PUT32(uart_base + AUX_MU_CNTL_REG,   0);
    PUT32(uart_base + AUX_MU_LCR_REG,    3);
    PUT32(uart_base + AUX_MU_MCR_REG,    0);
    PUT32(uart_base + AUX_MU_IER_REG,    0);
    PUT32(uart_base + AUX_MU_IIR_REG, 0xC6);
    PUT32(uart_base + AUX_MU_BAUD_REG, 270); // 115200 bps

    i = GET32(gpio_base + GPFSEL1);
    i &= ~(7<<12); // Configure GPIO14 as..
    i |= 2<<12; // ..TXD for UART
    i &= ~(7<<15); // Configure GPIO15 as..
    i |= 2<<15; // ..RXD for UART
    PUT32(gpio_base + GPFSEL1, i);

    PUT32(gpio_base + GPPUD, 0);
    for(i = 0; i<150; i++) barrier();
    PUT32(gpio_base + GPPUDCLK0, (1<<14) | (1<<15));
    for(i = 0; i<150; i++) barrier();
    PUT32(gpio_base + GPPUDCLK0, 0);

    PUT32(uart_base + AUX_MU_CNTL_REG, 3);

    kprintf(KPRINTF_PREFIX "serial is up\n");
}

void RaspberryPi_interrupt_init(void)
{
    return;
}

void RaspberryPi_timebase_init(void)
{
    gRaspberryPiSystimerBase = ml_io_map(SYSTIMER_BASE, PAGE_SIZE);
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
    return GET64(systimer_base + SYSTIMERCLK); // free-running 64 bit timer, about 1 MHz
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

void mb_send(uint8_t num, uint32_t msg)
{
    while(!GET32(mb_base + MB_STATUS) & 0x80000000)
        barrier();
    PUT32(mb_base + MB_WRITE, num & 0xf | msg & 0xfffffff0);
    //  0  1  2  3  4  5  6  ..  31
    // [0  1  2  3][4  5  6  ..  31] Like this..
    // [0  1  2  3][0  1  2  ..  27] ..but not like this
    // [    num   ][      msg      ]
}

uint32_t mb_read(uint8_t num)
{
    while(1)
    {
        while(!GET32(mb_base + MB_STATUS) & 0x40000000)
            barrier();
        if((GET32(mb_base + MB_READ) & 0xf) == (num & 0xf))
            return GET32(mb_base + MB_READ) & 0xfffffff0;
    }
}

void RaspberryPi_framebuffer_init(void)
{
    gRaspberryPiMailboxBase = ml_io_map(MAILBOX_BASE, PAGE_SIZE);
    gFBInfo.phys_w = 1280;
    gFBInfo.phys_h = 768;
    gFBInfo.virt_w = gFBInfo.phys_w;
    gFBInfo.virt_h = gFBInfo.phys_h;
    gFBInfo.bpp = 4 * (8); // 32bpp

    mb_send(1, pmap_extract(kernel_pmap, &gFBInfo) + 0x40000000);
    if(mb_read(1) == 0)
    {
        PE_state.video.v_baseAddr = (unsigned long) ml_io_map((void*) gFBInfo.gpu_ptr, PAGE_SIZE);
        PE_state.video.v_rowBytes = gFBInfo.phys_w * (gFBInfo.bpp / 8);
        PE_state.video.v_width = gFBInfo.phys_w;
        PE_state.video.v_height = gFBInfo.phys_h;
        PE_state.video.v_depth = gFBInfo.bpp;

        kprintf(KPRINTF_PREFIX "framebuffer initialized\n");

        initialize_screen((void*)&PE_state.video, kPEAcquireScreen);
        initialize_screen((void*)&PE_state.video, kPEEnableScreen);
    }
    else
    {
        kprintf(KPRINTF_PREFIX "Failed to get frambuffer :-/\n");
    }
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
    
    PE_kputc = _fb_putc; //gPESocDispatch.uart_putc;
    
}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for RASPBERRYPI\n");
    PE_init_SocSupport_raspberrypi();
}

#endif
