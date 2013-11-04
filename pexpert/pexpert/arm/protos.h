#ifndef _PEXPERT_ARM_PROTOS_H
#define _PEXPERT_ARM_PROTOS_H

extern uint32_t pe_arm_get_soc_base_phys(void);
extern uint32_t pe_arm_get_soc_revision(void);
extern uint32_t pe_arm_init_interrupts(void *args);
extern uint32_t pe_arm_init_timebase(void* args);
extern boolean_t pe_arm_dispatch_interrupt(void* context);
extern uint64_t pe_arm_get_timebase(void* args);

int serial_init(void);
int serial_getc(void);
void serial_putc(char);
void uart_putc(char);
int uart_getc(void);

//void vc_progress_initialize(void *, const unsigned char *, const unsigned char *);
//void vc_display_icon(void *, const unsigned char *);

int switch_to_serial_console(void);
void switch_to_old_console(int);

void cnputc(char);

void PE_init_SocSupport(void);

#ifdef BOARD_CONFIG_ARMPBA8
void PE_init_SocSupport_realview(void);
#endif

#ifdef BOARD_CONFIG_SUN4I
void PE_init_SocSupport_sun4i(void);
#endif

#if defined(BOARD_CONFIG_OMAP3530) || defined(BOARD_CONFIG_OMAP335X)
void PE_init_SocSupport_omap3(void);
#endif

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
