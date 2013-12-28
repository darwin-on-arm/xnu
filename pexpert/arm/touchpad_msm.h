/* 
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _TOUCHPAD_MSM_H_
#define _TOUCHPAD_MSM_H_

#define barrier()               __asm__ __volatile__("": : :"memory");

#define MSM_TMR_BASE             0x02000000
#define MSM_GPT_BASE            (0x04)
#define MSM_DGT_BASE            (0x24)

#define GPT_REG(off)            (MSM_GPT_BASE + (off))
#define DGT_REG(off)            (MSM_DGT_BASE + (off))

#define GPT_MATCH_VAL            GPT_REG(0x0000)
#define GPT_COUNT_VAL            GPT_REG(0x0004)
#define GPT_ENABLE               GPT_REG(0x0008)
#define GPT_CLEAR                GPT_REG(0x000C)

#define DGT_MATCH_VAL            DGT_REG(0x0000)
#define DGT_COUNT_VAL            DGT_REG(0x0004)
#define DGT_ENABLE               DGT_REG(0x0008)
#define DGT_CLEAR                DGT_REG(0x000C)
#define DGT_CLK_CTL              DGT_REG(0x0010)

#define MSM_GIC_DIST_BASE        0x02080000
#define MSM_GIC_CPU_BASE         0x02081000

#define GIC_CPU_REG(off)            ((off))
#define GIC_DIST_REG(off)           ((off))

#define GIC_CPU_CTRL                GIC_CPU_REG(0x00)
#define GIC_CPU_PRIMASK             GIC_CPU_REG(0x04)
#define GIC_CPU_BINPOINT            GIC_CPU_REG(0x08)
#define GIC_CPU_INTACK              GIC_CPU_REG(0x0c)
#define GIC_CPU_EOI                 GIC_CPU_REG(0x10)
#define GIC_CPU_RUNNINGPRI          GIC_CPU_REG(0x14)
#define GIC_CPU_HIGHPRI             GIC_CPU_REG(0x18)

#define GIC_DIST_CTRL               GIC_DIST_REG(0x000)
#define GIC_DIST_CTR                GIC_DIST_REG(0x004)
#define GIC_DIST_ENABLE_SET         GIC_DIST_REG(0x100)
#define GIC_DIST_ENABLE_CLEAR       GIC_DIST_REG(0x180)
#define GIC_DIST_PENDING_SET        GIC_DIST_REG(0x200)
#define GIC_DIST_PENDING_CLEAR      GIC_DIST_REG(0x280)
#define GIC_DIST_ACTIVE_BIT         GIC_DIST_REG(0x300)
#define GIC_DIST_PRI                GIC_DIST_REG(0x400)
#define GIC_DIST_TARGET             GIC_DIST_REG(0x800)
#define GIC_DIST_CONFIG             GIC_DIST_REG(0xc00)
#define GIC_DIST_SOFTINT            GIC_DIST_REG(0xf00)


/* MSM ACPU Interrupt Numbers */

/* 0-15:  STI/SGI (software triggered/generated interrupts)
 * 16-31: PPI (private peripheral interrupts)
 * 32+:   SPI (shared peripheral interrupts)
 */

#define GIC_PPI_START 16
#define GIC_SPI_START 32

#define INT_DEBUG_TIMER_EXP     (GIC_PPI_START + 1)

#define USB1_HS_BAM_IRQ         (GIC_SPI_START + 94)
#define USB1_HS_IRQ             (GIC_SPI_START + 100)
#define USB2_IRQ                (GIC_SPI_START + 141)
#define USB1_IRQ                (GIC_SPI_START + 142)

#define GSBI_QUP_IRQ(id)        ((id) <= 8 ? (GIC_SPI_START + 145 + 2*(id)) : \
                                             (GIC_SPI_START + 187 + 2*((id)-8)))

/* Retrofit universal macro names */
#define INT_USB_HS                  USB1_HS_IRQ

#define NR_MSM_IRQS                 256
#define NR_GPIO_IRQS                173
#define NR_BOARD_IRQS               0

#define NR_IRQS (NR_MSM_IRQS + NR_GPIO_IRQS + NR_BOARD_IRQS)

#define GPT_ENABLE_CLR_ON_MATCH_EN        2
#define GPT_ENABLE_EN                     1
#define DGT_ENABLE_CLR_ON_MATCH_EN        2
#define DGT_ENABLE_EN                     1

#define SPSS_TIMER_STATUS_DGT_EN    (1 << 0)

#endif