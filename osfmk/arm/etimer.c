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
 * ARM event timer.
 */

#include <mach/mach_types.h>

#include <kern/clock.h>
#include <kern/thread.h>
#include <kern/timer_queue.h>
#include <kern/processor.h>
#include <kern/macro_help.h>
#include <kern/spl.h>
#include <kern/etimer.h>
#include <kern/pms.h>
#include <kern/queue.h>

#include <machine/commpage.h>
#include <machine/machine_routines.h>

#ifndef __LP64__

/**
 * timer_grab
 *
 * Return the raw timer value from the timer.
 */
uint64_t timer_grab(timer_t timer)
{
    return *(uint64_t *) timer;
}
#endif

/**
 * timer_update
 *
 * Update the raw timer value.
 */
void timer_update(timer_t timer, uint32_t new_high, uint32_t new_low)
{
    uint64_t *ptr = (uint64_t *) timer;
    *ptr = ((uint64_t) (new_high) << 32) | new_low;
}

/**
 * etimer_set_deadline
 *
 * Set the clock deadline.
 */
void etimer_set_deadline(uint64_t deadline)
{
    rtclock_timer_t *mytimer;
    spl_t s;
    cpu_data_t *pp;

    s = splclock();             /* no interruptions */
    pp = current_cpu_datap();

    mytimer = &pp->rt_timer;    /* Point to the timer itself */
    mytimer->deadline = deadline;   /* Set the new expiration time */

    etimer_resync_deadlines();

    splx(s);
}

/**
 * etimer_resync_deadlines
 *
 * Resynchronize the timer deadlines, called from the interrupt routine.
 */

void etimer_resync_deadlines(void)
{
    uint64_t deadline;
    uint64_t pmdeadline;
    rtclock_timer_t *mytimer;
    spl_t s = splclock();
    cpu_data_t *pp;
    uint32_t decr;

    pp = current_cpu_datap();
    deadline = EndOfAllTime;

    /*
     * If we have a clock timer set, pick that.
     */
    mytimer = &pp->rt_timer;
    if (!mytimer->has_expired && 0 < mytimer->deadline
        && mytimer->deadline < EndOfAllTime)
        deadline = mytimer->deadline;

    /*
     * Go and set the "pop" event.
     */

    if (deadline > 0 && deadline <= pp->rtcPop) {
        int decr;
        uint64_t now;

        now = mach_absolute_time();
        decr = setPop(deadline);

        if (deadline < now) {
            pp->rtcPop = now + decr;
        } else {
            pp->rtcPop = deadline;
        }
    }

    splx(s);
}

/**
 * etimer_intr
 *
 * Timer interrupt routine, called from the realtime clock interrupt
 * routine.
 */
void etimer_intr(int inuser, uint64_t iaddr)
{
    uint64_t abstime;
    rtclock_timer_t *mytimer;
    cpu_data_t *pp;
    int32_t latency;

    pp = current_cpu_datap();

    SCHED_STATS_TIMER_POP(current_processor());

    abstime = mach_absolute_time(); /* Get the time now */

    /*
     * has a pending clock timer expired? 
     */
    mytimer = &pp->rt_timer;    /* Point to the event timer */
    if (mytimer->deadline <= abstime) {
        mytimer->has_expired = TRUE;    /* Remember that we popped */
        mytimer->deadline = timer_queue_expire(&mytimer->queue, abstime);
        mytimer->has_expired = FALSE;
    }

    pp->rtcPop = EndOfAllTime;  /* any real deadline will be earlier */
    /*
     * schedule our next deadline 
     */

    etimer_resync_deadlines();
}

/**
 * timer_queue_assign
 *
 * Assign a deadline and return the current processor's timer queue.
 */
mpqueue_head_t *timer_queue_assign(uint64_t deadline)
{
    cpu_data_t *cdp = current_cpu_datap();
    mpqueue_head_t *queue;

    if (cdp->cpu_running) {
        queue = &cdp->rt_timer.queue;
        if (deadline < cdp->rt_timer.deadline) {
            etimer_set_deadline(deadline);
        }
    } else {
        queue = &cpu_datap(master_cpu)->rt_timer.queue;
    }

    return (queue);
}

/**
 * timer_call_slop
 *
 * Used for coalescing timer deadlines.
 */
uint64_t timer_call_slop(uint64_t deadline)
{
    uint64_t now = mach_absolute_time();
    if (deadline > now) {
        return MIN((deadline - now) >> 3, NSEC_PER_MSEC);   /* Min of 12.5% and 1ms */
    }
}

/**
 * timer_queue_cancel
 *
 * Remove a timer from the queue.
 */
void timer_queue_cancel(mpqueue_head_t * queue, uint64_t deadline,
                        uint64_t new_deadline)
{
    if (queue == &current_cpu_datap()->rt_timer.queue) {
        if (deadline < new_deadline) {
            etimer_set_deadline(new_deadline);
        }
    }
}
