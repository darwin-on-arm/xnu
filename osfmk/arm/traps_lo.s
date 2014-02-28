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
 * ARM processor abort handlers.
 */

#include <arm/arch.h>
#include <arm/asm_help.h>
#include <assym.s>
#include <mach/arm/asm.h>

/**
 * fleh_reset
 *
 * Just halt the processor right here with a forever loop.
 */
EnterARM(fleh_reset)
    b       .

/**
 * fleh_undef
 *
 * Call the undefined handler.
 */
EnterARM(fleh_undef)
    /* Get current mode */
    mrs     sp, spsr
    tst     sp, #0x20

    /* Subtract lr based on cpsr bit */
    subeq   lr, lr, #4
    subne   lr, lr, #2

    /* See if it's in the kernel. */
    mrs     sp, spsr
    tst     sp, #0xf
    bne     undef_from_kernel
undef_from_user:

    /* Usermode undefined, maybe it's a VFP instruction, HUH?! */
    LoadThreadRegister(sp)
    ldr     sp, [sp, TH_PCB_USS]
    stmea   sp, {r0-lr}^

    str     lr, [sp, #0x3C]
    mrs     r2, spsr
    str     r2, [sp, #0x40]
    mov     r0, sp

    /* Change to supervisor */
    cpsid   i, #0x13
    LoadThreadRegister(r1)
    ldr     sp, [r1, TH_PCB_ISS]
    blx     _sleh_undef
    b       _thread_exception_return

undef_from_kernel:
    /* Oops. */
    cpsid   i, #0x13

    sub     sp, sp, #0x50
    stmea   sp, {r0-r12}

    str     lr, [sp, #0x38]
    mov     r0, sp

    /* Undefined */
    cpsid   i, #0x1b
    mrs     r2, spsr
    str     lr, [r0, #0x3C]
    str     r2, [r0, #0x40]

    /* Supervisor */
    cpsid   i, #0x13

    add     r3, sp, #0x50
    str     r3, [r0, #0x34]

    blx     _sleh_undef

    /* NEVER COME BACK */
    b       .

/**
 * fleh_swi
 *
 * Call the software interrupt handler.
 */
EnterARM(fleh_swi)
    cmn     r12, #3
    beq     swi_trap_tb

    /* Save current registers and spsr */
    LoadThreadRegister(sp)
    ldr     sp, [sp, TH_PCB_USS]
    stmea   sp, {r0-lr}^

    str     lr, [sp, #0x3C]
    mrs     r2, spsr
    str     r2, [sp, #0x40]

    mov     r8, sp

    LoadThreadRegister(sp)
    ldr     sp, [sp, TH_PCB_ISS]

    mov     r11, r12
    cpsie   i
    /* Is it a trap? */
    cmp     r11, #0x80000000
    beq     swi_trap

swi_trap_ret:
    rsbs    r5, r11, #0
    ble     swi_unix

swi_mach:
    /* Load the mach function from the mach trap table and call it. */
    adr     lr, swi_exit
    mov     r4, r5
    cmp     r5, #0x80
    bge     swi_mach_error
    LOAD_ADDR(r1, mach_trap_table)

    add     r11, r5, r5, lsl#1
    add     r1, r1, r11, lsl#2

    ldr     r1, [r1, #4]
    LOAD_ADDR(r2, kern_invalid)

    mov     r0, r8
    teq     r1, r2
    beq     swi_mach_error
    bx      r1
swi_exit64:
    str     r1, [r8, #4]
swi_exit:
    /* Exit and go back. */
    str     r0, [r8]
    mov     r0, r8
    blx     _mach_syscall_trace
    bl      _thread_exception_return

    mov     r0, #0x80
    b       irqvec_panic

swi_mach_error:
    /* There was an error processing the Mach system call, panic. */
    mov     r0, #7
    mov     r1, r4
    mov     r2, #1
    blx     _doexception

    mov     r0, #0x81
    b       irqvec_panic

swi_unix:
    /* Unix syscall, call unix_syscall to dispatch */
    mov     r0, r8
    blx     _unix_syscall
    mov     r0, #0x82
    b       irqvec_panic

swi_trap:
    /* Switch outcome based on our input value */
    cmp     r3, #3
    addls   pc, pc, r3, lsl#2
swi_trap_table:
    b       swi_trap_ret
    b       xxx_trap    /* icache clean */
    b       xxx_trap    /* dcache clean */
    b       thread_set_cthread_trap
    b       thread_get_cthread_trap

swi_trap_tb:
    /* Fast return */
    movs    pc, lr

xxx_trap:
    /* Just return. */
    bl      _thread_exception_return

thread_set_cthread_trap:
    /* Set the current cthread value. */
    blx     _thread_set_cthread_self
    bl      _thread_exception_return
    mov     r0, #0x82
    b       irqvec_panic

thread_get_cthread_trap:
    /* Get the current cthread value and return to user. */
    blx     _thread_get_cthread_self
    LoadThreadRegister(r1)
    ldr     r1, [r1, TH_PCB_USS]
    str     r0, [r1]
    bl      _thread_exception_return
    mov     r0, #0x83
    b       irqvec_panic

/**
 * fleh_prefabt
 *
 * Save registers and call the prefetch handler
 */
EnterARM(fleh_prefabt)
    sub     lr, lr, #4

    mrs     sp, cpsr
    bic     sp, sp, #0x100
    msr     cpsr_c, sp

    mrs     sp, spsr
    tst     sp, #0xf
    bne     prefetch_abort_in_kernel

prefetch_abort_in_user:
    /* Oh well, not now. */
    LoadThreadRegister(sp)
    ldr     sp, [sp, TH_PCB_USS]
    stmea   sp, {r0-lr}^
    str     lr, [sp, #0x3C]
    str     lr, [sp, #0x48]
    mrs     r0, spsr
    str     r0, [sp, #0x40]
    mrc     p15, 0, r0, c5, c0, 0
    str     r0, [sp, #0x44]    
    mov     r0, sp
    cpsid   i, #0x13
    LoadThreadRegister(r1)
    ldr     sp, [r1, TH_PCB_ISS]
    mov     r1, #3
    blx     _sleh_abort
    b       _thread_exception_return

prefetch_abort_in_kernel:
    /* supervisor state */
    msr     cpsr_c, #0x93
    
    /* make space on the stack for the registers. */
    sub     sp, sp, #0x50
    stmea   sp, {r0-r12}

    /* Save the remaining registers. */
    str     lr, [sp, #0x38]
    mov     r12, sp

    /* abort mode */
    msr     cpsr_c, #0x97

    /* Save more. */
    str     lr, [r12, #0x3C]
    str     lr, [r12, #0x48]

    /* Save DFSR. */
    mrc     p15, 0, r5, c5, c0, 0
    str     r5, [r12, #0x44]

    /* Save SPSR. */
    mrs     r4, spsr
    str     r4, [r12, #0x40]

    /* supervisor */
    msr     cpsr_c, #0x93

    add     r12, r12, #0x50
    str     r12, [sp, #0x34]
    sub     r12, r12, #0x50
    mov     r0, sp
    mov     r1, #3      // Prefetch Abort
    bl      _sleh_abort
    b       restore_kernel_context

/**
 * fleh_dataabt
 *
 * Handle data aborts.
 */
EnterARM(fleh_dataabt)
    /* Make sure the data abort was in the kernel. */
    sub     lr, lr, #8
    mrs     sp, spsr
    tst     sp, #0xf
    bne     data_abort_crash_in_kernel

data_abort_crash_in_usermode:
    /* Oh well, not now. */
    LoadThreadRegister(sp)
    ldr     sp, [sp, TH_PCB_USS]
    stmea   sp, {r0-lr}^
    str     lr, [sp, #0x3C]
    mrs     r0, spsr
    str     r0, [sp, #0x40]
    mov     r0, sp
    cpsid   i, #0x13
    mrc     p15, 0, r5, c5, c0, 0
    mrc     p15, 0, r6, c6, c0, 0
    str     r5, [sp, #0x44]
    str     r6, [sp, #0x48]
    LoadThreadRegister(r1)
    ldr     sp, [r1, TH_PCB_ISS]
    mov     r1, #4
    blx     _sleh_abort
    b       _thread_exception_return

data_abort_crash_in_kernel:
    /* supervisor */
    cpsid   i, #0x13
    
    /* make space on the stack for the registers. */
    sub     sp, sp, #0x50
    stmea   sp, {r0-r12}

    /* Save the remaining registers. */
    str     lr, [sp, #0x38]
    mov     r12, sp

    /* abort mode */
    cpsid   i, #0x17

    /* Save more registers */
    str     lr, [r12, #0x3C]
    mrs     r4, spsr
    str     r4, [r12, #0x40]

    /* supervisor */
    cpsid   i, #0x13

    /* Get DFSR */
    mrc     p15, 0, r5, c5, c0, 0

    /* Get FAR */
    mrc     p15, 0, r6, c6, c0, 0

    /* Save them on the stack */
    str     r5, [sp, #0x44]
    str     r6, [sp, #0x48]

    /* Go to abort handler */
    add     r12, r12, #0x50
    str     r12, [sp, #0x34]
    sub     r12, r12, #0x50
    mov     r0, sp
    mov     r1, #4      // Data Abort
    bl      _sleh_abort
    b       return_to_kernel

/**
 * fleh_dataexc
 *
 * "Reserved" by ARM standards.
 */
EnterARM(fleh_dataexc)
    b       .

/**
 * fleh_irq
 *
 * Dispatch timer and IRQ events.
 */
EnterARM(fleh_irq)
    /* Check to see if the IRQ ocurred in user or in kernel mode */
    sub     lr, lr, #4
    mrs     sp, spsr
    tst     sp, #0xF
    bne     irqhandler_from_kernel

irqhandler_from_user:
    LoadThreadRegister(sp)
    ldr     sp, [sp, TH_PCB_USS]
    stmea   sp, {r0-lr}^
    str     lr, [sp, #0x3C]
    mrs     r0, spsr
    str     r0, [sp, #0x40]
    mov     r5, sp
    LOAD_ADDR(sp, irqstack_top)
    b       irq_join

irqhandler_from_kernel:
    /* Set up IRQ stack */
    LOAD_ADDR(sp, irqstack_top)

    /* Now save the registers */
    sub     sp, sp, #0x50
    stmea   sp, {r0-r12}

    /* Set t-bit */
    orr     lr, lr, #1

    /* save lr */
    str     lr, [sp, #0x3C]

    /* Save SPSR */
    mrs     r4, spsr
    str     r4, [sp, #0x40]

    /* Change modes */
    mov     r5, sp
    and     r4, r4, #0x1F
    orr     r4, r4, #0xC0
    msr     cpsr_c, r4

    /* save sp */
    str     sp, [r5, #0x34]
    str     lr, [r5, #0x38]

    /* disable interrupts */
    cpsid   i, #0x12

irq_join:
    mov     r0, r5
    blx     _irq_handler
    mrs     r4, spsr
    tst     r4, #0xf
    beq     irq_restore_user

irq_restore_kernel:
    /* Restore kernel registers and threads */
    ldr     r4, [sp, #0x40]
    tst     r4, #0x80
    movne   r0, #1
    bne     irqvec_panic

    /* Threads. */
    LoadThreadRegister(r12)
    ldr     r0, [r12, MACHINE_THREAD_PREEMPT_COUNT]

    /* Preempt thread if necessary */
    bne     restore_kernel_context

irq_preempt:
    /* Prreempt thread */
    ldmfd   sp, {r0-r12}
    mrs     lr, spsr

    /* Switch modes */
    and     lr, lr, #0x1F
    orr     lr, lr, #0xC0
    msr     cpsr_c, lr

    /* Save registers */
    sub     sp, sp, #0x50
    stmea   sp, {r0-r12}
    mov     r5, sp

    /* Switch modes, disable interrupts */
    cpsid   i, #0x12

    /* Set r0-r3 */
    ldr     r0, [sp, #0x34]
    str     r0, [r5, #0x34]

    ldr     r0, [sp, #0x38]
    str     r0, [r5, #0x38]

    ldr     r0, [sp, #0x3c]
    str     r0, [r5, #0x3c]

    ldr     r0, [sp, #0x40]
    str     r0, [r5, #0x40]

    mov     sp, r5

    /* Return to the kernel */
    b       return_to_kernel
irq_restore_user:
    cpsid   i, #0x13
    LoadThreadRegister(sp)
    ldr     sp, [sp, TH_PCB_ISS]
    b       _thread_exception_return

return_to_kernel:
    mov     r5, sp

    /* Switch modes */
    cpsid   i, #0x13

    /* Check preemption */
    ldr     r4, [sp, #0x40]
    tst     r4, #0x80
    bne     restart_kernel_execution

    /* Check preemption count */
    LoadThreadRegister(r12)
    ldr     r0, [r12, MACHINE_THREAD_PREEMPT_COUNT]
    cmp     r0, #0
    bne     restart_kernel_execution

preempt_kernel:
    mov     r0, #7
    mov     r1, #0
    blx     _ast_taken

restart_kernel_execution:
    ldr     r6, [r5, #0x40]

    /* Switch modes */
    and     r6, r6, #0x1F
    orr     r6, r6, #0xC0
    msr     cpsr_c, r6
    ldr     lr, [r5, #0x38]
    ldr     sp, [r5, #0x34]
    cpsid   i, #0x17
    mov     sp, r5

    /* The following routine should also be shared with data abort. */
restore_kernel_context:
    ldmfd   sp, {r0-r12}
    add     sp, sp, #0x3C
    rfefd   sp!
    b       .

/*
 * vector panic helper.
 */
irqvec_panic:
    LOAD_ADDR(sp, irqstack_top)
    mov     r1, r0
    adr     r0, vecPanicString
    blx     _panic
    b       .

vecPanicString:
    .asciz  "fleh_vectors: exception in exception vectors, 0x%08x"

LOAD_ADDR_GEN_DEF(irqstack_top)
LOAD_ADDR_GEN_DEF(kern_invalid)
LOAD_ADDR_GEN_DEF(mach_trap_table)

