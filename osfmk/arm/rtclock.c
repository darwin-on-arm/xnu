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
 * ARM realtime clock.
 */

#include <platforms.h>

#include <mach/mach_types.h>

#include <kern/cpu_data.h>
#include <kern/cpu_number.h>
#include <kern/clock.h>
#include <kern/host_notify.h>
#include <kern/macro_help.h>
#include <kern/misc_protos.h>
#include <kern/spl.h>
#include <kern/assert.h>
#include <kern/etimer.h>
#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>         /* for kernel_map */
#include <pexpert/pexpert.h>
#include <machine/limits.h>
#include <machine/commpage.h>
#include <sys/kdebug.h>

#include <arm/machine_cpu.h>

#include <pexpert/pexpert.h>
#include <pexpert/arm/boot.h>
#include <pexpert/arm/protos.h>

#include <libkern/OSBase.h>

static uint32_t rtclock_sec_divisor;
static uint64_t rtclock_min_decrementer = 0;
static uint64_t rtclock_max_decrementer = 100000000;

static uint64_t rtclock_scaler = 0;

extern uint64_t clock_decrementer;

static uint64_t deadline_to_decrementer(uint64_t deadline, uint64_t now)
{
    uint64_t delta;

    if (deadline <= now)
        return 0;
    else {
        delta = (deadline) - now;
        //kprintf("wat %llu %llu %llu %llu\n", delta, deadline, now, (deadline - now));
        return (delta);
    }
}

int setPop(uint64_t time)
{
    uint64_t now;
    uint64_t pop;

    /*
     * 0 and EndOfAllTime are special-cases for "clear the timer" 
     */
    if (time == 0 || time == EndOfAllTime) {
        time = EndOfAllTime;
        now = 0;
        pop = deadline_to_decrementer(0, 0);
    } else {
        now = pe_arm_get_timebase(NULL);   /* The time in nanoseconds */
        pop = deadline_to_decrementer(time, now);
    }

    /*
     * Record requested and actual deadlines set 
     */
    current_cpu_datap()->rtcPop = pop;

    return (pop - now);
}

void rtclock_intr(arm_saved_state_t * regs)
{
    /*
     * Interrupts must be enabled. 
     */

    spl_t x = splclock();
    etimer_intr(0, 0);
    splx(x);

    return;
}

uint64_t mach_absolute_time(void)
{
    if (!rtclock_sec_divisor)
        return 0;
    return pe_arm_get_timebase(NULL);
}

void machine_delay_until(uint64_t interval, uint64_t deadline)
{
    uint64_t now;
    
    do {
        cpu_pause();
        now = mach_absolute_time();
    } while (now < deadline);
}

static inline uint32_t _absolutetime_to_microtime(uint64_t abstime,
                                                  clock_sec_t * secs,
                                                  clock_usec_t * microsecs)
{
    uint32_t remain;
    *secs = (abstime * rtclock_scaler) / (uint64_t) NSEC_PER_SEC;
    remain = (uint32_t) ((abstime * rtclock_scaler) % (uint64_t) NSEC_PER_SEC);
    *microsecs = remain / NSEC_PER_USEC;
    return remain;
}

static inline void _absolutetime_to_nanotime(uint64_t abstime,
                                             clock_sec_t * secs,
                                             clock_usec_t * nanosecs)
{
    *secs = (abstime * rtclock_scaler) / (uint64_t) NSEC_PER_SEC;
    *nanosecs =
        (clock_usec_t) ((abstime * rtclock_scaler) % (uint64_t) NSEC_PER_SEC);
}

void clock_interval_to_absolutetime_interval(uint32_t interval,
                                             uint32_t scale_factor,
                                             uint64_t * result)
{
    *result = ((uint64_t) interval * scale_factor) / rtclock_scaler;
}

void absolutetime_to_microtime(uint64_t abstime, clock_sec_t * secs,
                               clock_usec_t * microsecs)
{
    _absolutetime_to_microtime(abstime, secs, microsecs);
}

void absolutetime_to_nanotime(uint64_t abstime, clock_sec_t * secs,
                              clock_nsec_t * nanosecs)
{
    _absolutetime_to_nanotime(abstime, secs, nanosecs);
}

void nanotime_to_absolutetime(clock_sec_t secs, clock_nsec_t nanosecs,
                              uint64_t * result)
{
    *result = (((uint64_t) secs * NSEC_PER_SEC) + nanosecs) / rtclock_scaler;
}

void absolutetime_to_nanoseconds(uint64_t abstime, uint64_t * result)
{
    *result = abstime * rtclock_scaler;
}

void clock_get_system_nanotime(clock_sec_t * secs, clock_nsec_t * nanosecs)
{
    uint64_t now = pe_arm_get_timebase(NULL);

    _absolutetime_to_nanotime(now, secs, nanosecs);
}

void clock_get_system_microtime(clock_sec_t * secs, clock_usec_t * microsecs)
{
    uint64_t now = mach_absolute_time();
    uint32_t remain;

    *secs = (now * rtclock_scaler) / (uint64_t) NSEC_PER_SEC;
    remain = (uint32_t) ((now * rtclock_scaler) % (uint64_t) NSEC_PER_SEC);
    *microsecs = remain / NSEC_PER_USEC;
#if 0
    uint64_t now, t64;
    uint32_t divisor;

    now = mach_absolute_time();
    *secs = t64 = now / (divisor = rtclock_scaler);
    now -= (t64 * divisor);
    *microsecs = (now * USEC_PER_SEC) / divisor;
#endif
}

void nanoseconds_to_absolutetime(uint64_t nanoseconds, uint64_t * result)
{
    kprintf("nanoseconds_to_absolutetime: %llu => %llu\n", nanoseconds,
            nanoseconds / rtclock_scaler);
    *result = nanoseconds / rtclock_scaler;
}

int rtclock_config(void)
{
    /*
     * nothing to do 
     */
    return (1);
}

void rtc_configure(uint64_t hz)
{
    rtclock_sec_divisor = hz;
    return;
}

int rtclock_init(void)
{
    kprintf("rtclock_init: hello\n");

    /*
     * Interrupts must be initialized for the timer to work 
     */
    assert(!ml_get_interrupts_enabled());

    /*
     * initialize system dependent timer 
     */
    pe_arm_init_timebase(NULL);

    /*
     * Check the divisor 
     */
    if (!rtclock_sec_divisor) {
        panic
            ("Invalid RTC second divisor, perhaps the RTC wasn't configured correctly?");
    }

    rtclock_scaler = (NSEC_PER_SEC / rtclock_sec_divisor);

    /*
     * Did they go away? 
     */
    ml_set_interrupts_enabled(TRUE);

    current_cpu_datap()->rtcPop = 0;

    clock_timebase_init();

    etimer_resync_deadlines();

    return 1;
}

void clock_timebase_info(mach_timebase_info_t info)
{
    info->numer = info->denom = 1;
}


void clock_gettimeofday_set_commpage(uint64_t abstime, uint64_t epoch,
                                     uint64_t offset, clock_sec_t * secs,
                                     clock_usec_t * microsecs)
{
    uint64_t now = abstime + offset;
    uint32_t remain;

    remain = _absolutetime_to_microtime(now, secs, microsecs);
    *secs += (clock_sec_t)epoch;
    commpage_set_timestamp(abstime - remain, *secs, 0);
}


