/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
#include <mach/mach_types.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <libsa/types.h>

#include <kdp/kdp_core.h>
#include <kdp/kdp_internal.h>
#include <mach-o/loader.h>
#include <mach/thread_status.h>
#include <arm/thread.h>

#include <mach/machine/vm_types.h>
#include <arm/pmap.h>

boolean_t kdp_trans_off;

int kdp_dump_trap(int type, arm_saved_state_t * regs);

static const arm_state_hdr_t thread_flavor_array[] = {
    {ARM_THREAD_STATE, ARM_THREAD_STATE_COUNT}
};

size_t kern_collectth_state_size(void)
{
    unsigned int i;
    size_t tstate_size = 0;

    for (i = 0; i < sizeof(thread_flavor_array) / sizeof(thread_flavor_array[0]); i++)
        tstate_size += sizeof(arm_state_hdr_t) +
            (thread_flavor_array[i].count * sizeof(int));

    return tstate_size;
}

void kern_collectth_state(thread_t thread, void *buffer, size_t size)
{
    size_t hoffset;
    unsigned int i;
    struct thread_command *tc;

    /*
     *  Fill in thread command structure.
     */
    hoffset = 0;

    if (hoffset + sizeof(struct thread_command) > size)
        return;

    tc = (struct thread_command *) ((uintptr_t) buffer + hoffset);
    tc->cmd = LC_THREAD;
    tc->cmdsize = sizeof(struct thread_command) + kern_collectth_state_size();
    hoffset += sizeof(struct thread_command);
    /*
     * Follow with a struct thread_state_flavor and
     * the appropriate thread state struct for each
     * thread state flavor.
     */
    for (i = 0; i < sizeof(thread_flavor_array) / sizeof(thread_flavor_array[0]); i++) {

        if (hoffset + sizeof(arm_state_hdr_t) > size)
            return;

        *(arm_state_hdr_t *) ((uintptr_t) buffer + hoffset) = thread_flavor_array[i];
        hoffset += sizeof(arm_state_hdr_t);

        if (hoffset + thread_flavor_array[i].count * sizeof(int) > size)
            return;

        /*
         * Locate and obtain the non-volatile register context
         * * for this kernel thread. This should ideally be
         * * encapsulated in machine_thread_get_kern_state()
         * * but that routine appears to have been co-opted
         * * by CHUD to obtain pre-interrupt state.
         */
        if (thread_flavor_array[i].flavor == ARM_THREAD_STATE) {
            arm_thread_state_t *tstate =
                (arm_thread_state_t *) ((uintptr_t) buffer + hoffset);
            vm_offset_t kstack;

            bzero(tstate, ARM_THREAD_STATE_COUNT * sizeof(int));
            // broken
        } else {
            void *tstate = (void *) ((uintptr_t) buffer + hoffset);

            bzero(tstate, thread_flavor_array[i].count * sizeof(int));
        }

        hoffset += thread_flavor_array[i].count * sizeof(int);
    }
}

/*
 *
 */
static addr64_t kdp_vtophys(pmap_t pmap, addr64_t va)
{
    addr64_t pa;
    ppnum_t pp;

    pp = pmap_find_phys_fvtp(pmap, va); /* Get the page number */
    if (!pp)
        return 0;               /* Just return if no translation */

    pa = ((addr64_t) pp << 12) | (va & 0x0000000000000FFFULL);  /* Shove in the page offset */
    return (pa);
}

/* Intended to be called from the kernel trap handler if an unrecoverable fault
 * occurs during a crashdump (which shouldn't happen since we validate mappings
 * and so on). This should be reworked to attempt some form of recovery.
 */
int kdp_dump_trap(int type, __unused arm_saved_state_t * saved_state)
{
    printf("An unexpected trap (type %d) occurred during the system dump, terminating.\n",
           type);
    kdp_send_crashdump_pkt(KDP_EOF, NULL, 0, ((void *) 0));
    abort_panic_transfer();
    kdp_flag &= ~KDP_PANIC_DUMP_ENABLED;
    kdp_flag &= ~PANIC_CORE_ON_NMI;
    kdp_flag &= ~PANIC_LOG_DUMP;

    kdp_reset();

    kdp_raise_exception(EXC_BAD_ACCESS, 0, 0, kdp.saved_state);
    return (0);
}

