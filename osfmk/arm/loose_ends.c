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
/*      $OpenBSD: arc4random.c,v 1.19 2008/06/04 00:50:23 djm Exp $        */

/*
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Arc4 random number generator for OpenBSD.
 *
 * This code is derived from section 17.1 of Applied Cryptography,
 * second edition, which describes a stream cipher allegedly
 * compatible with RSA Labs "RC4" cipher (the actual description of
 * which is a trade secret).  The same algorithm is used as a stream
 * cipher called "arcfour" in Tatu Ylonen's ssh package.
 *
 * Here the stream cipher has been modified always to include the time
 * when initializing the state.  That makes it impossible to
 * regenerate the same random sequence twice, so this can't be used
 * for encryption, but will generate good random numbers.
 *
 * RC4 is a registered trademark of RSA Laboratories.
 */

/*
 * Loose ends for ARM.
 */

#include <kern/kern_types.h>
#include <string.h>
#include <arm/pmap.h>

#define INT_SIZE	(BYTE_SIZE * sizeof (int))

#ifdef __GNUC__
#define inline __inline
#else                           /* !__GNUC__ */
#define inline
#endif                          /* !__GNUC__ */

struct arc4_stream {
    u_int8_t i;
    u_int8_t j;
    u_int8_t s[256];
};

static int rs_initialized;
static struct arc4_stream rs;
static int arc4_count;

static inline u_int8_t arc4_getbyte(void);

static inline void arc4_init(void)
{
    int n;

    for (n = 0; n < 256; n++)
        rs.s[n] = n;
    rs.i = 0;
    rs.j = 0;
}

static inline void arc4_addrandom(u_char * dat, int datlen)
{
    int n;
    u_int8_t si;

    rs.i--;
    for (n = 0; n < 256; n++) {
        rs.i = (rs.i + 1);
        si = rs.s[rs.i];
        rs.j = (rs.j + si + dat[n % datlen]);
        rs.s[rs.i] = rs.s[rs.j];
        rs.s[rs.j] = si;
    }
    rs.j = rs.i;
}

static void arc4_stir(void)
{
    int i, fd;
    union {
        uint32_t tsc;
        u_int rnd[128 / sizeof(u_int)];
    } rdat;
    int n;

    if (!rs_initialized) {
        arc4_init();
        rs_initialized = 1;
    }

    /*
     * Get current performance monitor ticks.
     */
    rdat.tsc = 0;
#if 0
    __asm__ __volatile__("mrc p15, 0, %0, c9, c13, 0":"=r"(rdat.tsc));
#endif
    arc4_addrandom((void *) &rdat, sizeof(rdat));

    /*
     * Discard early keystream, as per recommendations in:
     * http://www.wisdom.weizmann.ac.il/~itsik/RC4/Papers/Rc4_ksa.ps
     */
    for (i = 0; i < 256; i++)
        (void) arc4_getbyte();
    arc4_count = 1600000;
}

static inline u_int8_t arc4_getbyte(void)
{
    u_int8_t si, sj;

    rs.i = (rs.i + 1);
    si = rs.s[rs.i];
    rs.j = (rs.j + si);
    sj = rs.s[rs.j];
    rs.s[rs.i] = sj;
    rs.s[rs.j] = si;
    return (rs.s[(si + sj) & 0xff]);
}

u_int8_t __arc4_getbyte(void)
{
    u_int8_t val;

    if (--arc4_count == 0 || !rs_initialized)
        arc4_stir();
    val = arc4_getbyte();
    return val;
}

static inline u_int32_t arc4_getword(void)
{
    u_int32_t val;
    val = arc4_getbyte() << 24;
    val |= arc4_getbyte() << 16;
    val |= arc4_getbyte() << 8;
    val |= arc4_getbyte();
    return val;
}

void arc4random_stir(void)
{
    arc4_stir();
}

void arc4random_addrandom(u_char * dat, int datlen)
{
    if (!rs_initialized)
        arc4_stir();
    arc4_addrandom(dat, datlen);
}

