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
 * Platform Expert for OMAP35xx/36xx and AM/DM37x.
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
#include <kern/cpu_data.h>

/* XXX: timer is so god awfully borked */

#ifdef BOARD_CONFIG_OMAP3530

void Omap3_timer_enabled(int enable);
uint64_t Omap3_timer_value(void);
uint64_t Omap3_get_timebase(void);

#include "omap3530.h"

#define mmio_read(a)    (*(volatile uint32_t *)(a))
#define mmio_write(a,v) (*(volatile uint32_t *)(a) = (v))
#define mmio_set(a,v)   mmio_write((a), mmio_read((a)) | (v))
#define mmio_clear(a,v) mmio_write((a), mmio_read((a)) & ~(v))

#define HwReg(x) *((volatile unsigned long*)(x))

#define KPRINTF_PREFIX  "PE_omap3530: "

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

    clock_decrementer = 500;
    kprintf(KPRINTF_PREFIX "decrementer frequency = %llu\n", clock_decrementer);

    rtc_configure(hz);
    return;
}

void Omap3_early_putc(int c)
{
    if (c == '\n')
        Omap3_early_putc('\r');

    while (!(HwReg(OMAP3_UART_BASE + LSR) & LSR_THRE))
        barrier();

    HwReg(OMAP3_UART_BASE + THR) = c;
}

void Omap3_putc(int c)
{
    if (!gOmapSerialUartBase)
        return;

    if (c == '\n')
        Omap3_putc('\r');

    while (!(HwReg(gOmapSerialUartBase + LSR) & LSR_THRE))
        barrier();

    HwReg(gOmapSerialUartBase + THR) = c;
}

int Omap3_getc(void)
{
    int i = 0x20000;
    while (!(HwReg(gOmapSerialUartBase + LSR) & LSR_DR)) {
        i--; if(!i) return -1;
    }

    return (HwReg(gOmapSerialUartBase + RBR));
}

