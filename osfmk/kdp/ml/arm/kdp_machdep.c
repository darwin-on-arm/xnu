/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
 
#include <mach_kdp.h>
#include <mach/mach_types.h>
#include <mach/machine.h>
#include <mach/exception_types.h>
#include <kern/cpu_data.h>
#include <arm/trap.h>
#include <arm/mp.h>
#include <kdp/kdp_internal.h>
#include <kdp/kdp_callout.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <IOKit/IOPlatformExpert.h> /* for PE_halt_restart */
#include <kern/machine.h> /* for halt_all_cpus */
#include <libkern/OSAtomic.h>

#include <kern/thread.h>
#include <arm/thread.h>
#include <vm/vm_map.h>
#include <arm/pmap.h>
#include <kern/kalloc.h>

#define KDP_TEST_HARNESS 0
#if KDP_TEST_HARNESS
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

extern cpu_type_t cpuid_cputype(void);
extern cpu_subtype_t cpuid_cpusubtype(void);

void		print_saved_state(void *);
void		kdp_call(void);
int		kdp_getc(void);
boolean_t	kdp_call_kdb(void);
void		kdp_getstate(arm_thread_state_t *);
void		kdp_setstate(arm_thread_state_t *);
void		kdp_print_phys(int);

int
machine_trace_thread(thread_t thread, char *tracepos, char *tracebound, int nframes, boolean_t user_p);

int
machine_trace_thread64(thread_t thread, char *tracepos, char *tracebound, int nframes, boolean_t user_p);

unsigned
machine_read64(addr64_t srcaddr, caddr_t dstaddr, uint32_t len);

static void	kdp_callouts(kdp_event_t event);

void
kdp_exception(
    unsigned char	*pkt,
    int	*len,
    unsigned short	*remote_port,
    unsigned int	exception,
    unsigned int	code,
    unsigned int	subcode
)
{
    kdp_exception_t	*rq = (kdp_exception_t *)pkt;

    rq->hdr.request = KDP_EXCEPTION;
    rq->hdr.is_reply = 0;
    rq->hdr.seq = kdp.exception_seq;
    rq->hdr.key = 0;
    rq->hdr.len = sizeof (*rq);
    
    rq->n_exc_info = 1;
    rq->exc_info[0].cpu = 0;
    rq->exc_info[0].exception = exception;
    rq->exc_info[0].code = code;
    rq->exc_info[0].subcode = subcode;
    
    rq->hdr.len += rq->n_exc_info * sizeof (kdp_exc_info_t);
    
    bcopy((char *)rq, (char *)pkt, rq->hdr.len);

    kdp.exception_ack_needed = TRUE;
    
    *remote_port = kdp.exception_port;
    *len = rq->hdr.len;
}

boolean_t
kdp_exception_ack(
    unsigned char	*pkt,
    int			len
)
{
    kdp_exception_ack_t	*rq = (kdp_exception_ack_t *)pkt;

    if (((unsigned int) len) < sizeof (*rq))
	return(FALSE);
	
    if (!rq->hdr.is_reply || rq->hdr.request != KDP_EXCEPTION)
    	return(FALSE);
	
    dprintf(("kdp_exception_ack seq %x %x\n", rq->hdr.seq, kdp.exception_seq));
	
    if (rq->hdr.seq == kdp.exception_seq) {
	kdp.exception_ack_needed = FALSE;
	kdp.exception_seq++;
    }
    return(TRUE);
}

void
kdp_getstate(
    arm_thread_state_t 	*state
)
{
    static arm_thread_state_t	null_state;
    arm_thread_state_t *saved_state;
    int i;
    
    saved_state = (arm_thread_state_t *)kdp.saved_state;
    
    *state = null_state;
    
    for(i = 0; i <= 13; i++) {
        state->r[i] = saved_state->r[i];
    }
    
    state->sp = saved_state->sp;
    state->lr = saved_state->lr;
    state->pc = saved_state->pc;
    state->cpsr = saved_state->cpsr;
}


void
kdp_setstate(
    arm_thread_state_t	*state
)
{
    arm_thread_state_t		*saved_state;
    int i;
    
    saved_state = (arm_thread_state_t *)kdp.saved_state;

    for(i = 0; i <= 13; i++) {
        saved_state->r[i] = state->r[i];
    }
    
    saved_state->sp = state->sp;
    saved_state->lr = state->lr;
    saved_state->pc = state->pc;
    saved_state->cpsr = state->cpsr;

}


