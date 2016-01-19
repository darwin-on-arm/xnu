/*
 * Copyright 2015, Vince Cali. <0x56.0x69.0x6e.0x63.0x65@gmail.com>
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
 * Platform Expert for Freescale i.MX53.
 */

#if defined(BOARD_CONFIG_IMX53)

#include <pexpert/pexpert.h>
#include <pexpert/arm/protos.h>
#include <pexpert/arm/boot.h>
#include <machine/machine_routines.h>
#include <vm/pmap.h>

#include "pe_imx53.h"

#define HwReg32(x) *((volatile uint32_t*)(x))

#define KPRINTF_PREFIX  "PE_imx53: "

#define IMX53_UART_BAUDRATE  115200

extern void rtclock_intr(arm_saved_state_t * regs);
extern void rtc_configure(uint64_t hz);

uint64_t imx53_timer_value(void);
void imx53_timer_enabled(int enable);

static boolean_t clock_initialized = FALSE;
static boolean_t clock_had_irq = FALSE;
static uint64_t clock_decrementer = 0;
static uint64_t clock_absolute_time = 0;

static vm_offset_t gIMX53TZICBase = 0;
static vm_offset_t gIMX53TimerBase = 0;
static vm_offset_t gIMX53SerialUartBase = 0;

static void timer_configure(void)
{
    /*
     * xxx hack for etimer since it does not know time yet 
     */

    //XXX: are these values sane? Hz might be a bit off, not 100% sure what the clock rate is or if the prescaler setting is correct
    uint64_t hz = 1000000; //XXX: this is not completely accurate, rounding for now
    gPEClockFrequencyInfo.timebase_frequency_hz = hz;

    clock_decrementer = 5000;
    kprintf(KPRINTF_PREFIX "decrementer frequency = %llu\n", clock_decrementer);

    rtc_configure(hz);
    return;
}

void imx53_putc(int c)
{
    if (c == '\n')
        imx53_putc('\r');

    while (regread32(gIMX53SerialUartBase + UART_UTS) & UART_UTS_TXFULL)
        /* TxFIFO is full, wait */
        barrier();

    regwrite32((gIMX53SerialUartBase + UART_UTXD), c);
}

int imx53_getc(void)
{
    int c = -1;

    for (int i = 0x80000; i > 0; i--) {
        // first check if RxFIFO is not empty
        if (!(regread32(gIMX53SerialUartBase + UART_UTS) & UART_UTS_RXEMPTY)) {
            uint32_t r = regread32(gIMX53SerialUartBase + UART_URXD);
            // then check for valid char (XXX paranoid ?)
            if (r & UART_URXD_CHARRDY) {
                c = r & 0xFF;
                break;
            }
        }
    }

    return c;
}

void imx53_uart_init(void)
{
    gIMX53SerialUartBase = ml_io_map(UART1_BASE, PAGE_SIZE);
    assert(gIMX53SerialUartBase);

    //TODO: use whatever U-Boot sets up for now
}

void imx53_interrupt_init(void)
{
    ml_set_interrupts_enabled(FALSE);

    gIMX53TZICBase = ml_io_map(TZIC_BASE, PAGE_SIZE);
    assert(gIMX53TZICBase);

    /* set interrupts as non-secure */
    regwrite32(gIMX53TZICBase + TZIC_INTSEC0, ~0);
    regwrite32(gIMX53TZICBase + TZIC_INTSEC1, ~0);
    regwrite32(gIMX53TZICBase + TZIC_INTSEC2, ~0);
    regwrite32(gIMX53TZICBase + TZIC_INTSEC3, ~0);

    /* disable all interrupt sources */
    regwrite32(gIMX53TZICBase + TZIC_ENCLEAR0, ~0);
    regwrite32(gIMX53TZICBase + TZIC_ENCLEAR1, ~0);
    regwrite32(gIMX53TZICBase + TZIC_ENCLEAR2, ~0);
    regwrite32(gIMX53TZICBase + TZIC_ENCLEAR3, ~0);

    /* somewhat arbitrarily chosen priority mask */
    regwrite32(gIMX53TZICBase + TZIC_PRIOMASK, 0x1F);

    /* enable non-secure interrupts */
    regwrite32(gIMX53TZICBase + TZIC_INTCTRL, TZIC_INTCTRL_EN | TZIC_INTCTRL_NSEN | TZIC_INTCTRL_NSENMASK);

    barrier();
}

