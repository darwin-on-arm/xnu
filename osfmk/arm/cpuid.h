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

#ifndef _ARM_CPUID_H_
#define _ARM_CPUID_H_

#include <arm/cpu_affinity.h>

typedef struct __arm_cache_level_t {
    int linesize;
    int ways;
    int level;
    int size;
} arm_cache_level_t;

typedef enum
{
    kProcessorFeatureARM_ISA = 0x1,
    kProcessorFeatureThumb = 0x2,
    kProcessorFeatureThumb2 = 0x4,
    kProcessorFeatureJazelle = 0x8,
    kProcessorFeatureThumbEE = 0x10,
    kProcessorFeatureARMv4 = 0x20,
    kProcessorFeatureSecurity = 0x40,
    kProcessorFeatureMicrocontroller = 0x80,
} arm_processor_features_t;

typedef struct __arm_processor_id_t {
    arm_cache_level_t cache_levels[MAX_CACHE_DEPTH]; 	/* 
    													 * Hopefully, we won't need more cache levels, 
     													 * modern ARM processors have L1 and L2 cache.
     													 */
    arm_processor_features_t processor_features;    	/* Supported processor features. */
    char* processor_class;
    uint32_t processor_midr;
} arm_processor_id_t;

extern arm_processor_id_t arm_processor_id;

#endif /* _ARM_CPUID_H_ */