void Omap3_uart_init(void)
{
    gOmapTimerBase = ml_io_map(OMAP3_TIMER0_BASE, PAGE_SIZE);
    gOmapInterruptControllerBase = ml_io_map(OMAP3_GIC_BASE, PAGE_SIZE);
    gOmapDisplayControllerBase = ml_io_map(OMAP3_DSS_BASE - 0x40, PAGE_SIZE);

    /*
     * XXX: God. 
     */
    gOmapPrcmBase = ml_io_map(0x48004000, PAGE_SIZE);

    int baudDivisor;
    gOmapSerialUartBase = ml_io_map(OMAP3_UART_BASE, PAGE_SIZE);

    assert(OMAP3_UART_BAUDRATE != 0);
    baudDivisor = (OMAP3_UART_CLOCK / 16 / OMAP3_UART_BAUDRATE);

    HwReg(gOmapSerialUartBase + IER) = 0x00;
    HwReg(gOmapSerialUartBase + LCR) = LCR_BKSE | LCRVAL;
    HwReg(gOmapSerialUartBase + DLL) = baudDivisor & 0xFF;
    HwReg(gOmapSerialUartBase + DLM) = (baudDivisor >> 8) & 0xFF;
    HwReg(gOmapSerialUartBase + LCR) = LCRVAL;
    HwReg(gOmapSerialUartBase + MCR) = MCRVAL;
    HwReg(gOmapSerialUartBase + FCR) = FCRVAL;
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
    HwReg(INTCPS_MIR(0)) = 0xffffffff;
    HwReg(INTCPS_MIR(1)) = 0xffffffff;
    HwReg(INTCPS_MIR(2)) = 0xffffffff;

    /*
     * Set the true bits 
     */
    mmio_write(INTCPS_MIR_CLEAR(37 >> 5), 1 << (37 & 0x1f));

    /*
     * Set enable new IRQs/FIQs 
     */
    HwReg(INTCPS_CONTROL) = (1 << 0);

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
    HwReg(gOmapTimerBase + TLDR) = 0xffffffe0;
    HwReg(gOmapTimerBase + TCRR) = 0xffffffe0;

    HwReg(gOmapTimerBase + TPIR) = 232000;
    HwReg(gOmapTimerBase + TNIR) = -768000;

    HwReg(gOmapTimerBase + TOCR) = 0;
    HwReg(gOmapTimerBase + TOWR) = 100;

    HwReg(gOmapTimerBase + TCLR) = (1 << 6);

    /*
     * !!! SET INTERRUPTS ENABLED ON OVERFLOW 
     */
    HwReg(gOmapTimerBase + TISR) = 0x7;
    HwReg(gOmapTimerBase + TIER) = 0x7;

    /*
     * Set to 32KHz 
     */
    mmio_set(gOmapPrcmBase + 0xc40, 0x40);

    /*
     * Arm the timer 
     */
    HwReg(gOmapTimerBase + TCLR) = (1 << 0) | (1 << 1) | (2 << 10);

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
    uint32_t irq_number = (HwReg(INTCPS_SIR_IRQ)) & 0x7F;

    if (irq_number == 37) {     /* GPTimer1 IRQ */
        /*
         * Stop the timer 
         */
        Omap3_timer_enabled(FALSE);

        /*
         * Clear interrupt status 
         */
        HwReg(gOmapTimerBase + TISR) = 0x7;

        /*
         * FFFFF 
         */
        rtclock_intr((arm_saved_state_t *) context);

        /*
         * Set new IRQ generation 
         */
        HwReg(INTCPS_CONTROL) = 0x1;

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
    return 0xffffffff - (HwReg(gOmapTimerBase + TCRR));
}

void Omap3_timer_enabled(int enable)
{
    /*
     * Clear the TCLR [ST] bit 
     */
    if (enable)
        HwReg(gOmapTimerBase + TCLR) |= (1 << 0);
    else
        HwReg(gOmapTimerBase + TCLR) &= ~(1 << 0);

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

struct video_mode {
    short width, height;
    char *name;
    uint32_t dispc_size;
    uint32_t dispc_timing_h;
    uint32_t dispc_timing_v;
    uint32_t dispc_divisor;
    uint32_t dss_divisor;
};

/* DISPC_TIMING_H bits and masks */
#define DISPCB_HBP 20
#define DISPCB_HFP 8
#define DISPCB_HSW 0
#define DISPCM_HBP 0xfff00000
#define DISPCM_HFP 0x000fff00
#define DISPCM_HSW 0x000000ff

/* DISPC_TIMING_V bits and masks */
#define DISPCB_VBP 20
#define DISPCB_VFP 8
#define DISPCB_VSW 0
#define DISPCM_VBP 0x0ff00000
#define DISPCM_VFP 0x0000ff00
#define DISPCM_VSW 0x0000003f

// Master clock is 864 Mhz, and changing it is a pita since it cascades to other
// devices.
// Pixel clock is  864 / cm_clksel_dss.dss1_alwan_fclk / dispc_divisor.divisor.pcd
// So most of these modes are just approximate.
// List must be in ascending order.
struct video_mode modes[] = {
    // 640x480@72    31.500        640    24    40    128      480    9    3    28
    {
        640, 480, "640x480-71",
        ((480-1) << 16) | (640-1),
        (128 << DISPCB_HBP) | (24 << DISPCB_HFP) | (40 << DISPCB_HSW),
        (28 << DISPCB_VBP) | (9 << DISPCB_VFP) | (3 << DISPCB_VSW),
        2, 14
    },
    //      800x600, 60Hz      40.000      800    40    128    88      600    1    4    23
    {
        800, 600, "800x600-59",
        ((600-1) << 16) | (800-1),
        (88 << DISPCB_HBP) | (40 << DISPCB_HFP) | (128 << DISPCB_HSW),
        (23 << DISPCB_VBP) | (1 << DISPCB_VFP) | (4 << DISPCB_VSW),
        2, 11
    },             
    // 1024x768, 60Hz      65.000      1024    24    136    160      768    3    6    29
    {
        1024, 768, "1024x768-61",
        ((768-1) << 16) | (1024-1),
        (160 << DISPCB_HBP) | (24 << DISPCB_HFP) | (136 << DISPCB_HSW),
        (29 << DISPCB_VBP) | (3 << DISPCB_VFP) | (6 << DISPCB_VSW),
        1, 13
    },
    {
        1280, 1024, "1280x1024-60",
        ((1024-1) << 16) | (1280-1),
        (248 << DISPCB_HBP) | (48 << DISPCB_HFP) | (112 << DISPCB_HSW),
        (38 << DISPCB_VBP) | (1 << DISPCB_VFP) | (3 << DISPCB_VSW),
        1, 8
    },
    /* 1280x800, dotclock 83.46MHz, approximated 86.4MHz */
    {
        1280, 800, "1280x800-60",
        ((800 - 1) << 16) | (1280 - 1),
        (200 << DISPCB_HBP) | (64 < DISPCB_HBP) | (136 << DISPCB_HSW),
        (24 << DISPCB_VBP) | (1 << DISPCB_VFP) | (3 << DISPCB_HSW),
        2, 5
    },
    /* 1366x768, dotclock 85.86MHz, approximated 86.4MHz */
    {
        1366, 768, "1366x768-60",
        ((768 - 1) << 16) | (1366 - 1),
        (216 << DISPCB_HBP) | (72 < DISPCB_HBP) | (144 << DISPCB_HSW),
        (23 << DISPCB_VBP) | (1 << DISPCB_VFP) | (3 << DISPCB_HSW),
        2, 5
    },
    /* 1440x900, dotclock 106.47MHz, approximated 108MHz */
    {
        1440, 900, "1440x900-60",
        ((900 - 1) << 16) | (1440 - 1),
        (232 << DISPCB_HBP) | (80 < DISPCB_HBP) | (152 << DISPCB_HSW),
        (28 << DISPCB_VBP) | (1 << DISPCB_VFP) | (28 << DISPCB_HSW),
        2, 4
    },
};

void Omap3_framebuffer_init(void)
{
    int dont_mess_with_this = 0;
    char tempbuf[16];

    if (PE_parse_boot_argn("-dont-fuck-with-framebuffer", tempbuf, sizeof(tempbuf))) {
        /* Do not fuck with the framebuffer whatsoever, rely on whatever u-boot set. */
        dont_mess_with_this = 1;
    }

    /*
     * This *must* be page aligned. 
     */
    struct dispc_regs *OmapDispc = (struct dispc_regs *) (gOmapDisplayControllerBase + 0x440);

    /*
     * Set defaults 
     */
    uint32_t timing_h, timing_v;
    uint32_t hbp, hfp, hsw, vsw, vfp, vbp;

    char tmpbuf[16];
    struct video_mode *current_mode = &modes[0];

    if (PE_parse_boot_argn("omapfbres", tmpbuf, sizeof(tmpbuf))) {
        int cur = 0;
        while (cur != (sizeof(modes) / sizeof(struct video_mode))) {
            if (!strcmp(tmpbuf, modes[cur].name)) {
                current_mode = &modes[cur];
                break;
            }
            cur++;
        }
    }

    uint32_t vs;
    if(!dont_mess_with_this) {
        OmapDispc->size_lcd = current_mode->dispc_size;
        OmapDispc->timing_h = current_mode->dispc_timing_h;
        OmapDispc->timing_v = current_mode->dispc_timing_v;
        OmapDispc->pol_freq = 0x00007028;
        OmapDispc->divisor = current_mode->dispc_divisor;
        HwReg(gOmapPrcmBase + 0xE00 + 0x40) = current_mode->dss_divisor;
        OmapDispc->config = (2 << 1);
        OmapDispc->default_color0 = 0xffff0000;
        OmapDispc->control = ((1 << 3) | (3 << 8));

        /*
         * Initialize display control 
         */
        OmapDispc->control |= DISPC_ENABLE;
        OmapDispc->default_color0 = 0xffff0000;

        /*
         * initialize lcd defaults 
         */
        barrier();
    }

    vs = OmapDispc->size_lcd;
    uint32_t lcd_width, lcd_height;

    lcd_height = (vs >> 16) + 1;
    lcd_width = (vs & 0xffff) + 1;
    kprintf(KPRINTF_PREFIX "lcd size is %u x %u\n", lcd_width, lcd_height);

    /*
     * Allocate framebuffer 
     */
    void *framebuffer = pmap_steal_memory(lcd_width * lcd_width * 4);
    void *framebuffer_phys = pmap_extract(kernel_pmap, framebuffer);
    bzero(framebuffer, lcd_width * lcd_height * 4);
    kprintf(KPRINTF_PREFIX "software framebuffer at %p\n", framebuffer);

    /*
     * Set attributes in display controller 
     */
    OmapDispc->gfx_ba0 = framebuffer_phys;
    OmapDispc->gfx_ba1 = 0;
    OmapDispc->gfx_position = 0;
    OmapDispc->gfx_row_inc = 1;
    OmapDispc->gfx_pixel_inc = 1;
    OmapDispc->gfx_window_skip = 0;
    OmapDispc->gfx_size = vs;
    OmapDispc->gfx_attributes = 0x91;

    /*
     * Enable the display 
     */
    *((volatile unsigned long *) (&OmapDispc->control)) |= 1 | (1 << 1) | (1 << 5) | (1 << 6) | (1 << 15) | (1 << 16);

    /*
     * Hook to our framebuffer 
     */
    PE_state.video.v_baseAddr = (unsigned long) framebuffer_phys;
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
        /*
         * BootX like framebuffer. 
         */
        memset(framebuffer, 0xb9, lcd_width * lcd_height * 4);
        initialize_screen((void *) &PE_state.video, kPEGraphicsMode);
    } else {
        initialize_screen((void *) &PE_state.video, kPETextMode);
    }

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

    Omap3_uart_init();
    PE_kputc = gPESocDispatch.uart_putc;

    Omap3_framebuffer_init();
}

void PE_init_SocSupport_stub(void)
{
    PE_early_puts("PE_init_SocSupport: Initializing for OMAP3530\n");
    PE_init_SocSupport_omap3();
}

#endif
