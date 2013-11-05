/*
 * Copyright (c) 2004-2007 Apple Inc. All rights reserved.
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

#ifndef	_ARM_LOCKS_H_
#define	_ARM_LOCKS_H_

#include <sys/appleapiopts.h>
#include <kern/kern_types.h>

#ifdef	MACH_KERNEL_PRIVATE

struct hslock {
    uintptr_t lock_data;
};
typedef struct hslock hw_lock_data_t, *hw_lock_t;

extern unsigned int LcksOpts;

#define enaLkDeb		0x00000001  /* Request debug in default attribute */
#define enaLkStat		0x00000002  /* Request statistic in default attribute */

#endif

#ifdef	MACH_KERNEL_PRIVATE
typedef struct {
    unsigned long interlock;
    unsigned long lck_spin_pad[9];  /* XXX - usimple_lock_data_t */
} lck_spin_t;

#define	LCK_SPIN_TAG_DESTROYED		0x00002007  /* lock marked as Destroyed */

#else
#ifdef	KERNEL_PRIVATE
typedef struct {
    unsigned long opaque[10];
} lck_spin_t;
#else
typedef struct __lck_spin_t__ lck_spin_t;
#endif
#endif

#ifdef	MACH_KERNEL_PRIVATE
typedef struct _lck_mtx_ {
    union {
        struct {
            unsigned int lck_mtxd_data;
            unsigned short lck_mtxd_waiters;
            unsigned short lck_mtxd_pri;
            unsigned int lck_mtxd_pad8;
        } lck_mtxd;
        struct {
            unsigned int lck_mtxi_tag;
            struct _lck_mtx_ext_ *lck_mtxi_ptr;
            unsigned int lck_mtxi_pad;
        } lck_mtxi;
    } lck_mtx_sw;
} lck_mtx_t;

#define lck_mtx_data    lck_mtx_sw.lck_mtxd.lck_mtxd_data
#define	lck_mtx_owner	lck_mtx_sw.lck_mtxd.lck_mtxd_owner
#define	lck_mtx_waiters	lck_mtx_sw.lck_mtxd.lck_mtxd_waiters
#define	lck_mtx_pri	lck_mtx_sw.lck_mtxd.lck_mtxd_pri
#define	lck_mtx_ilocked	lck_mtx_sw.lck_mtxd.lck_mtxd_ilocked
#define	lck_mtx_mlocked	lck_mtx_sw.lck_mtxd.lck_mtxd_mlocked
#define	lck_mtx_promoted lck_mtx_sw.lck_mtxd.lck_mtxd_promoted
#define	lck_mtx_spin	lck_mtx_sw.lck_mtxd.lck_mtxd_spin

#define lck_mtx_tag	lck_mtx_sw.lck_mtxi.lck_mtxi_tag
#define lck_mtx_ptr	lck_mtx_sw.lck_mtxi.lck_mtxi_ptr
#define lck_mtx_state	lck_mtx_sw.lck_mtxi.lck_mtxi_pad

#define	LCK_MTX_TAG_INDIRECT			0x00001007  /* lock marked as Indirect  */
#define	LCK_MTX_TAG_DESTROYED			0x00002007  /* lock marked as Destroyed */
#define LCK_MTX_PTR_EXTENDED			0x00003007  /* lock is extended version */

/* Adaptive spin before blocking */
extern unsigned int MutexSpin;

extern void lck_mtx_lock_mark_destroyed(lck_mtx_t * mutex);
extern int lck_mtx_lock_mark_promoted(lck_mtx_t * mutex);
extern int lck_mtx_lock_decr_waiter(lck_mtx_t * mutex);
extern int lck_mtx_lock_grab_mutex(lck_mtx_t * mutex);
extern integer_t lck_mtx_lock_get_pri(lck_mtx_t * mutex);

extern void hw_lock_byte_init(uint8_t * lock_byte);
extern void hw_lock_byte_lock(uint8_t * lock_byte);
extern void hw_lock_byte_unlock(uint8_t * lock_byte);

#define     lck_rw_lock_exclusive       lck_rw_lock_exclusive_gen

