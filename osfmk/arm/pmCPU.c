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
 * ARM processor power management.
 */

#include <mach/mach_types.h>
#include <mach/vm_param.h>
#include <kern/debug.h>
#include <arm/cpu_data.h>
#include <kern/thread.h>
#include <arm/misc_protos.h>

void thread_tell_urgency(int urgency, uint64_t rt_period, uint64_t rt_deadline)
{
    return;
}

boolean_t machine_processor_is_inactive(processor_t processor)
{
    return (FALSE);
}

void machine_idle(void)
{
    __asm__ __volatile("cpsie if");
}

/******************************************************************************
 *
 * All of the following are deprecated interfaces and no longer used.
 *
 ******************************************************************************/
kern_return_t
pmsControl(__unused uint32_t request, __unused user_addr_t reqaddr,
	   __unused uint32_t reqsize)
{
    return(KERN_SUCCESS);
}

void
pmsInit(void)
{
}

void
pmsStart(void)
{
}

void
pmsPark(void)
{
}

void
pmsRun(__unused uint32_t nstep)
{
}

kern_return_t
pmsBuild(__unused pmsDef *pd, __unused uint32_t pdsize,
	 __unused pmsSetFunc_t *functab,
	 __unused uint32_t platformData, __unused pmsQueryFunc_t queryFunc)
{
    return(KERN_SUCCESS);
}
