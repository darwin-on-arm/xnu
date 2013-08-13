#ifndef	_BSD_ARM_VMPARAM_H_
#define	_BSD_ARM_VMPARAM_H_ 1

#include <sys/resource.h>

#define	USRSTACK	(0x2fe00000)

/*
 * Virtual memory related constants, all in bytes
 */
#ifndef DFLDSIZ
#define	DFLDSIZ		(RLIM_INFINITY)		/* initial data size limit */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		(RLIM_INFINITY)		/* max data size */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(1024*1024 - 4*1024)	/* initial stack size limit */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(1024*1024)		/* max stack size */
#endif
#ifndef	DFLCSIZ
#define DFLCSIZ		(0)			/* initial core size limit */
#endif
#ifndef	MAXCSIZ
#define MAXCSIZ		(RLIM_INFINITY)		/* max core size */
#endif	/* MAXCSIZ */

#endif	/* _BSD_ARM_VMPARAM_H_ */

