/*
 * ARM common page
 */

#ifndef _ARM_COMMPAGE_COMMPAGE_H_
#define _ARM_COMMPAGE_COMMPAGE_H_

#ifndef	__ASSEMBLER__
#include <stdint.h>
#endif /* __ASSEMBLER__ */

#ifndef __ASSEMBLER__

extern	void	commpage_set_timestamp(uint64_t tbr, uint64_t secs, uint32_t ticks_per_sec);

#define	commpage_disable_timestamp() commpage_set_timestamp( 0, 0, 0 )
#define commpage_set_memory_pressure( pressure )

#endif /* assembler */

// rms, 12:34AM, i should go to sleep

#endif