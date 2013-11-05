/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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

/*
 * @OSF_COPYRIGHT@
 */

/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 *	File:	kern/lock.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Locking primitives implementation
 */

/*
 * ARM locks.
 */

#include <mach_ldebug.h>

#include <kern/lock.h>
#include <kern/locks.h>
#include <kern/kalloc.h>
#include <kern/misc_protos.h>
#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/cpu_data.h>
#include <kern/cpu_number.h>
#include <kern/sched_prim.h>
#include <kern/xpr.h>
#include <kern/debug.h>
#include <string.h>

#include <arm/machine_routines.h>
#include <machine/machine_cpu.h>
#include <arm/mp.h>

#include <sys/kdebug.h>
#include <mach/branch_predicates.h>

#include <arm/misc_protos.h>

/*
 * This file works, don't mess with it.
 */

unsigned int lock_wait_time[2] = { (unsigned int) -1, 0 };

#define	lck_mtx_data	lck_mtx_sw.lck_mtxd.lck_mtxd_data
#define	lck_mtx_waiters	lck_mtx_sw.lck_mtxd.lck_mtxd_waiters
#define	lck_mtx_pri		lck_mtx_sw.lck_mtxd.lck_mtxd_pri

uint32_t LcksOpts;

void lck_rw_ilk_lock(lck_rw_t * lck)
{
    lck_spin_lock(lck);
}

void lck_rw_ilk_unlock(lck_rw_t * lck)
{
    lck_spin_unlock(lck);
}

static void lck_mtx_ext_init(lck_mtx_ext_t * lck, lck_grp_t * grp,
                             lck_attr_t * attr)
{
    bzero((void *) lck, sizeof(lck_mtx_ext_t));

    lck->lck_mtx_grp = grp;

    if (grp->lck_grp_attr & LCK_GRP_ATTR_STAT)
        lck->lck_mtx_attr |= LCK_MTX_ATTR_STAT;
}

lck_mtx_t *lck_mtx_alloc_init(lck_grp_t * grp, lck_attr_t * attr)
{
    lck_mtx_t *lck;

    if ((lck = (lck_mtx_t *) kalloc(sizeof(lck_mtx_t))) != 0)
        lck_mtx_init(lck, grp, attr);

    return (lck);
}

void lck_mtx_free(lck_mtx_t * lck, lck_grp_t * grp)
{
    lck_mtx_destroy(lck, grp);
    kfree((void *) lck, sizeof(lck_mtx_t));
}

void lck_mtx_init(lck_mtx_t * lck, lck_grp_t * grp, lck_attr_t * attr)
{
    lck_mtx_ext_t *lck_ext;
    lck_attr_t *lck_attr;

    if (attr != LCK_ATTR_NULL)
        lck_attr = attr;
    else
        lck_attr = &LockDefaultLckAttr;

    lck->lck_mtx_data = 0;
    lck->lck_mtx_waiters = 0;
    lck->lck_mtx_state = 0;

    lck_grp_reference(grp);
    lck_grp_lckcnt_incr(grp, LCK_TYPE_MTX);
}

void lck_mtx_destroy(lck_mtx_t * lck, lck_grp_t * grp)
{
    boolean_t lck_is_indirect;

    if (lck->lck_mtx_tag == LCK_MTX_TAG_DESTROYED)
        return;

    lck_is_indirect = (lck->lck_mtx_tag == LCK_MTX_TAG_INDIRECT);

    lck_mtx_lock_mark_destroyed(lck);

    if (lck_is_indirect)
        kfree(lck->lck_mtx_ptr, sizeof(lck_mtx_ext_t));

    lck_grp_lckcnt_decr(grp, LCK_TYPE_MTX);
    lck_grp_deallocate(grp);
    return;
}

void lck_mtx_init_ext(lck_mtx_t * lck, lck_mtx_ext_t * lck_ext, lck_grp_t * grp,
                      lck_attr_t * attr)
{
    lck_attr_t *lck_attr;

    if (attr != LCK_ATTR_NULL)
        lck_attr = attr;
    else
        lck_attr = &LockDefaultLckAttr;

    lck->lck_mtx_data = 0;
    lck->lck_mtx_waiters = 0;
    lck->lck_mtx_state = 0;

    lck_grp_reference(grp);
    lck_grp_lckcnt_incr(grp, LCK_TYPE_MTX);

}

