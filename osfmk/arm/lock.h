#ifndef _LOCK_H_
#define _LOCK_H_

#ifdef MACH_KERNEL_PRIVATE
#include <arm/locks.h>
typedef lck_rw_t lock_t;
#endif

#if defined(MACH_KERNEL_PRIVATE) && defined(__APPLE_API_PRIVATE)
#include <mach_ldebug.h>

#if	MACH_LDEBUG
#define	USLOCK_DEBUG 1
#else
#define	USLOCK_DEBUG 0
#endif                          /* USLOCK_DEBUG */

typedef struct uslock_debug {
    void *lock_pc;              /* pc where lock operation began    */
    void *lock_thread;          /* thread that acquired lock */
    void *unlock_thread;        /* last thread to release lock */
    void *unlock_pc;            /* pc where lock operation ended    */
    unsigned long duration[2];
    unsigned short state;
    unsigned char lock_cpu;
    unsigned char unlock_cpu;
} uslock_debug;

typedef struct slock {
    hw_lock_data_t interlock;   /* must be first... see lock.c */
#if	USLOCK_DEBUG
    unsigned short lock_type;   /* must be second... see lock.c */
#define USLOCK_TAG	0x5353
    uslock_debug debug;
#endif
} usimple_lock_data_t, *usimple_lock_t;

#else

typedef struct slock {
    unsigned long lock_data[10];
} usimple_lock_data_t, *usimple_lock_t;

#endif                          /* defined(MACH_KERNEL_PRIVATE) && defined(__APPLE_API_PRIVATE) */

#define	USIMPLE_LOCK_NULL	((usimple_lock_t) 0)

#if !defined(decl_simple_lock_data)
typedef usimple_lock_data_t *simple_lock_t;
typedef usimple_lock_data_t simple_lock_data_t;

#define	decl_simple_lock_data(class,name) \
	class	simple_lock_data_t	name;

#endif                          /* !defined(decl_simple_lock_data) */

extern unsigned int LockTimeOutTSC;
extern unsigned int LockTimeOut;

#endif
