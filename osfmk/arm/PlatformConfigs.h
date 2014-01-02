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

#ifndef _PLATFORMCONFIGS_H_
#define _PLATFORMCONFIGS_H_

/* ARMv7 Cortex-A9 platforms. */

#ifdef BOARD_CONFIG_MSM8960_TOUCHPAD
#define __ARM_PROCESSOR_CLASS_CORTEX_A9__		1
#define __ARM_PROCESSOR_CLASS_QUALCOMM_A9__		1
#endif

/* ARMv7 Cortex-A8 platforms. */

#ifdef BOARD_CONFIG_S5L8920X
#define __ARM_PROCESSOR_CLASS_CORTEX_A8__		1
#endif

#ifdef BOARD_CONFIG_S5L8922X
#define __ARM_PROCESSOR_CLASS_CORTEX_A8__		1
#endif

#ifdef BOARD_CONFIG_S5L8930X
#define __ARM_PROCESSOR_CLASS_CORTEX_A8__		1
#endif

#ifdef BOARD_CONFIG_OMAP3430_RX51
#define __ARM_PROCESSOR_CLASS_CORTEX_A8__		1
#endif

#ifdef BOARD_CONFIG_OMAP335X
#define __ARM_PROCESSOR_CLASS_CORTEX_A8__		1
#endif

#ifdef BOARD_CONFIG_OMAP3530
#define __ARM_PROCESSOR_CLASS_CORTEX_A8__		1
#endif

#ifdef BOARD_CONFIG_ARMPBA8
#define __ARM_PROCESSOR_CLASS_CORTEX_A8__		1
#endif

/* ARMv6 ARM11JZF-S platforms. */
#ifdef BOARD_CONFIG_S5L8900XRB
#define __ARM_PROCESSOR_CLASS_11JZFS__			1
#endif

#ifdef BOARD_CONFIG_RASPBERRYPI
#define __ARM_PROCESSOR_CLASS_11JZFS__			1
#endif

#ifdef BOARD_CONFIG_S5L8720X
#define __ARM_PROCESSOR_CLASS_11JZFS__			1
#endif

#endif /* _PLATFORMCONFIGS_H_ */