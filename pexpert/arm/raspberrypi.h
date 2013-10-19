/*
 * Raspberry Pi support
 */

#ifndef xnu_raspberrypi_h
#define xnu_raspberrypi_h

#include <mach/mach_types.h>

#define PUT32_phy(addr, val)       *((volatile uint32_t*) ml_io_map(addr, PAGE_SIZE)) = (val);
#define GET32_phy(addr)            (*((volatile uint32_t*) ml_io_map(addr, PAGE_SIZE)))
#define PUT32(addr, val)       *((volatile uint32_t*) (addr)) = (val);
#define GET32(addr)            (*((volatile uint32_t*) (addr)))
#define GET64(addr)            (*((volatile uint64_t*) (addr)))
#define barrier()               __asm__ __volatile__("": : :"memory");

/* UART */
#define GPFSEL1 0x20200004
#define GPSET0  0x2020001C
#define GPCLR0  0x20200028
#define GPPUD       0x20200094
#define GPPUDCLK0   0x20200098

#define AUX_ENABLES     0x20215004
#define AUX_MU_IO_REG   0x20215040
#define AUX_MU_IER_REG  0x20215044
#define AUX_MU_IIR_REG  0x20215048
#define AUX_MU_LCR_REG  0x2021504C
#define AUX_MU_MCR_REG  0x20215050
#define AUX_MU_LSR_REG  0x20215054
#define AUX_MU_MSR_REG  0x20215058
#define AUX_MU_SCRATCH  0x2021505C
#define AUX_MU_CNTL_REG 0x20215060
#define AUX_MU_STAT_REG 0x20215064
#define AUX_MU_BAUD_REG 0x20215068

/* Timer */
#define SYSTIMERCLK 0x20003004

/* Framebuffer */
#define MAILBOX_BASE 0x2000B880

#define MB_READ   0x00
#define MB_POLL   0x10
#define MB_SENDER 0x14
#define MB_STATUS 0x18
#define MB_CFG    0x1C
#define MB_WRITE  0x20

struct fb_info {
    uint32_t phys_w;
    uint32_t phys_h;
    uint32_t virt_w;
    uint32_t virt_h;
    uint32_t gpu_pitch;
    uint32_t bpp;
    uint32_t x;
    uint32_t y;
    void *gpu_ptr;
    uint32_t gpu_size;
};

#endif