void imx53_timebase_init(void)
{
    timer_configure();

    gIMX53TimerBase = ml_io_map(EPIT1_BASE, PAGE_SIZE);
    assert(gIMX53TimerBase);

    /* ensure timer is disabled before initialization */
    imx53_timer_enabled(FALSE);
    
    /* set output mode to disabled */
    uint32_t r = regread32(gIMX53TimerBase + EPIT_EPITCR);
    regwrite32(gIMX53TimerBase + EPIT_EPITCR, r & ~(3 << 22));

    /* select clock source */
    r = regread32(gIMX53TimerBase + EPIT_EPITCR);
    r |= 2 << 24; //XXX: high frequency reference clock, is this 133 or 110MHz (or some other value) ???
    regwrite32(gIMX53TimerBase + EPIT_EPITCR, r);
    
    /* clear output compare flag */
    regwrite32(gIMX53TimerBase + EPIT_EPITSR, EPIT_EPITSR_OCIF);

    // IRQ_EPIT_1 == 40
    regwrite32(gIMX53TZICBase + TZIC_ENSET1, (1 << 8));
    //XXX: what should the priority be?
    regwrite32(gIMX53TZICBase + TZIC_PRIORITY10, 0x00000010);

    /* set load value to decrementer */
    regwrite32(gIMX53TimerBase + EPIT_EPITLR, clock_decrementer);

    /* bring counter to defined state */
    r = regread32(gIMX53TimerBase + EPIT_EPITCR);
    regwrite32(gIMX53TimerBase + EPIT_EPITCR, r | EPIT_EPITCR_ENMOD);

    /* set clock prescaler value */
    r = regread32(gIMX53TimerBase + EPIT_EPITCR);
    r |= 0x85 << 4; //XXX: clock/133 I think ???
    regwrite32(gIMX53TimerBase + EPIT_EPITCR, r);

    /* enable output compare event */
    r = regread32(gIMX53TimerBase + EPIT_EPITCR);
    regwrite32(gIMX53TimerBase + EPIT_EPITCR, r | EPIT_EPITCR_OCIEN);
    regwrite32(gIMX53TimerBase + EPIT_EPITCMPR, clock_decrementer);

    ml_set_interrupts_enabled(TRUE);

    imx53_timer_enabled(TRUE);

    clock_initialized = TRUE;

    kprintf(KPRINTF_PREFIX "waiting for timer...\n");
    while (!clock_had_irq)
        barrier();

    kprintf(KPRINTF_PREFIX "timer is now up, ticks %llu\n", imx53_timer_value());
}

void imx53_handle_interrupt(void *context)
{
    //TODO: actually figure out which IRQ is pending
    
    if (regread32(gIMX53TZICBase + TZIC_SRCSET1) & (1 << 8)) {
        imx53_timer_enabled(FALSE);

        rtclock_intr((arm_saved_state_t *) context);

        clock_absolute_time += (clock_decrementer - imx53_timer_value());

        /* deassert interrupt by clearing timer output compare interrupt flag */
        regwrite32(gIMX53TimerBase + EPIT_EPITSR, EPIT_EPITSR_OCIF);

        imx53_timer_enabled(TRUE);

        clock_had_irq = TRUE;
    }
    else {
        //TODO
        //irq_iokit_dispatch(irq_number);
    }
}

uint64_t imx53_get_timebase(void)
{
    uint32_t timestamp;

    if (!clock_initialized)
        return 0;

    timestamp = imx53_timer_value();
    if (timestamp) {
        uint64_t v = clock_absolute_time;
        v += (uint64_t) (((uint64_t) clock_decrementer) - (uint64_t) (timestamp));
        return v;
    }
    else {
        clock_absolute_time += clock_decrementer;
        return clock_absolute_time;
    } 
}

uint64_t imx53_timer_value(void)
{
    return regread32(gIMX53TimerBase + EPIT_EPITCNR);
}

void imx53_timer_enabled(int enable)
{
    uint32_t r = regread32(gIMX53TimerBase + EPIT_EPITCR);

    if (enable)
        regwrite32(gIMX53TimerBase + EPIT_EPITCR, r | EPIT_EPITCR_EN);
    else
        regwrite32(gIMX53TimerBase + EPIT_EPITCR, r & ~EPIT_EPITCR_EN);
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
    imx53_putc(c);
}

void imx53_framebuffer_init(void)
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

    initialize_screen((void *) &PE_state.video, kPETextMode);
}


/*
 * Setup the i.MX53 SoC dispatch table
 */
void PE_init_SocSupport_imx53(void)
{
    gPESocDispatch.uart_getc = imx53_getc;
    gPESocDispatch.uart_putc = imx53_putc;
    gPESocDispatch.uart_init = imx53_uart_init;

    gPESocDispatch.interrupt_init = imx53_interrupt_init;
    gPESocDispatch.timebase_init = imx53_timebase_init;

    gPESocDispatch.get_timebase = imx53_get_timebase;

    gPESocDispatch.handle_interrupt = imx53_handle_interrupt;

    gPESocDispatch.timer_value = imx53_timer_value;
    gPESocDispatch.timer_enabled = imx53_timer_enabled;

    gPESocDispatch.framebuffer_init = imx53_framebuffer_init;

    imx53_uart_init();
    imx53_framebuffer_init();
}

/*
 * Initialize SoC support for i.MX53.
 */
void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for Freescale i.MX53\n");
    PE_init_SocSupport_imx53();
}

#endif /* !BOARD_CONFIG_IMX53 */
