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

#ifndef _PEXPERT_ARM_PROTOS_H
#define _PEXPERT_ARM_PROTOS_H

#include <stdint.h>

/*
 * Prototypes used internally by pexpert.
 */
extern uint32_t pe_arm_get_soc_base_phys(void);
extern uint32_t pe_arm_get_soc_revision(void);
extern uint32_t pe_arm_init_interrupts(void *args);
extern uint32_t pe_arm_init_timebase(void* args);
extern boolean_t pe_arm_dispatch_interrupt(void* context);
extern uint64_t pe_arm_get_timebase(void* args);
extern void pe_arm_set_timer_enabled(boolean_t enable);

int serial_init(void);
int serial_getc(void);
void serial_putc(char);
void uart_putc(char);
int uart_getc(void);

/*
 * Generic 32/26/8-bit memory I/O functions. 
 */
static inline void regwrite32(uint32_t addr, uint32_t data)
{
    *(volatile uint32_t *)addr = data;
}

static inline uint32_t regread32(uint32_t addr)
{
    uint32_t value = *(volatile uint32_t *)addr;

    return value;
}

static inline void regwrite16(uint16_t addr, uint16_t data)
{
    *(volatile uint16_t *)addr = data;
}

static inline uint16_t regread16(uint16_t addr)
{
    uint16_t value = *(volatile uint16_t *)addr;

    return value;
}

static inline void regwrite8(uint8_t addr, uint8_t data)
{
    *(volatile uint8_t *)addr = data;
}

static inline uint8_t regread8(uint8_t addr)
{
    uint8_t value = *(volatile uint8_t *)addr;

    return value;
}

/*
 * Memory barrier.
 */
#define barrier() \
    __asm__ __volatile__("": : :"memory");


/*
 * Console-related functions
 */
void cnputc(char);
int switch_to_serial_console(void);
void switch_to_old_console(int);


/*
 * SoC system service dispatch table prototypes and structures.
 */
void PE_init_SocSupport(void);

typedef void (*SocDevice_Uart_Putc)(char c);
typedef int (*SocDevice_Uart_Getc)(void);
typedef void (*SocDevice_Uart_Initialize)(void);
typedef void (*SocDevice_InitializeInterrupts)(void);
typedef void (*SocDevice_InitializeTimebase)(void);
typedef void (*SocDevice_HandleInterrupt)(void* context);
typedef uint64_t (*SocDevice_GetTimebase)(void);
typedef uint64_t (*SocDevice_GetTimer0_Value)(void);
typedef void (*SocDevice_SetTimer0_Enabled)(int enable);
typedef void (*SocDevice_PrepareFramebuffer)(void);

int PE_early_putc(int c);
void PE_early_puts(char* s);

typedef struct SocDeviceDispatch {
    SocDevice_Uart_Getc             uart_getc;
    SocDevice_Uart_Putc             uart_putc;
    SocDevice_Uart_Initialize       uart_init;
    SocDevice_InitializeInterrupts  interrupt_init;
    SocDevice_InitializeTimebase    timebase_init;
    SocDevice_HandleInterrupt       handle_interrupt;
    SocDevice_GetTimer0_Value       timer_value;
    SocDevice_SetTimer0_Enabled     timer_enabled;
    SocDevice_PrepareFramebuffer    framebuffer_init;
    SocDevice_GetTimebase           get_timebase;
} SocDeviceDispatch;

extern SocDeviceDispatch    gPESocDispatch;

#endif /* _PEXPERT_ARM_PROTOS_H */