lck_rw_t *lck_rw_alloc_init(lck_grp_t * grp, lck_attr_t * attr)
{
    lck_rw_t *lck;

    if ((lck = (lck_rw_t *) kalloc(sizeof(lck_rw_t))) != 0) {
        bzero(lck, sizeof(lck_rw_t));
        lck_rw_init(lck, grp, attr);
    }

    return (lck);
}

void lck_rw_destroy(lck_rw_t * lck, lck_grp_t * grp)
{
    if (lck->lck_rw_tag == LCK_RW_TAG_DESTROYED)
        return;
    lck->lck_rw_tag = LCK_RW_TAG_DESTROYED;
    lck_grp_lckcnt_decr(grp, LCK_TYPE_RW);
    lck_grp_deallocate(grp);
    return;
}

void lck_rw_free(lck_rw_t * lck, lck_grp_t * grp)
{
    lck_rw_destroy(lck, grp);
    kfree(lck, sizeof(lck_rw_t));
}

void lck_rw_init(lck_rw_t * lck, lck_grp_t * grp, lck_attr_t * attr)
{
    lck_attr_t *lck_attr = (attr != LCK_ATTR_NULL) ? attr : &LockDefaultLckAttr;

    lck->lck_rw_interlock = 0;
    lck->lck_rw_want_excl = FALSE;
    lck->lck_rw_want_upgrade = FALSE;
    lck->lck_rw_shared_count = 0;
    lck->lck_rw_waiting = 0;
    lck->lck_rw_tag = 0;
    lck->lck_rw_priv_excl =
        ((lck_attr->lck_attr_val & LCK_ATTR_RW_SHARED_PRIORITY) == 0);

    lck_grp_reference(grp);
    lck_grp_lckcnt_incr(grp, LCK_TYPE_RW);
}

void lck_spin_destroy(lck_spin_t * lck, lck_grp_t * grp)
{
    if (lck->interlock == LCK_SPIN_TAG_DESTROYED)
        return;
    lck->interlock = LCK_SPIN_TAG_DESTROYED;
    lck_grp_lckcnt_decr(grp, LCK_TYPE_SPIN);
    lck_grp_deallocate(grp);
}

void lck_spin_init(lck_spin_t * lck, lck_grp_t * grp,
                   __unused lck_attr_t * attr)
{
    lck->interlock = 0;
    lck_grp_reference(grp);
    lck_grp_lckcnt_incr(grp, LCK_TYPE_SPIN);
}

void lock_init(lock_t * l, boolean_t can_sleep, __unused unsigned short tag,
               __unused unsigned short tag1)
{
    l->lck_rw_interlock = 0;
    l->lck_rw_want_excl = FALSE;
    l->lck_rw_want_upgrade = FALSE;
    l->lck_rw_shared_count = 0;
    l->lck_rw_tag = tag;
    l->lck_rw_priv_excl = 1;
    l->lck_rw_waiting = 0;
}

void lck_rw_assert(lck_rw_t * lck, unsigned int type)
{
    switch (type) {
    case LCK_RW_ASSERT_SHARED:
        if (lck->lck_rw_shared_count != 0) {
            return;
        }
        break;
    case LCK_RW_ASSERT_EXCLUSIVE:
        if ((lck->lck_rw_want_excl || lck->lck_rw_want_upgrade)
            && lck->lck_rw_shared_count == 0) {
            return;
        }
        break;
    case LCK_RW_ASSERT_HELD:
        if (lck->lck_rw_want_excl || lck->lck_rw_want_upgrade
            || lck->lck_rw_shared_count != 0) {
            return;
        }
        break;
    default:
        break;
    }

    panic("rw lock (%p) not held (mode=%u), first word %08x\n", lck, type,
          *(uint32_t *) lck);
}

void lck_rw_lock(lck_rw_t * lck, lck_rw_type_t lck_rw_type)
{
    if (lck_rw_type == LCK_RW_TYPE_SHARED)
        lck_rw_lock_shared(lck);
    else if (lck_rw_type == LCK_RW_TYPE_EXCLUSIVE)
        lck_rw_lock_exclusive(lck);
    else
        panic("lck_rw_lock(): Invalid RW lock type: %d\n", lck_rw_type);
    return;
}

void lck_spin_free(lck_spin_t * lck, lck_grp_t * grp)
{
    lck_spin_destroy(lck, grp);
    kfree((void *) lck, sizeof(lck_spin_t));
}

