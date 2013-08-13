/*
 * ARM CPU Capabilities
 */

#ifndef _ARM_CPU_CAPABILITIES_H_
#define _ARM_CPU_CAPABILITIES_H_

#define _COMM_PAGE32_AREA_LENGTH    ( 1 * 4096 )                        /* reserved length of entire comm area */
#define _COMM_PAGE32_BASE_ADDRESS    ( 0x40000000 )                     /* base address of allocated memory */
#define _COMM_PAGE32_START_ADDRESS    ( _COMM_PAGE32_BASE_ADDRESS )     /* address traditional commpage code starts on */
#define _COMM_PAGE32_AREA_USED        ( 1 * 4096 )                      /* this is the amt actually allocated */
#define _COMM_PAGE32_SIGS_OFFSET        0x8000                          /* offset to routine signatures */


#define _COMM_PAGE64_AREA_LENGTH    ( 1 * 4096 )        
#ifdef __ASSEMBLER__
#define _COMM_PAGE64_BASE_ADDRESS    ( _COMM_PAGE32_BASE_ADDRESS )      /* base address of allocated memory */
#else /* __ASSEMBLER__ */
#define _COMM_PAGE64_BASE_ADDRESS    ( _COMM_PAGE32_BASE_ADDRESS )      /* base address of allocated memory */
#endif /* __ASSEMBLER__ */
#define _COMM_PAGE64_START_ADDRESS    ( _COMM_PAGE32_BASE_ADDRESS )     /* address traditional commpage code starts on */
#define _COMM_PAGE64_AREA_USED        ( 1 * 4096 )                      /* this is the amt actually populated */

#define _COMM_PAGE32_OBJC_SIZE        0ULL
#define _COMM_PAGE32_OBJC_BASE        0ULL
#define _COMM_PAGE64_OBJC_SIZE        0ULL
#define _COMM_PAGE64_OBJC_BASE        0ULL

#define _COMM_PAGE_TEXT_START       (_COMM_PAGE_START_ADDRESS+0x1000)
#define _COMM_PAGE32_TEXT_START     (_COMM_PAGE32_BASE_ADDRESS+0x1000)      /* start of text section */
#define _COMM_PAGE64_TEXT_START     (_COMM_PAGE64_BASE_ADDRESS+0x1000)
#define _COMM_PAGE_TEXT_AREA_USED   ( 1 * 4096 )
#define _COMM_PAGE_TEXT_AREA_LENGTH ( 1 * 4096 )

#define _COMM_PAGE_AREA_LENGTH      _COMM_PAGE32_AREA_LENGTH
#define _COMM_PAGE_BASE_ADDRESS     _COMM_PAGE32_BASE_ADDRESS
#define _COMM_PAGE_START_ADDRESS    _COMM_PAGE32_START_ADDRESS
#define _COMM_PAGE_AREA_USED        _COMM_PAGE32_AREA_USED
#define _COMM_PAGE_SIGS_OFFSET      _COMM_PAGE32_SIGS_OFFSET

#endif
