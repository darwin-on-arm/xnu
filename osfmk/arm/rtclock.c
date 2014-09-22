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
static uint64_t rtclock_scaler = 0;

static uint64_t rtc_decrementer_min;
static uint64_t rtc_decrementer_max;

extern uint64_t clock_decrementer;

#define UI_CPUFREQ_ROUNDING_FACTOR  10000000

/* Decimal powers: */
#define kilo (1000ULL)
#define Mega (kilo * kilo)
#define Giga (kilo * Mega)
#define Tera (kilo * Giga)
#define Peta (kilo * Tera)

uint64_t tscFreq = 0, tscFCvtt2n, tscFCvtn2t;

uint64_t rtclock_timer_scale_convert(void)
{
    uint64_t ticks;
    uint64_t nsecs = pe_arm_get_timebase(NULL);

    /* nanoseconds = (((rdtsc - rnt_tsc_base) * 10e9) / tscFreq) - rnt_ns_base; */
    if(!tscFreq)
        return 0;

    ticks = (nsecs * NSEC_PER_SEC / tscFreq);
    return ticks;
}

static uint64_t deadline_to_decrementer(uint64_t deadline, uint64_t now)
{
    uint64_t  delta;
    if (deadline <= now)
        return rtc_decrementer_min;
    else {
        delta = deadline - now;
        return MIN(MAX(rtc_decrementer_min,delta),rtc_decrementer_max); 
    }
}

/*
 * The units are a 64-bit fixed point integer which contains a high 32-bit integer part
 * and a low 32-bit fractional part.
 */
uint64_t tmrCvt(uint64_t time, uint64_t unit)
{
    return ((time * unit) >> 32);
}

static uint64_t rtclock_internal_arm_timer(uint64_t deadline, uint64_t now)
{
    uint64_t current = pe_arm_get_timebase(NULL);
    uint64_t set = 0;

    if(deadline) {
        uint64_t delta = deadline_to_decrementer(deadline, now);
        uint64_t delta_abs;

        set = now + delta;

        /* Convert to Absolute frequency. */
        delta_abs = tmrCvt(delta, tscFCvtn2t);

#if CONFIG_TICKLESS
        /* Set internal decrementer and rearm. */        
        clock_decrementer = delta_abs;
#endif

    } else {
#if CONFIG_TICKLESS
        printf("disarming timer...\n");
        clock_decrementer = EndOfAllTime;
#endif
    }

#if CONFIG_TICKLESS
    /* Rearm decrementer by disable-reenable. This resets the timer. */
    pe_arm_set_timer_enabled(FALSE);
    pe_arm_set_timer_enabled(TRUE);
#endif
    
    return set;
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
        pop = rtclock_internal_arm_timer(0, 0);
    } else {
        now = mach_absolute_time();       /* The time in nanoseconds */
        pop = rtclock_internal_arm_timer(time, now);
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
    return rtclock_timer_scale_convert();
}

void machine_delay_until(uint64_t interval, uint64_t deadline)
{
    uint64_t now;
    
    do {
        cpu_pause();
        now = mach_absolute_time();
    } while (now < deadline);
}

static inline uint32_t
_absolutetime_to_microtime(uint64_t abstime, clock_sec_t *secs, clock_usec_t *microsecs)
{
    uint32_t remain;
    *secs = abstime / (uint64_t)NSEC_PER_SEC;
    remain = (uint32_t)(abstime % (uint64_t)NSEC_PER_SEC);
    *microsecs = remain / NSEC_PER_USEC;
    return remain;
}

static inline void
_absolutetime_to_nanotime(uint64_t abstime, clock_sec_t *secs, clock_usec_t *nanosecs)
{
    *secs = abstime / (uint64_t)NSEC_PER_SEC;
    *nanosecs = (clock_usec_t)(abstime % (uint64_t)NSEC_PER_SEC);
}

void
clock_interval_to_absolutetime_interval(uint32_t interval, uint32_t scale_factor, uint64_t* result)
{
    *result = (uint64_t)interval * scale_factor;
}

void
absolutetime_to_microtime(uint64_t abstime, clock_sec_t *secs, clock_usec_t *microsecs)
{
    _absolutetime_to_microtime(abstime, secs, microsecs);
}

void
absolutetime_to_nanotime(uint64_t abstime, clock_sec_t* secs, clock_nsec_t *nanosecs)
{
    _absolutetime_to_nanotime(abstime, secs, nanosecs);
}

void
nanotime_to_absolutetime(clock_sec_t secs, clock_nsec_t nanosecs, uint64_t* result)
{
    *result = ((uint64_t)secs * NSEC_PER_SEC) + nanosecs;
}

void
absolutetime_to_nanoseconds(uint64_t abstime, uint64_t* result)
{
    *result = abstime;
}

void
nanoseconds_to_absolutetime(uint64_t nanoseconds, uint64_t* result)
{
    *result = nanoseconds;
}

void clock_get_system_nanotime(clock_sec_t * secs, clock_nsec_t * nanosecs)
{
    uint64_t now = pe_arm_get_timebase(NULL);

    _absolutetime_to_nanotime(now, secs, nanosecs);
}

void clock_get_system_microtime(clock_sec_t * secs, clock_usec_t * microsecs)
{
    uint64_t  now = mach_absolute_time();
    _absolutetime_to_microtime(now, secs, microsecs);
}

int rtclock_config(void)
{
    return (1);
}

void rtc_configure(uint64_t hz)
{
    rtclock_sec_divisor = hz;
    return;
}

void rtclock_bus_configure(void)
{
    uint64_t cycles;
    kprintf("rtclock_bus_configure: Configuring RTCLOCK...\n");

    /* We need a Hz, this will be specified in ...Hz. */
    if(!gPEClockFrequencyInfo.timebase_frequency_hz)
        panic("rtclock_bus_configure");

    if(!rtclock_sec_divisor)
        panic("rtclock_bus_configure 2");

    cycles = gPEClockFrequencyInfo.timebase_frequency_hz;

    /* Calculate 'TSC' frequency. */
    tscFreq = cycles;
    tscFCvtt2n = ((1 * Giga) << 32) / tscFreq;
    tscFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / tscFCvtt2n;

    kprintf("[RTCLOCK] Frequency = %6d.%06dMHz, "
        "cvtt2n = %08X.%08X, cvtn2t = %08X.%08X\n",
        (uint32_t)(tscFreq / Mega),
        (uint32_t)(tscFreq % Mega), 
        (uint32_t)(tscFCvtt2n >> 32), (uint32_t)tscFCvtt2n,
        (uint32_t)(tscFCvtn2t >> 32), (uint32_t)tscFCvtn2t);

    rtc_decrementer_max = UINT64_MAX; /* No limit. */
    rtc_decrementer_min = 1*NSEC_PER_USEC;  /* 1 usec */
}

int rtclock_init(void)
{
    uint64_t cycles;
    assert(!ml_get_interrupts_enabled());

    if(cpu_number() == master_cpu) {
        pe_arm_init_timebase(NULL);
        rtclock_bus_configure();

        current_cpu_datap()->rtcPop = 0;
        clock_timebase_init();
    }

    ml_init_lock_timeout();

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