boolean_t lck_rw_try_lock(lck_rw_t * lck, lck_rw_type_t lck_rw_type)
{
    if (lck_rw_type == LCK_RW_TYPE_SHARED)
        return lck_rw_try_lock_shared(lck);
    else if (lck_rw_type == LCK_RW_TYPE_EXCLUSIVE)
        return lck_rw_try_lock_exclusive(lck);
    else
        panic("lck_rw_try_lock(): Invalid RW lock type: %d\n", lck_rw_type);
    return FALSE;
}

lck_spin_t *lck_spin_alloc_init(lck_grp_t * grp, lck_attr_t * attr)
{
    lck_spin_t *lck;

    if ((lck = (lck_spin_t *) kalloc(sizeof(lck_spin_t))) != 0)
        lck_spin_init(lck, grp, attr);

    return (lck);
}

void usimple_lock(usimple_lock_t l)
{
    lck_spin_lock((lck_spin_t *) l);
}

void usimple_unlock(usimple_lock_t l)
{
    lck_spin_unlock((lck_spin_t *) l);
}

unsigned int usimple_lock_try(usimple_lock_t l)
{
    return (lck_spin_try_lock((lck_spin_t *) l));
}

extern void arm_usimple_lock_init(usimple_lock_t, unsigned short);

void usimple_lock_init(usimple_lock_t l, unsigned short tag)
{
    arm_usimple_lock_init(l, tag);
}

void lck_rw_unlock_shared(lck_rw_t * lck)
{
    lck_rw_type_t ret;

    ret = lck_rw_done(lck);

    if (ret != LCK_RW_TYPE_SHARED)
        panic("lck_rw_unlock(): lock held in mode: %d\n", ret);
}

void lck_rw_unlock_exclusive(lck_rw_t * lck)
{
    lck_rw_type_t ret;

    ret = lck_rw_done(lck);

    if (ret != LCK_RW_TYPE_EXCLUSIVE)
        panic("lck_rw_unlock_exclusive(): lock held in mode: %d\n", ret);
}

void lck_rw_lock_shared_gen(lck_rw_t * lck)
{
    int i;
    wait_result_t res;

    lck_rw_ilk_lock(lck);

    while ((lck->lck_rw_want_excl || lck->lck_rw_want_upgrade)
           && ((lck->lck_rw_shared_count == 0) || (lck->lck_rw_priv_excl))) {
        i = lock_wait_time[1];

        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_CODE) |
                     DBG_FUNC_START, (int) lck, lck->lck_rw_want_excl,
                     lck->lck_rw_want_upgrade, i, 0);

        if (i != 0) {
            lck_rw_ilk_unlock(lck);
            while (--i != 0
                   && (lck->lck_rw_want_excl || lck->lck_rw_want_upgrade)
                   && ((lck->lck_rw_shared_count == 0)
                       || (lck->lck_rw_priv_excl)))
                continue;
            lck_rw_ilk_lock(lck);
        }

        if ((lck->lck_rw_want_excl || lck->lck_rw_want_upgrade)
            && ((lck->lck_rw_shared_count == 0) || (lck->lck_rw_priv_excl))) {
            lck->lck_rw_waiting = TRUE;
            res =
                assert_wait((event_t)
                            (((unsigned int *) lck) +
                             ((sizeof(lck_rw_t) - 1) / sizeof(unsigned int))),
                            THREAD_UNINT);
            if (res == THREAD_WAITING) {
                lck_rw_ilk_unlock(lck);
                res = thread_block(THREAD_CONTINUE_NULL);
                lck_rw_ilk_lock(lck);
            }
        }
        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_CODE) |
                     DBG_FUNC_END, (int) lck, lck->lck_rw_want_excl,
                     lck->lck_rw_want_upgrade, res, 0);
    }

    lck->lck_rw_shared_count++;

    lck_rw_ilk_unlock(lck);
}