typedef struct {
    unsigned int type;
    vm_offset_t pc;
    vm_offset_t thread;
} lck_mtx_deb_t;

#define MUTEX_TAG       0x4d4d

typedef struct {
    unsigned int lck_mtx_stat_data;
} lck_mtx_stat_t;

typedef struct _lck_mtx_ext_ {
    lck_mtx_t lck_mtx;
    struct _lck_grp_ *lck_mtx_grp;
    unsigned int lck_mtx_attr;
    lck_mtx_deb_t lck_mtx_deb;
    uint64_t lck_mtx_stat;
} lck_mtx_ext_t;

#define	LCK_MTX_ATTR_DEBUG	0x1
#define	LCK_MTX_ATTR_DEBUGb	0
#define	LCK_MTX_ATTR_STAT	0x2
#define	LCK_MTX_ATTR_STATb	1

#else
#ifdef	KERNEL_PRIVATE
typedef struct {
    unsigned long opaque[3];
} lck_mtx_t;

typedef struct {
    unsigned long opaque[10];
} lck_mtx_ext_t;

#else
typedef struct __lck_mtx_t__ lck_mtx_t;
typedef struct __lck_mtx_ext_t__ lck_mtx_ext_t;
#endif
#endif

#ifdef	MACH_KERNEL_PRIVATE
#pragma pack(1)                 /* Make sure the structure stays as we defined it */
typedef struct _lck_rw_t_internal_ {
    union {
        struct {
            unsigned int lck_rwd_interlock:1, lck_rwd_waiting:1,
                lck_rwd_want_upgrade:1, lck_rwd_want_excl:1, lck_rwd_pad17:11,
                lck_rwd_priv_excl:1, lck_rwd_shared_cnt:16;
            unsigned int lck_rwd_pad4;
            unsigned int lck_rwd_pad8;
        } lck_rwd;
        struct {
            unsigned int lck_rwi_tag;
            struct _lck_rw_ext_ *lck_rwi_ptr;
            unsigned int lck_rwi_pad8;
        } lck_rwi;
    } lck_rw_sw;
} lck_rw_t;

#define	lck_rw_interlock		lck_rw_sw.lck_rwd.lck_rwd_interlock
#define	lck_rw_want_upgrade		lck_rw_sw.lck_rwd.lck_rwd_want_upgrade
#define	lck_rw_want_excl		lck_rw_sw.lck_rwd.lck_rwd_want_excl
#define	lck_rw_waiting			lck_rw_sw.lck_rwd.lck_rwd_waiting
#define	lck_rw_priv_excl		lck_rw_sw.lck_rwd.lck_rwd_priv_excl
#define	lck_rw_shared_count		lck_rw_sw.lck_rwd.lck_rwd_shared_cnt

#define lck_rw_tag				lck_rw_sw.lck_rwi.lck_rwi_tag
#define lck_rw_ptr				lck_rw_sw.lck_rwi.lck_rwi_ptr

#pragma pack()

#define	LCK_RW_ATTR_DEBUG	0x1
#define	LCK_RW_ATTR_DEBUGb	0
#define	LCK_RW_ATTR_STAT	0x2
#define	LCK_RW_ATTR_STATb	1
#define LCK_RW_ATTR_READ_PRI	0x3
#define LCK_RW_ATTR_READ_PRIb	2
#define	LCK_RW_ATTR_DIS_THREAD	0x40000000
#define	LCK_RW_ATTR_DIS_THREADb	30
#define	LCK_RW_ATTR_DIS_MYLOCK	0x10000000
#define	LCK_RW_ATTR_DIS_MYLOCKb	28

#define	LCK_RW_TAG_DESTROYED		0x00002007  /* lock marked as Destroyed */

#else
#ifdef	KERNEL_PRIVATE
#pragma pack(1)
typedef struct {
    uint32_t opaque[3];
} lck_rw_t;
#pragma pack()
#else
typedef struct __lck_rw_t__ lck_rw_t;
#endif
#endif

#endif                          /* _ARM_LOCKS_H_ */
