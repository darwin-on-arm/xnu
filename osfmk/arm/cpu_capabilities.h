/*
 * ARM CPU Capabilities
 */

#ifndef _ARM_CPU_CAPABILITIES_H_
#define _ARM_CPU_CAPABILITIES_H_

#ifndef	__ASSEMBLER__
#include <stdint.h>
#endif /* __ASSEMBLER__ */

#define _COMM_PAGE32_AREA_LENGTH    ( 1 * 4096 )    /* reserved length of entire comm area */
#define _COMM_PAGE32_BASE_ADDRESS    ( 0x40000000 ) /* base address of allocated memory */
#define _COMM_PAGE32_START_ADDRESS    ( _COMM_PAGE32_BASE_ADDRESS ) /* address traditional commpage code starts on */
#define _COMM_PAGE32_AREA_USED        ( 1 * 4096 )  /* this is the amt actually allocated */
#define _COMM_PAGE32_SIGS_OFFSET        0x8000  /* offset to routine signatures */

#define _COMM_PAGE64_AREA_LENGTH    ( 1 * 4096 )
#ifdef __ASSEMBLER__
#define _COMM_PAGE64_BASE_ADDRESS    ( _COMM_PAGE32_BASE_ADDRESS )  /* base address of allocated memory */
#else                           /* __ASSEMBLER__ */
#define _COMM_PAGE64_BASE_ADDRESS    ( _COMM_PAGE32_BASE_ADDRESS )  /* base address of allocated memory */
#endif                          /* __ASSEMBLER__ */
#define _COMM_PAGE64_START_ADDRESS    ( _COMM_PAGE32_BASE_ADDRESS ) /* address traditional commpage code starts on */
#define _COMM_PAGE64_AREA_USED        ( 1 * 4096 )  /* this is the amt actually populated */

#define _COMM_PAGE32_OBJC_SIZE        0ULL
#define _COMM_PAGE32_OBJC_BASE        0ULL
#define _COMM_PAGE64_OBJC_SIZE        0ULL
#define _COMM_PAGE64_OBJC_BASE        0ULL

#define _COMM_PAGE_TEXT_START       (_COMM_PAGE_START_ADDRESS+0x1000)
#define _COMM_PAGE32_TEXT_START     (_COMM_PAGE32_BASE_ADDRESS+0x1000)  /* start of text section */
#define _COMM_PAGE64_TEXT_START     (_COMM_PAGE64_BASE_ADDRESS+0x1000)
#define _COMM_PAGE_TEXT_AREA_USED   ( 1 * 4096 )
#define _COMM_PAGE_TEXT_AREA_LENGTH ( 1 * 4096 )

#define _COMM_PAGE_AREA_LENGTH      _COMM_PAGE32_AREA_LENGTH
#define _COMM_PAGE_BASE_ADDRESS     _COMM_PAGE32_BASE_ADDRESS
#define _COMM_PAGE_START_ADDRESS    _COMM_PAGE32_START_ADDRESS
#define _COMM_PAGE_AREA_USED        _COMM_PAGE32_AREA_USED
#define _COMM_PAGE_SIGS_OFFSET      _COMM_PAGE32_SIGS_OFFSET

/* 
 * COMMPAGE Definitions
 */
#define _COMMPAGE_CPUFAMILY 			0x40000080		/* Set to CPUFAMILY_ARM_13 for CortexA8 */
#define _COMMPAGE_TIMEBASE_INFO         0x40000040
#define _COMMPAGE_NUMBER_OF_CPUS		0x40000034
#define _COMMPAGE_CPU_CAPABILITIES		0x40000020		
#define _COMMPAGE_MYSTERY_VALUE			0x4000001E		/* This is a uint16_t with value of 3. */

/*
 * CPU definitions.
 */
#define kUP 		0x8000
#define kHasEvent	0x1000

#ifndef	__ASSEMBLER__
/*
 * Guessed from http://opensource.apple.com/source/Libc/Libc-825.24/arm/sys/arm_commpage_gettimeofday.c.
 *
 * Thanks Apple! <3
 */
typedef struct __commpage_timeofday_data_t {
    uint64_t TimeBase;
    uint32_t TimeStamp_usec;
    uint32_t TimeStamp_sec;
    uint32_t TimeBaseTicks_per_sec;
    uint64_t TimeBase_magic;
    uint32_t TimeBase_add;
    uint32_t TimeBase_shift; 
} commpage_timeofday_data_t;
#endif

#endif
