/*
 * Copyright 2015, Vince Cali. <0x56.0x69.0x6e.0x63.0x65@gmail.com>
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
 * i.MX53 support
 *
 * Values derived from i.MX53 Multimedia Applications Processor Reference Manual (iMX53RM.pdf)
 */

#ifndef _PEXPERT_IMX53_H_
#define _PEXPERT_IMX53_H_

/*
 * iMX53RM.pdf, p. 214
 */

/* ARM platform interrupts */
#define IRQ_ESDHCV2_1         1
#define IRQ_ESDHCV2_2         2
#define IRQ_ESDHCV2_3         3
#define IRQ_ESDHCV2_4         4
#define IRQ_DAP               5
#define IRQ_SDMA              6
#define IRQ_IOMUXC            7
#define IRQ_NFC               8
#define IRQ_VPU               9
#define IRQ_IPU_ERROR         10
#define IRQ_IPU_SYNC          11
#define IRQ_GPU               12
#define IRQ_UART_4            13
#define IRQ_USB_HOST_1        14
#define IRQ_EXTMC             15
#define IRQ_USB_HOST_2        16
#define IRQ_USB_HOST_3        17
#define IRQ_USB_OTG           18
#define IRQ_SAHARA_HOST_0     19
#define IRQ_SAHARA_HOST_1     20
#define IRQ_SCC_HIGH          21
#define IRQ_SCC_TZ            22
#define IRQ_SCC_NS            23
#define IRQ_SRTC_NS           24
#define IRQ_SRTC_TZ           25
#define IRQ_RTIC              26
#define IRQ_CSU               27
#define IRQ_SATA              28
#define IRQ_SSI_1             29
#define IRQ_SSI_2             30
#define IRQ_UART_1            31
#define IRQ_UART_2            32
#define IRQ_UART_3            33
#define IRQ_IPTP_RTC          34
#define IRQ_IPTP_PTP          35
#define IRQ_ECSPI_1           36
#define IRQ_ECSPI_2           37
#define IRQ_CSPI              38
#define IRQ_GPT               39
#define IRQ_EPIT_1            40
#define IRQ_EPIT_2            41
#define IRQ_GPIO_1_INT7       42
#define IRQ_GPIO_1_INT6       43
#define IRQ_GPIO_1_INT5       44
#define IRQ_GPIO_1_INT4       45
#define IRQ_GPIO_1_INT3       46
#define IRQ_GPIO_1_INT2       47
#define IRQ_GPIO_1_INT1       48
#define IRQ_GPIO_1_INT0       49
#define IRQ_GPIO_1_0_15       50
#define IRQ_GPIO_1_16_31      51
#define IRQ_GPIO_2_0_15       52
#define IRQ_GPIO_2_16_31      53
#define IRQ_GPIO_3_0_15       54
#define IRQ_GPIO_3_16_31      55
#define IRQ_GPIO_4_0_15       56
#define IRQ_GPIO_4_16_31      57
#define IRQ_WDOG_1            58
#define IRQ_WDOG_2            59
#define IRQ_KPP               60
#define IRQ_PWM_1             61
#define IRQ_I2C_1             62
#define IRQ_I2C_2             63
#define IRQ_I2C_3             64
#define IRQ_MLB               65
#define IRQ_ASRC              66
#define IRQ_SPDIF             67
#define IRQ_IIM               69
#define IRQ_PATA              70
#define IRQ_CCM1              71
#define IRQ_CCM2              72
#define IRQ_GPC1              73
#define IRQ_GPC2              74
#define IRQ_SRC               75
#define IRQ_NEON              76
#define IRQ_NPMUIRQ           77
#define IRQ_CTI               78
#define IRQ_DBG_CT1_1         79
#define IRQ_DBG_CT1_0         80
#define IRQ_ESAI              81
#define IRQ_FLEXCAN_1         82
#define IRQ_FLEXCAN_2         83
#define IRQ_OPENVG_GEN        84
#define IRQ_OPENVG_BUSY       85
#define IRQ_UART_5            86
#define IRQ_FEC               87
#define IRQ_OWIRE             88
#define IRQ_DBG_CT1_2         89
#define IRQ_SJC               90
#define IRQ_TVE               92
#define IRQ_FIRI              93
#define IRQ_PWM_2             94
#define IRQ_SSI_3             96
#define IRQ_DBG_CT1_3         98
#define IRQ_VPU_IDLE          100
#define IRQ_NFC_ALL_TRANS     101
#define IRQ_GPU_IDLE          102
#define IRQ_GPIO_5_0_15       103
#define IRQ_GPIO_5_16_31      104
#define IRQ_GPIO_6_0_15       105
#define IRQ_GPIO_6_16_31      106
#define IRQ_GPIO_7_0_15       107
#define IRQ_GPIO_7_16_31      108