mach_vm_size_t kdp_machine_vm_read(mach_vm_address_t src, caddr_t dst, mach_vm_size_t len)
{
    addr64_t cur_virt_src, cur_virt_dst;
    addr64_t cur_phys_src, cur_phys_dst;
    unsigned resid, cnt;
    unsigned int dummy;
    pmap_t pmap;

#if 0
    kprintf("kdp_machine_vm_read1: src %llx dst %llx len %x - %08X %08X\n", src, dst, len,
            ((unsigned long *) src)[0], ((unsigned long *) src)[1]);
#endif

    cur_virt_src = (addr64_t) src;
    cur_virt_dst = (addr64_t) (intptr_t) dst;

    if (kdp_trans_off) {
        resid = len;            /* Get the length to copy */

        while (resid != 0) {

            if ((cur_phys_dst = kdp_vtophys(kernel_pmap, cur_virt_dst)) == 0)
                goto exit;

            cnt = 4096 - (cur_virt_src & 0xFFF);    /* Get length left on page */
            if (cnt > (4096 - (cur_virt_dst & 0xFFF)))
                cnt = 4096 - (cur_virt_dst & 0xFFF);

            if (cnt > resid)
                cnt = resid;

            bcopy_phys(cur_virt_src, cur_phys_dst, cnt);    /* Copy stuff over */

            cur_virt_src += cnt;
            cur_virt_dst += cnt;
            resid -= cnt;
        }

    } else {

        resid = len;

        while (resid != 0) {
            /*
             * Always translate the destination using the kernel_pmap. 
             */
            if ((cur_phys_dst = kdp_vtophys(kernel_pmap, cur_virt_dst)) == 0)
                goto exit;

            if ((cur_phys_src = kdp_vtophys(kernel_pmap, cur_virt_src)) == 0)
                goto exit;

            cnt = 4096 - (cur_virt_src & 0xFFF);    /* Get length left on page */
            if (cnt > (4096 - (cur_virt_dst & 0xFFF)))
                cnt = 4096 - (cur_virt_dst & 0xFFF);

            if (cnt > resid)
                cnt = resid;

#if 0
            kprintf("kdp_machine_vm_read2: pmap %08X, virt %016LLX, phys %016LLX\n",
                    pmap, cur_virt_src, cur_phys_src);
#endif

            bcopy_phys(cur_phys_src, cur_phys_dst, cnt);    /* Copy stuff over */

            cur_virt_src += cnt;
            cur_virt_dst += cnt;
            resid -= cnt;
        }
    }
 exit:
#if 0
    kprintf("kdp_machine_vm_read: ret %08X\n", len - resid);
#endif
    return (len - resid);
}

mach_vm_size_t
kdp_machine_phys_read(kdp_readphysmem64_req_t * rq __unused, caddr_t dst __unused,
                      uint16_t lcpu __unused)
{
    return 0;                   /* unimplemented */
}

mach_vm_size_t
kdp_machine_phys_write(kdp_writephysmem64_req_t * rq __unused, caddr_t src __unused,
                       uint16_t lcpu __unused)
{
    return 0;                   /* unimplemented */
}

mach_vm_size_t
kdp_machine_vm_write(caddr_t src, mach_vm_address_t dst, mach_vm_size_t len)
{
    addr64_t cur_virt_src, cur_virt_dst;
    addr64_t cur_phys_src, cur_phys_dst;
    unsigned resid, cnt;
    unsigned int dummy;
    pmap_t pmap;

    cur_virt_src = (addr64_t) src;
    cur_virt_dst = (addr64_t) (intptr_t) dst;

    if (kdp_trans_off) {
        return 0;
    } else {
        resid = len;
        while (resid != 0) {
            /*
             * Always translate the destination using the kernel_pmap. 
             */
            if ((cur_phys_dst = kdp_vtophys(kernel_pmap, cur_virt_dst)) == 0)
                goto exit;

            if ((cur_phys_src = kdp_vtophys(kernel_pmap, cur_virt_src)) == 0)
                goto exit;

            cnt = 4096 - (cur_virt_src & 0xFFF);    /* Get length left on page */
            if (cnt > (4096 - (cur_virt_dst & 0xFFF)))
                cnt = 4096 - (cur_virt_dst & 0xFFF);

            if (cnt > resid)
                cnt = resid;

            bcopy_phys(cur_phys_src, cur_phys_dst, cnt);    /* Copy stuff over */

            cur_virt_src += cnt;
            cur_virt_dst += cnt;
            resid -= cnt;
        }
    }
 exit:
#if 0
    kprintf("kdp_machine_vm_read: ret %08X\n", len - resid);
#endif
    return (len - resid);
}
