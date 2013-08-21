/*
 * Samsung S5L8930X support
 */

#ifndef _S5L8930X_H_
#define _S5L8930X_H_

#define UART0_BASE          0x82500000
#define GPIO_BASE           0xBFA00000
#define FRAMEBUFFER_BASE    0x5F700000
#define CLOCK_GATE_BASE     0xBF101000

#define CLK_REG_OFF         0x10

#define VIC_START           0xBF200000
#define VIC_REGISTER_SIZE   0x10000
#define VIC(x)              (VIC_START + (x * VIC_REGISTER_SIZE))

#define VICIRQSTATUS 		0x000
#define VICRAWINTR 			0x8
#define VICINTSELECT 		0xC
#define VICINTENABLE 		0x10
#define VICINTENCLEAR 		0x14
#define VICSWPRIORITYMASK	0x24
#define VICVECTADDRS 		0x100
#define VICADDRESS 			0xF00

#define CLOCK_HZ            24000000

#define IRQ_TIMER0          300

#define UART_CLOCKGATE      		0x30
#define UART_CLOCK_SELECTION_MASK 	(0x3 << 10)
#define UART_CLOCK_SELECTION_SHIFT 	10
#define UART_DIVVAL_MASK 			0x0000FFFF
#define UART_SAMPLERATE_MASK 		0x00030000
#define UART_SAMPLERATE_SHIFT 		16
#define UART_UCON_RXMODE_SHIFT 		0
#define UART_UCON_TXMODE_SHIFT 		2
#define UART_8BITS 					3
#define UART_FIFO_RESET_TX 			4
#define UART_FIFO_RESET_RX 			2
#define UART_FIFO_ENABLE 			1

#define UART_UCON_MODE_IRQORPOLL 	1

#define ULCON      0x0000 /* Line Control             */
#define UCON       0x0004 /* Control                  */
#define UFCON      0x0008 /* FIFO Control             */
#define UMCON      0x000C /* Modem Control            */
#define UTRSTAT    0x0010 /* Tx/Rx Status             */
#define UERSTAT    0x0014 /* UART Error Status        */
#define UFSTAT     0x0018 /* FIFO Status              */
#define UMSTAT     0x001C /* Modem Status             */
#define UTXH       0x0020 /* Transmit Buffer          */
#define URXH       0x0024 /* Receive Buffer           */
#define UBRDIV     0x0028 /* Baud Rate Divisor        */
#define UFRACVAL   0x002C /* Divisor Fractional Value */
#define UINTP      0x0030 /* Interrupt Pending        */
#define UINTSP     0x0034 /* Interrupt Source Pending */
#define UINTM      0x0038 /* Interrupt Mask           */

#define UART_UFSTAT_TXFIFO_FULL			(0x1 << 9)
#define UART_UTRSTAT_TRANSMITTEREMPTY 	0x4
#define UART_UMSTAT_CTS 				0x1

#define barrier()               __asm__ __volatile__("": : :"memory");

#endif