/*
 * iMX53RM.pdf, p. 4604
 */

#define TZIC_BASE             0xFFFC000

/* TZIC registers */
#define TZIC_INTCTRL          0
#define TZIC_INTTYPE          4
#define TZIC_PRIOMASK         0xC
#define TZIC_SYNCCTRL         0x10
#define TZIC_DSMINT           0x14
#define TZIC_INTSEC0          0x80
#define TZIC_INTSEC1          0x84
#define TZIC_INTSEC2          0x88
#define TZIC_INTSEC3          0x8C
#define TZIC_ENSET0           0x100
#define TZIC_ENSET1           0x104
#define TZIC_ENSET2           0x108
#define TZIC_ENSET3           0x10C
#define TZIC_ENCLEAR0         0x180
#define TZIC_ENCLEAR1         0x184
#define TZIC_ENCLEAR2         0x188
#define TZIC_ENCLEAR3         0x18C
#define TZIC_SRCSET0          0x200
#define TZIC_SRCSET1          0x204
#define TZIC_SRCSET2          0x208
#define TZIC_SRCSET3          0x20C
#define TZIC_SRCCLEAR0        0x280
#define TZIC_SRCCLEAR1        0x284
#define TZIC_SRCCLEAR2        0x288
#define TZIC_SRCCLEAR3        0x28C
#define TZIC_PRIORITY0        0x400
#define TZIC_PRIORITY1        0x404
#define TZIC_PRIORITY2        0x408
#define TZIC_PRIORITY3        0x40C
#define TZIC_PRIORITY4        0x410
#define TZIC_PRIORITY5        0x414
#define TZIC_PRIORITY6        0x418
#define TZIC_PRIORITY7        0x41C
#define TZIC_PRIORITY8        0x420
#define TZIC_PRIORITY9        0x424
#define TZIC_PRIORITY10       0x428
#define TZIC_PRIORITY11       0x42C
#define TZIC_PRIORITY12       0x430
#define TZIC_PRIORITY13       0x434
#define TZIC_PRIORITY14       0x438
#define TZIC_PRIORITY15       0x43C
#define TZIC_PRIORITY16       0x440
#define TZIC_PRIORITY17       0x444
#define TZIC_PRIORITY18       0x448
#define TZIC_PRIORITY19       0x44C
#define TZIC_PRIORITY20       0x450
#define TZIC_PRIORITY21       0x454
#define TZIC_PRIORITY22       0x458
#define TZIC_PRIORITY23       0x45C
#define TZIC_PRIORITY24       0x460
#define TZIC_PRIORITY25       0x464
#define TZIC_PRIORITY26       0x468
#define TZIC_PRIORITY27       0x46C
#define TZIC_PRIORITY28       0x470
#define TZIC_PRIORITY29       0x474
#define TZIC_PRIORITY30       0x478
#define TZIC_PRIORITY31       0x47C
#define TZIC_PND0             0xD00
#define TZIC_PND1             0xD04
#define TZIC_PND2             0xD08
#define TZIC_PND3             0xD0C
#define TZIC_HIPND0           0xD80
#define TZIC_HIPND1           0xD84
#define TZIC_HIPND2           0xD88
#define TZIC_HIPND3           0xD8C
#define TZIC_WAKEUP0          0xE00
#define TZIC_WAKEUP1          0xE04
#define TZIC_WAKEUP2          0xE08
#define TZIC_WAKEUP3          0xE0C
#define TZIC_SWINT            0xF00

