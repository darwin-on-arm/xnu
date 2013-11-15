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
 * ARM processor data
 */

#ifndef _ARM_CPU_DATA_H_
#define _ARM_CPU_DATA_H_

#include <mach_assert.h>

#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/queue.h>
#include <kern/processor.h>
#include <kern/pms.h>
#include <pexpert/pexpert.h>
#include <mach/arm/thread_status.h>
#include <mach/arm/vm_param.h>

/*
 * The current thread is returned from an assembly routine.
 */
extern thread_t current_thread(void);

/*
 * The boot processor.
 */
extern struct processor BootProcessor;

#ifndef offsetof
#define offsetof(TYPE,MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

typedef enum {
    TASK_MAP_32BIT,             /* 32-bit user, compatibility mode */
} task_map_t;

typedef struct rtclock_timer {
    mpqueue_head_t queue;
    uint64_t deadline;
    uint64_t when_set;
    boolean_t has_expired;
} rtclock_timer_t;

/*
 * The core processor data. This is also referred to as the processor
 * control region block (PCRB) or processor control block (PCB). Use
 * whatever terminology suits you on this fine day.
 */
typedef struct cpu_data {
    struct cpu_data *cpu_this;  /* pointer to myself */
    thread_t cpu_active_thread;
    int cpu_preemption_level;
    int cpu_number;             /* Logical CPU */
    void *cpu_int_state;        /* interrupt state */
    vm_offset_t cpu_active_stack;   /* kernel stack base */
    vm_offset_t cpu_kernel_stack;   /* kernel stack top */
    vm_offset_t cpu_int_stack_top;
    int cpu_interrupt_level;
    int cpu_phys_number;        /* Physical CPU */
    cpu_id_t cpu_id;            /* Platform Expert */
    int cpu_signals;            /* IPI events */
    int cpu_prior_signals;      /* Last set of events, and debugging. */
    int cpu_mcount_off;         /* mcount recursion */
    int cpu_type;
    int cpu_subtype;
    int cpu_threadtype;
    int cpu_running;
    struct processor *cpu_processor;
    uint64_t debugger_entry_time;
    arm_saved_state_t *cpu_fatal_trap_state;
    arm_saved_state_t *cpu_post_fatal_trap_state;
    caddr_t cpu_onfault;        /* for alignment faults */
    int cpu_pending_ast;
    uint64_t interrupt_entry_time;
    uint32_t interrupt_count[8];
    uint32_t rtcPop;
    struct pmap *user_pmap;
    uint64_t absolute_time;
    rtclock_timer_t rt_timer;
    thread_t old_thread;
    thread_t new_thread;

    IOInterruptHandler handler;     /* for IOKit */
    void* nub;
    void* target;
    void* refCon;

} cpu_data_t;

extern cpu_data_t *cpu_data_ptr[];
extern cpu_data_t cpu_data_master;

static inline cpu_data_t *cpu_datap(int cpu)
{
    return cpu_data_ptr[cpu];
}

#define disable_preemption              _disable_preemption
#define enable_preemption               _enable_preemption
#define enable_preemption_no_check        _enable_preemption_no_check
#define mp_disable_preemption           _disable_preemption
#define mp_enable_preemption            _enable_preemption
#define mp_enable_preemption_no_check        _enable_preemption_no_check

extern int get_preemption_level(void);
extern void disable_preemption(void);
extern void enable_preemption(void);
extern void enable_preemption_no_check(void);
extern void mp_disable_preemption(void);
extern void mp_enable_preemption(void);
extern void mp_enable_preemption_no_check(void);
extern int get_simple_lock_count(void);

int get_interrupt_level(void);
int get_cpu_number(void);
int get_cpu_phys_number(void);

cpu_data_t *current_cpu_datap(void);

#endif