void lck_rw_lock_exclusive_gen(lck_rw_t * lck)
{
    int i;
    wait_result_t res;

    lck_rw_ilk_lock(lck);
    /*
     *  Try to acquire the lck_rw_want_excl bit.
     */

    while (lck->lck_rw_want_excl) {
        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EXCLUSIVE_CODE) |
                     DBG_FUNC_START, (int) lck, 0, 0, 0, 0);

        i = lock_wait_time[1];
        if (i != 0) {
            lck_rw_ilk_unlock(lck);
            while (--i != 0 && lck->lck_rw_want_excl)
                continue;
            lck_rw_ilk_lock(lck);
        }

        if (lck->lck_rw_want_excl) {
            lck->lck_rw_waiting = TRUE;
            res =
                assert_wait((event_t)
                            (((unsigned int *) lck) +
                             ((sizeof(lck_rw_t) - 1) / sizeof(unsigned int))),
                            THREAD_UNINT);
            if (res == THREAD_WAITING) {
                lck_rw_ilk_unlock(lck);
                res = thread_block(THREAD_CONTINUE_NULL);
                lck_rw_ilk_lock(lck);
            }
        }
        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EXCLUSIVE_CODE) |
                     DBG_FUNC_END, (int) lck, res, 0, 0, 0);
    }
    lck->lck_rw_want_excl = TRUE;

    /*
     * Wait for readers (and upgrades) to finish 
     */

    while ((lck->lck_rw_shared_count != 0) || lck->lck_rw_want_upgrade) {

        i = lock_wait_time[1];

        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EXCLUSIVE1_CODE) |
                     DBG_FUNC_START, (int) lck, lck->lck_rw_shared_count,
                     lck->lck_rw_want_upgrade, i, 0);

        if (i != 0) {
            lck_rw_ilk_unlock(lck);
            while (--i != 0
                   && (lck->lck_rw_shared_count != 0
                       || lck->lck_rw_want_upgrade))
                continue;
            lck_rw_ilk_lock(lck);
        }

        if (lck->lck_rw_shared_count != 0 || lck->lck_rw_want_upgrade) {
            lck->lck_rw_waiting = TRUE;
            res =
                assert_wait((event_t)
                            (((unsigned int *) lck) +
                             ((sizeof(lck_rw_t) - 1) / sizeof(unsigned int))),
                            THREAD_UNINT);

            if (res == THREAD_WAITING) {
                lck_rw_ilk_unlock(lck);
                res = thread_block(THREAD_CONTINUE_NULL);
                lck_rw_ilk_lock(lck);
            }
        }
        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EXCLUSIVE1_CODE) |
                     DBG_FUNC_END, (int) lck, lck->lck_rw_shared_count,
                     lck->lck_rw_want_upgrade, res, 0);
    }

    lck_rw_ilk_unlock(lck);
}

void lck_mtx_lock_mark_destroyed(lck_mtx_t * mutex)
{
    return;
}

boolean_t lck_rw_lock_shared_to_exclusive_gen(lck_rw_t * lck)
{
    int i;
    boolean_t do_wakeup = FALSE;
    wait_result_t res;

    lck_rw_ilk_lock(lck);

    lck->lck_rw_shared_count--;

    if (lck->lck_rw_want_upgrade) {
        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_CODE) |
                     DBG_FUNC_START, (int) lck, lck->lck_rw_shared_count,
                     lck->lck_rw_want_upgrade, 0, 0);

        /*
         *  Someone else has requested upgrade.
         *  Since we've released a read lock, wake
         *  him up.
         */
        if (lck->lck_rw_waiting && (lck->lck_rw_shared_count == 0)) {
            lck->lck_rw_waiting = FALSE;
            do_wakeup = TRUE;
        }

        lck_rw_ilk_unlock(lck);

        if (do_wakeup)
            thread_wakeup((event_t)
                          (((unsigned int *) lck) +
                           ((sizeof(lck_rw_t) - 1) / sizeof(unsigned int))));

        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_CODE) |
                     DBG_FUNC_END, (int) lck, lck->lck_rw_shared_count,
                     lck->lck_rw_want_upgrade, 0, 0);

        return (FALSE);
    }

    lck->lck_rw_want_upgrade = TRUE;

    while (lck->lck_rw_shared_count != 0) {
        i = lock_wait_time[1];

        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX1_CODE) |
                     DBG_FUNC_START, (int) lck, lck->lck_rw_shared_count, i, 0,
                     0);
        if (i != 0) {
            lck_rw_ilk_unlock(lck);
            while (--i != 0 && lck->lck_rw_shared_count != 0)
                continue;
            lck_rw_ilk_lock(lck);
        }

        if (lck->lck_rw_shared_count != 0) {
            lck->lck_rw_waiting = TRUE;
            res =
                assert_wait((event_t)
                            (((unsigned int *) lck) +
                             ((sizeof(lck_rw_t) - 1) / sizeof(unsigned int))),
                            THREAD_UNINT);
            if (res == THREAD_WAITING) {
                lck_rw_ilk_unlock(lck);
                res = thread_block(THREAD_CONTINUE_NULL);
                lck_rw_ilk_lock(lck);
            }
        }
        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX1_CODE) |
                     DBG_FUNC_END, (int) lck, lck->lck_rw_shared_count, 0, 0,
                     0);
    }

    lck_rw_ilk_unlock(lck);

    return (TRUE);
}
