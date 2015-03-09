/*
 * Raspberry Pi support
 */

#ifndef _PEXPERT_BCM2835_H_
#define _PEXPERT_BCM2835_H_

#include <mach/mach_types.h>

#define PUT32(addr, val)       *((volatile uint32_t*) (addr)) = (val);
#define GET32(addr)            (*((volatile uint32_t*) (addr)))
#define GET64(addr)            (*((volatile uint64_t*) (addr)))

/* UART */
#define GP_BASE   0x20200000
#define GPFSEL1   0x04
#define GPSET0    0x1C
#define GPCLR0    0x28
#define GPPUD     0x94
#define GPPUDCLK0 0x98

#define AUX_BASE        0x20215000
#define AUX_ENABLES     0x04
#define AUX_MU_IO_REG   0x40
#define AUX_MU_IER_REG  0x44
#define AUX_MU_IIR_REG  0x48
#define AUX_MU_LCR_REG  0x4C
#define AUX_MU_MCR_REG  0x50
#define AUX_MU_LSR_REG  0x54
#define AUX_MU_MSR_REG  0x58
#define AUX_MU_SCRATCH  0x5C
#define AUX_MU_CNTL_REG 0x60
#define AUX_MU_STAT_REG 0x64
#define AUX_MU_BAUD_REG 0x68

/* Timer */
#define SYSTIMER_BASE  0x20003000
#define SYSTIMERCLK    0x04

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

#endif /* !_PEXPERT_BCM2835_H_ */