/* TZIC register fields */
#define TZIC_INTCTRL_NSENMASK         (1 << 31)
#define TZIC_INTCTRL_NSEN             (1 << 16)
#define TZIC_INTCTRL_EN               (1 << 0)

#define TZIC_INTTYPE_DOM              (1 << 10)
#define TZIC_INTTYPE_CPUS             (7 << 5)
#define TZIC_INTTYPE_ITLINES          0x1F

#define TZIC_PRIOMASK_MASK            0xFF

#define TZIC_SYNCCTRL_SYNCMODE        3

#define TZIC_DSMINT_DSM               (1 << 0)


/*
 * iMX53RM.pdf, p. 1756
 */

#define GPT_BASE              0x53FA0000

/* GPT registers */
#define GPT_CR                0
#define GPT_PR                4
#define GPT_SR                8
#define GPT_IR                0xC
#define GPT_OCR1              0x10
#define GPT_OCR2              0x14
#define GPT_OCR3              0x18
#define GPT_ICR1              0x1C
#define GPT_ICR2              0x20
#define GPT_CNT               0x24

/* GPT register fields */
#define GPT_CR_FO3               (1 << 31)
#define GPT_CR_FO2               (1 << 30)
#define GPT_CR_FO1               (1 << 29)
#define GPT_CR_OM3               (7 << 26)
#define GPT_CR_OM2               (7 << 23)
#define GPT_CR_OM1               (7 << 20)
#define GPT_CR_IM2               (3 << 18)
#define GPT_CR_IM1               (3 << 16)
#define GPT_CR_SWR               (1 << 15)
#define GPT_CR_FRR               (1 << 9)
#define GPT_CR_CLKSRC            (7 << 6)
#define GPT_CR_STOPEN            (1 << 5)
#define GPT_CR_DOZEEN            (1 << 4)
#define GPT_CR_WAITEN            (1 << 3)
#define GPT_CR_DBGEN             (1 << 2)
#define GPT_CR_ENMOD             (1 << 1)
#define GPT_CR_EN                (1 << 0)

#define GPT_PR_PRESCALER         0xFFF

#define GPT_SR_ROV               (1 << 5)
#define GPT_SR_IF2               (1 << 4)
#define GPT_SR_IF1               (1 << 3)
#define GPT_SR_OF3               (1 << 2)
#define GPT_SR_OF2               (1 << 1)
#define GPT_SR_OF1               (1 << 0)

#define GPT_IR_ROVIE             (1 << 5)
#define GPT_IR_IF2IE             (1 << 4)
#define GPT_IR_IF1IE             (1 << 3)
#define GPT_IR_OF3IE             (1 << 2)
#define GPT_IR_OF2IE             (1 << 1)
#define GPT_IR_OF1IE             (1 << 0)


/*
 * iMX53RM.pdf, p.1140
 */

#define EPIT1_BASE            0x53FAC000
#define EPIT2_BASE            0x53FB0000

/* EPIT registers */
#define EPIT_EPITCR           0
#define EPIT_EPITSR           4
#define EPIT_EPITLR           8
#define EPIT_EPITCMPR         0xC
#define EPIT_EPITCNR          0x10

/* EPIT status bits */
#define EPIT_EPITSR_OCIF      (1 << 0)

/* EPIT control bits */
#define EPIT_EPITCR_CLKSRC    (3 << 24)
#define EPIT_EPITCR_OM        (3 << 22)
#define EPIT_EPITCR_STOPEN    (1 << 21)
#define EPIT_EPITCR_WAITEN    (1 << 19)
#define EPIT_EPITCR_DBGEN     (1 << 18)
#define EPIT_EPITCR_IOVW      (1 << 17)
#define EPIT_EPITCR_SWR       (1 << 16)
#define EPIT_EPITCR_PRESCALER (0xFFF << 4)
#define EPIT_EPITCR_RLD       (1 << 3)
#define EPIT_EPITCR_OCIEN     (1 << 2)
#define EPIT_EPITCR_ENMOD     (1 << 1)
#define EPIT_EPITCR_EN        (1 << 0)


/*
 * iMX53RM.pdf, p. 4668
 */