kdp_error_t
kdp_machine_read_regs(
    __unused unsigned int cpu,
    __unused unsigned int flavor,
    char *data,
    __unused int *size
)
{
    static arm_vfp_state_t  null_fpstate;

    switch (flavor) {

    case ARM_THREAD_STATE:
	dprintf(("kdp_readregs THREAD_STATE\n"));
	kdp_getstate((arm_thread_state_t *)data);
	*size = sizeof (arm_thread_state_t);
	return KDPERR_NO_ERROR;
	
    case ARM_VFP_STATE:
	dprintf(("kdp_readregs THREAD_FPSTATE\n"));
	*(arm_vfp_state_t *)data = null_fpstate;
	*size = sizeof (arm_vfp_state_t);
	return KDPERR_NO_ERROR;
	
    default:
	dprintf(("kdp_readregs bad flavor %d, probably not implented yet\n", flavor));
	*size = 0;
	return KDPERR_BADFLAVOR;
    }
}

kdp_error_t
kdp_machine_write_regs(
    __unused unsigned int cpu,
    unsigned int flavor,
    char *data,
    __unused int *size
)
{
    switch (flavor) {

    case ARM_THREAD_STATE:
	dprintf(("kdp_writeregs THREAD_STATE\n"));
	kdp_setstate((arm_thread_state_t *)data);
	return KDPERR_NO_ERROR;
	
    case ARM_VFP_STATE:
	dprintf(("kdp_writeregs THREAD_FPSTATE\n"));
	return KDPERR_NO_ERROR;
	
    default:
	dprintf(("kdp_writeregs bad flavor %d, not done?\n"));
	return KDPERR_BADFLAVOR;
    }
}



void
kdp_machine_hostinfo(
    kdp_hostinfo_t *hostinfo
)
{
    int			i;

    hostinfo->cpus_mask = 0;

    for (i = 0; i < machine_info.max_cpus; i++) {
	if (cpu_data_ptr[i] == NULL)
            continue;
	
        hostinfo->cpus_mask |= (1 << i);
    }

    hostinfo->cpu_type = cpu_type();
    hostinfo->cpu_subtype = cpu_subtype();
}

void
kdp_panic(
    const char		*msg
)
{
    kprintf("kdp panic: %s\n", msg);    
    while(1) { };
}


void
kdp_machine_reboot(void)
{
	printf("Attempting system restart...");
	kprintf("Attempting system restart...");
	/* Call the platform specific restart*/
	if (PE_halt_restart)
		(*PE_halt_restart)(kPERestartCPU);
	/* If we do reach this, give up */
	halt_all_cpus(TRUE);
}

int
kdp_intr_disbl(void)
{
   return splhigh();
}

void
kdp_intr_enbl(int s)
{
	splx(s);
}

int
kdp_getc(void)
{
	return	cnmaygetc();
}

void
kdp_us_spin(int usec)
{
    delay(usec/100);
}

void print_saved_state(void *state)
{
    arm_thread_state_t		*saved_state;

    saved_state = state;

	kprintf("pc = 0x%x\n", saved_state->pc);
	kprintf("sp = %p\n", saved_state);

}

void
kdp_sync_cache(void)
{
	return;	/* No op here. */
}

void
kdp_call(void)
{
    
}

static struct kdp_callout {
	struct kdp_callout	*callout_next;
	kdp_callout_fn_t	callout_fn;
	void			*callout_arg;
} *kdp_callout_list = NULL;


/*
 * Called from kernel context to register a kdp event callout.
 */
void
kdp_register_callout(
	kdp_callout_fn_t	fn,
	void			*arg)
{
	struct kdp_callout	*kcp;
	struct kdp_callout	*list_head;

	kcp = kalloc(sizeof(*kcp));
	if (kcp == NULL)
		panic("kdp_register_callout() kalloc failed");

	kcp->callout_fn  = fn;
	kcp->callout_arg = arg;

	/* Lock-less list insertion using compare and exchange. */
	do {
		list_head = kdp_callout_list;
		kcp->callout_next = list_head;
	} while (!OSCompareAndSwapPtr(list_head, kcp, (void * volatile *)&kdp_callout_list));
}

/*
 * Called at exception/panic time when extering or exiting kdp.  
 * We are single-threaded at this time and so we don't use locks.
 */
static void
kdp_callouts(kdp_event_t event)
{
	struct kdp_callout	*kcp = kdp_callout_list;

	while (kcp) {
		kcp->callout_fn(kcp->callout_arg, event); 
		kcp = kcp->callout_next;
	}	
}

void
kdp_ml_enter_debugger(void)
{

}


int
kdp_machine_ioport_read(kdp_readioport_req_t *rq, caddr_t data, uint16_t lcpu)
{
    return 0;
}

int
kdp_machine_ioport_write(kdp_writeioport_req_t *rq, caddr_t data, uint16_t lcpu)
{
    return 0;
}

int
kdp_machine_msr64_read(kdp_readmsr64_req_t *rq, caddr_t data, uint16_t lcpu)
{
    return 0;
}

int
kdp_machine_msr64_write(kdp_writemsr64_req_t *rq, __unused caddr_t data, uint16_t lcpu)
{
    return 0;
}
