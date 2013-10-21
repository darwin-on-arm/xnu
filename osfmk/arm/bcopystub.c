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
 * Bad copyin/copyout because I am a lazy bastard.
 */

#include <mach/mach_types.h>
#include <mach_assert.h>
#include <sys/errno.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_fault.h>
#include <sys/kdebug.h>

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
