/*
 * ARM CPU affinity.
 */

#ifndef _ARM_CPU_AFFINITY_H_
#define _ARM_CPU_AFFINITY_H_

#define MAX_CACHE_DEPTH        3 /* maximum cache depth for this architecture */

/* these are found in machine_routines.c */
extern int ml_get_max_affinity_sets(void);
extern processor_set_t  ml_affinity_to_pset(uint32_t affinity_num);

#endif
