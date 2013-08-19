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
#ifndef _PEXPERT_ARM_SUN4I_H_
#define _PEXPERT_ARM_SUN4I_H_

#define readl(a)		(*(volatile unsigned int *)(a))
#define writel(v, a)	(*(volatile unsigned int *)(a) = (v))
#define readw(a)		(*(volatile unsigned short *)(a))
#define writew(v, a)	(*(volatile unsigned short *)(a) = (v))


#define barrier()               __asm__ __volatile__("": : :"memory");

#define UART 0
#define TX_READY (readl(UART_LSR(UART)) & UART_LSR_TEMT)
#define UART_BASE 0x01C28000
/* receive buffer register */
#define UART_RBR(n) (uart_base + (n)*0x400 + 0x0)
/* transmit holding register */
#define UART_THR(n) (uart_base + (n)*0x400 + 0x0)
/* divisor latch low register */
#define UART_DLL(n) (uart_base + (n)*0x400 + 0x0)
/* divisor latch high register */
#define UART_DLH(n) (uart_base + (n)*0x400 + 0x4)
/* interrupt enable reigster */
#define UART_IER(n) (uart_base + (n)*0x400 + 0x4)
/* interrupt identity register */
#define UART_IIR(n) (uart_base + (n)*0x400 + 0x8)
/* fifo control register */
#define UART_FCR(n) (uart_base + (n)*0x400 + 0x8)
/* line control register */
#define UART_LCR(n) (uart_base + (n)*0x400 + 0xc)
#define UART_LCR_DLAB (0x1 << 7)
/* line status register */
#define UART_LSR(n) (uart_base + (n)*0x400 + 0x14)
#define UART_LSR_TEMT (0x1 << 6)
/* receive buffer register */
#define UART_RBR(n) (uart_base + (n)*0x400 + 0x0)
/* transmit holding register */
#define UART_THR(n) (uart_base + (n)*0x400 + 0x0)
/* divisor latch low register */
#define UART_DLL(n) (uart_base + (n)*0x400 + 0x0)
#define BAUD_115200    (0xd) /* 24 * 1000 * 1000 / 16 / 115200 = 13 */
#define NO_PARITY      (0)
#define ONE_STOP_BIT   (0)
#define DAT_LEN_8_BITS (3)
#define LC_8_N_1          (NO_PARITY << 3 | ONE_STOP_BIT << 2 | DAT_LEN_8_BITS)

#endif
