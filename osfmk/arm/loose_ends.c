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
 * Loose ends for ARM.
 */

#include <kern/kern_types.h>
#include <string.h>

#define INT_SIZE	(BYTE_SIZE * sizeof (int))

/**
 * setbit
 *
 * Set indicated bit in bit string.
 */
void setbit(int bitno, int *s)
{
	s[bitno / INT_SIZE] |= 1 << (bitno % INT_SIZE);
}

/**
 * clrbit
 *
 * Clear indicated bit in bit string.
 */
void clrbit(int bitno, int *s)
{
	s[bitno / INT_SIZE] &= ~(1 << (bitno % INT_SIZE));
}

/**
 * testbit
 *
 * Test if indicated bit is set in bit string.
 */
int testbit(int bitno, int *s)
{
	return s[bitno / INT_SIZE] & (1 << (bitno % INT_SIZE));
}

/*
 * ffsbit
 *
 * Find first bit set in bit string.
 */
int ffsbit(int *s)
{
	int             offset;

	for (offset = 0; !*s; offset += (int)INT_SIZE, ++s);
	return offset + __builtin_ctz(*s);
}

/**
 * early_random
 *
 * Return a "uniformly random" random number.
 */
uint64_t early_random(void)
{
    return 4;
}

/**
 * ffs
 *
 * Find first bit set.
 */
int ffs(unsigned int mask)
{
	if (mask == 0)
		return 0;

	/*
	 * NOTE: cannot use __builtin_ffs because it generates a call to
	 * 'ffs'
	 */
	return 1 + __builtin_ctz(mask);
}

/**
 * strlen
 *
 * Computes the length of a string.
 */
size_t strlen(register const char *string)
{
	register const char *ret = string;
    
	while (*string++ != '\0')
		continue;
	return string - 1 - ret;
}

/**
 * fillPage
 *
 * Fill a page with junk. I am very sad and bad.
 */
void fillPage(ppnum_t pa, unsigned int fill)
{
    return;
}


