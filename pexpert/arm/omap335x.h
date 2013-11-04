/*
 * OMAP 35xx support
 */

#ifndef xnu_omap335x_h
#define xnu_omap335x_h

/*
 * Note, on older OMAP platforms, the size of the NS16550 UARTS
 * is different, for this one it's 32-bit.
 */

#define RBR     0x0
#define IER     0x4
#define FCR     0x8
#define LCR     0xC
#define MCR     0x10
#define LSR     0x14            // 335x
#define MSR     0x18
#define SCR     0x1C
#define SSR     0x44            // 335x

#define barrier()               __asm__ __volatile__("": : :"memory");

#define OMAP3_GIC_BASE           0x48200000

#define OMAP3_L4_WKUP_PERIPH_BASE    0x44C00000
#define OMAP3_L4_PERIPH_BASE    0x48000000
#define OMAP3_UART_BASE         (OMAP3_L4_WKUP_PERIPH_BASE + 0x209000)  // This is uart0 (default usb on beaglebone)

#define OMAP3_UART_CLOCK        48000000
#define OMAP3_UART_BAUDRATE     115200

#define LCR_BKSE	0x80        /* Bank select enable */
#define LCR_8N1		0x03

#define SSR_TXFIFOFULL	0x01    /* 335x */

#define LSR_DR		0x01        /* Data ready */
#define LSR_THRE	0x20        /* Xmit holding register empty */

#define MCR_DTR         0x01
#define MCR_RTS         0x02

#define FCR_FIFO_EN     0x01    /* Fifo enable */
#define FCR_RXSR        0x02    /* Receiver soft reset */
#define FCR_TXSR        0x04    /* Transmitter soft reset */

#define LCRVAL LCR_8N1          /* 8 data, 1 stop, no parity */
#define MCRVAL (MCR_DTR | MCR_RTS)  /* RTS/DTR */
#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)  /* Clear & enable FIFOs */

#define THR     RBR
#define DLL     RBR
#define DLM     IER

/*
 * gPIC configuration
 */
#define INT_NROF_VECTORS      (96)
#define MAX_VECTOR            (INT_NROF_VECTORS - 1)
#define INTCPS_SYSCONFIG      (gOmapInterruptControllerBase + 0x0010)
#define INTCPS_SYSSTATUS      (gOmapInterruptControllerBase + 0x0014)
#define INTCPS_SIR_IRQ        (gOmapInterruptControllerBase + 0x0040)
#define INTCPS_SIR_IFQ        (gOmapInterruptControllerBase + 0x0044)
#define INTCPS_CONTROL        (gOmapInterruptControllerBase + 0x0048)
#define INTCPS_PROTECTION     (gOmapInterruptControllerBase + 0x004C)
#define INTCPS_IDLE           (gOmapInterruptControllerBase + 0x0050)
#define INTCPS_IRQ_PRIORITY   (gOmapInterruptControllerBase + 0x0060)
#define INTCPS_FIQ_PRIORITY   (gOmapInterruptControllerBase + 0x0064)
#define INTCPS_THRESHOLD      (gOmapInterruptControllerBase + 0x0068)
#define INTCPS_ITR(n)         (gOmapInterruptControllerBase + 0x0080 + (0x20 * (n)))
#define INTCPS_MIR(n)         (gOmapInterruptControllerBase + 0x0084 + (0x20 * (n)))
#define INTCPS_MIR_CLEAR(n)   (gOmapInterruptControllerBase + 0x0088 + (0x20 * (n)))
#define INTCPS_MIR_SET(n)     (gOmapInterruptControllerBase + 0x008C + (0x20 * (n)))
#define INTCPS_ISR_SET(n)     (gOmapInterruptControllerBase + 0x0090 + (0x20 * (n)))
#define INTCPS_ISR_CLEAR(n)   (gOmapInterruptControllerBase + 0x0094 + (0x20 * (n)))
#define INTCPS_PENDING_IRQ(n) (gOmapInterruptControllerBase + 0x0098 + (0x20 * (n)))
#define INTCPS_PENDING_FIQ(n) (gOmapInterruptControllerBase + 0x009C + (0x20 * (n)))
#define INTCPS_ILR(m)         (gOmapInterruptControllerBase + 0x0100 + (0x04 * (m)))

/*
 * Timer.
 */

#define OMAP3_TIMER0_BASE       (OMAP3_L4_WKUP_PERIPH_BASE + 0x205000)  // 0x44E05000

#define OMAP3_TIMER1_BASE       (OMAP3_L4_WKUP_PERIPH_BASE + 0x231000)  // 0x44E31000
#define OMAP3_TIMER1_ENAB		0x44E004C4  // CM_WKUP 0x44E0_0400 + CM_WKUP_TIMER1_CLKCTRL    C4h . 0

#define OMAP3_TIMER2_BASE       (OMAP3_L4_PERIPH_BASE + 0x40000)    // 0x48040000
#define OMAP3_TIMER3_BASE       (OMAP3_L4_PERIPH_BASE + 0x42000)    // 0x48042000
#define OMAP3_TIMER4_BASE       (OMAP3_L4_PERIPH_BASE + 0x44000)    // 0x48044000
#define OMAP3_TIMER5_BASE       (OMAP3_L4_PERIPH_BASE + 0x46000)    // 0x48046000
#define OMAP3_TIMER6_BASE       (OMAP3_L4_PERIPH_BASE + 0x48000)    // 0x48048000
#define OMAP3_TIMER7_BASE       (OMAP3_L4_PERIPH_BASE + 0x4A000)    // 0x4804A000

#define OMAP3_TIMER0_IRQ		66
#define OMAP3_TIMER1_IRQ		67
#define OMAP3_TIMER2_IRQ		68
#define OMAP3_TIMER3_IRQ		69
#define OMAP3_TIMER4_IRQ		92
#define OMAP3_TIMER5_IRQ		93
#define OMAP3_TIMER6_IRQ		94
#define OMAP3_TIMER7_IRQ		95

#define OMAP335X_SCH_TIMER		1
#define OMAP335X_SCH_TIMER_BASE	OMAP3_TIMER1_BASE
#define OMAP335X_SCH_TIMER_IRQ	OMAP3_TIMER1_IRQ

#ifdef OMAP335X_SCH_TIMER==1

#define TIDR            0x0
#define TIOCP_CFG       0x10
#define TISTAT          0x14
#define TISR            0x18
#define TIER            0x1C
#define TWER            0x20
#define TCLR            0x24
#define TCRR            0x28
#define TLDR            0x2C
#define TTGR            0x30
#define TWPS            0x34
#define TMAR            0x38
#define TCAR1           0x3C
#define TSICR           0x40
#define TCAR2           0x44

/* Only for DMTIMER_1MS; Timer1 */
#define TPIR            0x48
#define TNIR            0x4C
#define TCVR            0x50
#define TOCR            0x54
#define TOWR            0x58

#else

#define TIDR            0x0
#define TIOCP_CFG       0x10
#define TISTAT          0x28
#define TISR            0x2C
#define TIER            0x30
#define TWER            0x34
#define TCLR            0x38
#define TCRR            0x3C
#define TLDR            0x40
#define TTGR            0x44
#define TWPS            0x48
#define TMAR            0x4C
#define TCAR1           0x50
#define TSICR           0x54
#define TCAR2           0x58

#endif

#endif
