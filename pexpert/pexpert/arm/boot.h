#ifndef _PEXPERT_ARM_BOOT_H_
#define _PEXPERT_ARM_BOOT_H_

#define BOOT_LINE_LENGTH        256

/*
 * Video information.
 */

struct Boot_Video {
	unsigned long	v_baseAddr;
    unsigned long	v_display;
	unsigned long	v_rowBytes;
    unsigned long	v_width;	
	unsigned long	v_height;	
	unsigned long	v_depth;	
};

#define GRAPHICS_MODE         1
#define FB_TEXT_MODE          2

#define kBootVideoDepthMask         (0xFF)
#define kBootVideoDepthDepthShift	(0)
#define kBootVideoDepthRotateShift	(8)
#define kBootVideoDepthScaleShift	(16)

typedef struct Boot_Video	Boot_Video;

#define kBootArgsRevision		1
#define kBootArgsVersion1		1
#define kBootArgsVersion2		2

typedef struct boot_args {
	uint16_t		Revision;
	uint16_t		Version;
	uint32_t		virtBase;
	uint32_t		physBase;
	uint32_t		memSize;
	uint32_t		topOfKernelData;
	Boot_Video		Video;
	uint32_t		machineType;
	void			*deviceTreeP;
	uint32_t		deviceTreeLength;
	char			CommandLine[BOOT_LINE_LENGTH];
} boot_args;

#endif /* _PEXPERT_ARM_BOOT_H_ */