u_int32_t arc4random(void)
{
    u_int32_t val;
    arc4_count -= 4;
    if (arc4_count <= 0 || !rs_initialized)
        arc4_stir();
    val = arc4_getword();
    return val;
}

void arc4random_buf(void *_buf, size_t n)
{
    u_char *buf = (u_char *) _buf;
    if (!rs_initialized)
        arc4_stir();
    while (n--) {
        if (--arc4_count <= 0)
            arc4_stir();
        buf[n] = arc4_getbyte();
    }
}

/*
 * Calculate a uniformly distributed random number less than upper_bound
 * avoiding "modulo bias".
 *
 * Uniformity is achieved by generating new random numbers until the one
 * returned is outside the range [0, 2**32 % upper_bound).  This
 * guarantees the selected random number will be inside
 * [2**32 % upper_bound, 2**32) which maps back to [0, upper_bound)
 * after reduction modulo upper_bound.
 */
u_int32_t arc4random_uniform(u_int32_t upper_bound)
{
    u_int32_t r, min;

    if (upper_bound < 2)
        return 0;

#if (ULONG_MAX > 0xffffffffUL)
    min = 0x100000000UL % upper_bound;
#else
    /*
     * Calculate (2**32 % upper_bound) avoiding 64-bit math 
     */
    if (upper_bound > 0x80000000)
        min = 1 + ~upper_bound; /* 2**32 - upper_bound */
    else {
        /*
         * (2**32 - (x * 2)) % x == 2**32 % x when x <= 2**31 
         */
        min = ((0xffffffff - (upper_bound * 2)) + 1) % upper_bound;
    }
#endif

    /*
     * This could theoretically loop forever but each retry has
     * p > 0.5 (worst case, usually far better) of selecting a
     * number inside the range we need, so it should rarely need
     * to re-roll.
     */
    for (;;) {
        r = arc4random();
        if (r >= min)
            break;
    }

    return r % upper_bound;
}

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
    int offset;

    for (offset = 0; !*s; offset += (int) INT_SIZE, ++s) ;
    return offset + __builtin_ctz(*s);
}

/**
 * early_random
 *
 * Return a "uniformly random" random number.
 */
uint64_t early_random(void)
{
    union {
        struct {
            uint32_t lo;
            uint32_t hi;
        } r;
        uint64_t uval;
    } random;

    random.r.lo = arc4random();
    random.r.hi = arc4random();
    return random.uval;
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
#if 0
    vm_offset_t v = phys_to_virt(pa << PAGE_SHIFT);
    uint32_t i;

    for (i = 0; i <= PAGE_SIZE; i += sizeof(uint32_t)) {
        *(uint32_t *) ((uintptr_t) v + i) = 0;
    }
#endif
    return;
}

/**
 * copypv
 *
 * Copy a memory address across a virtual/physical memory barrier.
 */
kern_return_t copypv(addr64_t src64, addr64_t snk64, unsigned int size, int which)
{
    addr64_t src64_virt, snk64_virt;
    int bothphys = 0;

    if ((which & (cppvPsrc | cppvPsnk)) == 0)   /* Make sure that only one is virtual */
        panic("copypv: no more than 1 parameter may be virtual\n"); /* Not allowed */

    if ((which & (cppvPsrc | cppvPsnk)) == (cppvPsrc | cppvPsnk))
        bothphys = 1;           /* both are physical */

    /*
     * Do a physical copy in if requested, else, move data out. 
     */
    if (which & cppvPsrc)
        src64 = phys_to_virt(src64);

    if (which & cppvPsnk)
        snk64 = phys_to_virt(snk64);

    /*
     * If it's a kernel copy, use ovbcopy. 
     */
    if (!(which & (cppvKmap | cppvPsrc)))
        copyout(src64, snk64, size);
    else if (!(which & (cppvKmap | cppvPsrc)))
        copyin(src64, snk64, size);
    else
        bcopy(src64, snk64, size);

    return KERN_SUCCESS;
}

void bzero_phys_nc(addr64_t src64, uint32_t bytes)
{
    bzero_phys(src64, bytes);
}