#define UART1_BASE            0x53FBC000

/* UART registers */
#define UART_URXD             0
#define UART_UTXD             0x40
#define UART_UCR1             0x80
#define UART_UCR2             0x84
#define UART_UCR3             0x88
#define UART_UCR4             0x8C
#define UART_UFCR             0x90
#define UART_USR1             0x94
#define UART_USR2             0x98
#define UART_UESC             0x9C
#define UART_UTIM             0xA0
#define UART_UBIR             0xA4
#define UART_UBMR             0xA8
#define UART_UBRC             0xAC
#define UART_ONEMS            0xB0
#define UART_UTS              0xB4

/* UART register fields */
#define UART_URXD_CHARRDY     (1 << 15)
#define UART_URXD_ERR         (1 << 14)
#define UART_URXD_OVRRUN      (1 << 13)
#define UART_URXD_FRMERR      (1 << 12)
#define UART_URXD_BRK         (1 << 11)
#define UART_URXD_PRERR       (1 << 10)

#define UART_UCR1_ADEN        (1 << 15)
#define UART_UCR1_ADBR        (1 << 14)
#define UART_UCR1_TRDYEN      (1 << 13)
#define UART_UCR1_IDEN        (1 << 12)
#define UART_UCR1_ICD         (3 << 10)
#define UART_UCR1_RRDYEN      (1 << 9)
#define UART_UCR1_RXDMAEN     (1 << 8)
#define UART_UCR1_IREN        (1 << 7)
#define UART_UCR1_TXMPTYEN    (1 << 6)
#define UART_UCR1_RTSDEN      (1 << 5)
#define UART_UCR1_SNDBRK      (1 << 4)
#define UART_UCR1_TXDMAEN     (1 << 3)
#define UART_UCR1_ATDMAEN     (1 << 2)
#define UART_UCR1_DOZE        (1 << 1)
#define UART_UCR1_UARTEN      (1 << 0)

#define UART_USR1_PARITYERR   (1 << 15)
#define UART_USR1_RTSS        (1 << 14)
#define UART_USR1_TRDY        (1 << 13)
#define UART_USR1_RTSD        (1 << 12)
#define UART_USR1_ESCF        (1 << 11)
#define UART_USR1_FRAMERR     (1 << 10)
#define UART_USR1_RRDY        (1 << 9)
#define UART_USR1_AGTIM       (1 << 8)
#define UART_USR1_DTRD        (1 << 7)
#define UART_USR1_RXDS        (1 << 6)
#define UART_USR1_AIRINT      (1 << 5)
#define UART_USR1_AWAKE       (1 << 4)

#define UART_USR2_ADET        (1 << 15)
#define UART_USR2_TXFE        (1 << 14)
#define UART_USR2_DTRF        (1 << 13)
#define UART_USR2_IDLE        (1 << 12)
#define UART_USR2_ACST        (1 << 11)
#define UART_USR2_RIDELT      (1 << 10)
#define UART_USR2_RIIN        (1 << 9)
#define UART_USR2_IRINT       (1 << 8)
#define UART_USR2_WAKE        (1 << 7)
#define UART_USR2_DCDDELT     (1 << 6)
#define UART_USR2_DCDIN       (1 << 5)
#define UART_USR2_RTSF        (1 << 4)
#define UART_USR2_TXDC        (1 << 3)
#define UART_USR2_BRCD        (1 << 2)
#define UART_USR2_ORE         (1 << 1)
#define UART_USR2_RDR         (1 << 0)

#define UART_UTS_FRCPERR      (1 << 13)
#define UART_UTS_LOOP         (1 << 12)
#define UART_UTS_DBGEN        (1 << 11)
#define UART_UTS_LOOPIR       (1 << 10)
#define UART_UTS_RXDBG        (1 << 9)
#define UART_UTS_TXEMPTY      (1 << 6)
#define UART_UTS_RXEMPTY      (1 << 5)
#define UART_UTS_TXFULL       (1 << 4)
#define UART_UTS_RXFULL       (1 << 3)
#define UART_UTS_SOFTRST      (1 << 0)


#endif /* !_PEXPERT_IMX53_H_ */

