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
 * ARM RealView PB-A8 platform information
 */

#ifndef _PEXPERT_ARM_REALVIEW_H_
#define _PEXPERT_ARM_REALVIEW_H_

#define AMBA_UART_FR(base)      (*(volatile unsigned char *)((base) + 0x18))
#define AMBA_UART_DR(base)      (*(volatile unsigned char *)((base) + 0x00))

#define REALVIEW_UART0_BASE     0x10009000
#define REALVIEW_PIC0_BASE      0x1E000000
#define REALVIEW_TIMER0_BASE    0x10011000
#define REALVIEW_SYSCTL_BASE    0x10000000
#define REALVIEW_PL111_BASE     0x10020000

#define REALVIEW_EB_PIC0_BASE   0x10050000

#define HARDWARE_REGISTER(x)    *((unsigned int*)(x)) 

#define LCDTIMING0_PPL(x)           ((((x) / 16 - 1) & 0x3f) << 2)
#define LCDTIMING1_LPP(x)           (((x) & 0x3ff) - 1)
#define LCDCONTROL_LCDPWR           (1 << 11)
#define LCDCONTROL_LCDEN            (1)
#define LCDCONTROL_LCDBPP(x)        (((x) & 7) << 1)
#define LCDCONTROL_LCDTFT           (1 << 5)

#define barrier()               __asm__ __volatile__("": : :"memory");

#define PL111_TIMINGS_0         0x0
#define PL111_TIMINGS_1         0x4
#define PL111_TIMINGS_2         0x8
#define PL111_TIMINGS_3         0xC

#define PL111_UPPER_FB          0x10
#define PL111_LOWER_FB          0x14
#define PL111_CONTROL           0x18

#define PIC_ENABLE              0x1
#define PIC_ALLOW_INTR          0xF0

#define PICPRIOMASK             0x4

#define PIC_INTPRIO             0x10101010
#define PIC_CPUPRIO             0x01010101

#define TIMER_LOAD              0x0
#define TIMER_VALUE             0x4
#define TIMER_CONTROL           0x8
#define TIMER_INTCLR            0xC
#define TIMER_RIS               0x10
#define TIMER_MIS               0x14

#define TIMER_SET_ENABLE            (1 << 7)

#define TIMER_MODE_FREE_RUNNING     (1 << 5)
#define TIMER_SIZE_32_BIT           (1 << 1)
#define TIMER_ENABLE                0x1

int RealView_getc(void);
void RealView_uart_init(void);
void RealView_interrupt_init(void);
void RealView_timebase_init(void);
void RealView_handle_interrupt(void* context);
uint64_t RealView_get_timebase(void);
uint64_t RealView_timer_value(void);
void RealView_timer_enabled(int enable);
void RealView_framebuffer_init(void);

#endif
 
