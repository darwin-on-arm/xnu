/*
 * ARM commpage support.
 */

#include <mach/mach_types.h>
#include <mach/machine.h>
#include <mach/vm_map.h>

#include <machine/commpage.h>
#include <machine/pmap.h>

#include <kern/processor.h>

#include <vm/vm_map.h>
#include <vm/vm_kern.h>

void
commpage_update_active_cpus(void)
{
    // Please fix when commpage header is correct.
    return;
}

void
commpage_populate(void)
{
    kprintf("commpage_populate()\n");
    pmap_create_sharedpage();
    return;
}


void
commpage_set_timestamp(uint64_t tbr, uint64_t secs, uint32_t ticks_per_sec)
{
    ;
}