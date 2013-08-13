/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
 */
/*
 * FILE_ID: thread_status.h
 */


#ifndef _ARM_THREAD_STATUS_H_
#define _ARM_THREAD_STATUS_H_

#include <mach/arm/_structs.h>
#include <mach/message.h>
#include <mach/arm/thread_state.h>

/*
 *    Support for determining the state of a thread
 */


/*
 * Flavors
 */

#define ARM_THREAD_STATE		1
#define ARM_VFP_STATE			2
#define ARM_EXCEPTION_STATE		3
#define ARM_DEBUG_STATE			4
#define THREAD_STATE_NONE		5


#define VALID_THREAD_STATE_FLAVOR(x)\
((x == ARM_THREAD_STATE) 		||	\
 (x == ARM_VFP_STATE) 			||	\
 (x == ARM_EXCEPTION_STATE) 	||	\
 (x == ARM_DEBUG_STATE) 		||	\
 (x == THREAD_STATE_NONE))

typedef _STRUCT_ARM_THREAD_STATE		arm_thread_state_t;
typedef _STRUCT_ARM_VFP_STATE			arm_vfp_state_t;
typedef _STRUCT_ARM_EXCEPTION_STATE		arm_exception_state_t;
typedef _STRUCT_ARM_DEBUG_STATE			arm_debug_state_t;

typedef arm_thread_state_t arm_saved_state_t;

#define ARM_THREAD_STATE_COUNT ((mach_msg_type_number_t) \
   (sizeof (arm_thread_state_t)/sizeof(uint32_t)))

#define ARM_VFP_STATE_COUNT ((mach_msg_type_number_t) \
   (sizeof (arm_vfp_state_t)/sizeof(uint32_t)))

#define ARM_EXCEPTION_STATE_COUNT ((mach_msg_type_number_t) \
   (sizeof (arm_exception_state_t)/sizeof(uint32_t)))

#define ARM_DEBUG_STATE_COUNT ((mach_msg_type_number_t) \
   (sizeof (arm_debug_state_t)/sizeof(uint32_t)))

#define MACHINE_THREAD_STATE ARM_THREAD_STATE
#define MACHINE_THREAD_STATE_COUNT  ARM_THREAD_STATE_COUNT

/*
 * Largest state on this machine:
 */
#define THREAD_MACHINE_STATE_MAX	THREAD_STATE_MAX


struct arm_state_hdr {
	int	flavor;
	int	count;
};
typedef struct arm_state_hdr arm_state_hdr_t;


#endif    /* _ARM_THREAD_STATUS_H_ */
