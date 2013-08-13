/*
 * OMAP 35xx support
 */

#ifndef xnu_omap3530_h
#define xnu_omap3530_h

/*
 * Note, on older OMAP platforms, the size of the NS16550 UARTS
 * is different, for this one it's 32-bit.
 */

#define RBR     0x0
#define IER     0x4
#define FCR     0x8
#define LCR     0xC
#define MCR     0x10
#define LSR     0x14
#define MSR     0x18
#define SCR     0x1C

#define barrier()               __asm__ __volatile__("": : :"memory");

#define OMAP3_TIMER0_BASE        0x48318000
#define OMAP3_GIC_BASE           0x48200000

#define OMAP3_L4_PERIPH_BASE    0x49000000
#define OMAP3_UART_BASE         (OMAP3_L4_PERIPH_BASE + 0x20000)    // This is uart2

#define OMAP3_UART_CLOCK        48000000
#define OMAP3_UART_BAUDRATE     115200

#define LCR_BKSE	0x80                /* Bank select enable */
#define LCR_8N1		0x03

#define LSR_DR		0x01                /* Data ready */
#define LSR_THRE	0x20                /* Xmit holding register empty */

#define MCR_DTR         0x01
#define MCR_RTS         0x02

#define FCR_FIFO_EN     0x01            /* Fifo enable */
#define FCR_RXSR        0x02            /* Receiver soft reset */
#define FCR_TXSR        0x04            /* Transmitter soft reset */

#define LCRVAL LCR_8N1                              /* 8 data, 1 stop, no parity */
#define MCRVAL (MCR_DTR | MCR_RTS)                  /* RTS/DTR */
#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)	/* Clear & enable FIFOs */

#define THR     RBR
#define DLL     RBR
#define DLM     IER

/*
 * DSS/framebuffer stuff
 */
#define OMAP3_DSS_BASE      0x48050040
#define OMAP3_DISPC_BASE    0x48050440
#define OMAP3_VENC_BASE     0x48050C00

typedef uint32_t u32;

/* DSS Registers */
struct dss_regs {
    u32 control;                /* 0x40 */
    u32 sdi_control;            /* 0x44 */
    u32 pll_control;            /* 0x48 */
};

/* DISPC Registers */
struct dispc_regs {
    u32 control;                /* 0x40 */
    u32 config;             /* 0x44 */
    u32 reserve_2;              /* 0x48 */
    u32 default_color0;         /* 0x4C */
    u32 default_color1;         /* 0x50 */
    u32 trans_color0;           /* 0x54 */
    u32 trans_color1;           /* 0x58 */
    u32 line_status;            /* 0x5C */
    u32 line_number;            /* 0x60 */
    u32 timing_h;               /* 0x64 */
    u32 timing_v;               /* 0x68 */
    u32 pol_freq;               /* 0x6C */
    u32 divisor;                /* 0x70 */
    u32 global_alpha;           /* 0x74 */
    u32 size_dig;               /* 0x78 */
    u32 size_lcd;               /* 0x7C */
	u32 gfx_ba0;				/* 0x80 */
	u32 gfx_ba1;				/* 0x84 */
	u32 gfx_position;			/* 0x88 */
	u32 gfx_size;				/* 0x8C */
	uint8_t unused[16];				/* 0x90 */
	u32 gfx_attributes;			/* 0xA0 */
	u32 gfx_fifo_threshold;     /* 0xA4 */
	u32 gfx_fifo_size_status;   /* 0xA8 */
	u32 gfx_row_inc;			/* 0xAC */
	u32 gfx_pixel_inc;			/* 0xB0 */
	u32 gfx_window_skip;        /* 0xB4 */
	u32 gfx_table_ba;			/* 0xB8 */
};

/* Few Register Offsets */
#define FRAME_MODE_SHIFT            1
#define TFTSTN_SHIFT                3
#define DATALINES_SHIFT             8

/* Enabling Display controller */
#define LCD_ENABLE              1
#define DIG_ENABLE              (1 << 1)
#define GO_LCD                  (1 << 5)
#define GO_DIG                  (1 << 6)
#define GP_OUT0                 (1 << 15)
#define GP_OUT1                 (1 << 16)

#define DISPC_ENABLE                (LCD_ENABLE | \
                         DIG_ENABLE | \
                         GO_LCD | \
                         GO_DIG | \
                         GP_OUT0| \
                         GP_OUT1)

/* Configure VENC DSS Params */
#define VENC_CLK_ENABLE             (1 << 3)
#define DAC_DEMEN               (1 << 4)
#define DAC_POWERDN             (1 << 5)
#define VENC_OUT_SEL                (1 << 6)
#define DIG_LPP_SHIFT               16
#define VENC_DSS_CONFIG             (VENC_CLK_ENABLE | \
                         DAC_DEMEN | \
                         DAC_POWERDN | \
                         VENC_OUT_SEL)
/*
 * Panel Configuration
 */
struct panel_config {
    u32 timing_h;
    u32 timing_v;
    u32 pol_freq;
    u32 divisor;
    u32 lcd_size;
    u32 panel_type;
    u32 data_lines;
    u32 load_mode;
    u32 panel_color;
};

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
#define GPT1_IRQ        37
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

/* Only for GPTIMER1/2 */
#define TPIR            0x48
#define TNIR            0x4C
#define TCVR            0x50
#define TOCR            0x54 
#define TOWR            0x58




#endif
