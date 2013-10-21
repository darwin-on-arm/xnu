//
//  AppleARMCPU.h
//  AppleARMPlatform
//
//  Created by rms on 5/22/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#ifndef __AppleARMPlatform__AppleARMCPU__
#define __AppleARMPlatform__AppleARMCPU__

#include <mach/mach_types.h>
#include <IOKit/IOPlatformExpert.h>
#include "IOCPU.h"

#define PE_LOG \
    IOLog("[%s]: ", __PRETTY_FUNCTION__), IOLog

class AppleARMCPU:public IOCPU {
    OSDeclareDefaultStructors(AppleARMCPU);
 private:
    IOCPUInterruptController * gIC;
 public:
    bool start(IOService * provider);
    void initCPU(bool boot);
    void quiesceCPU(void);
    kern_return_t startCPU(vm_offset_t start_paddr, vm_offset_t parg_addr);
    void haltCPU(void);
    const OSSymbol *getCPUName(void);
    bool init(OSDictionary * propTable);
    void ipiHandler(void *refCon, IOService * nub, int source);
};

/*
 * AppleARMGrandCentral
 */

class AppleARMGrandCentral:public IOCPUInterruptController {
    OSDeclareDefaultStructors(AppleARMGrandCentral);
 public:
    IOReturn handleInterrupt(void *refCon, IOService * nub, int source);
};

#endif                          /* defined(__AppleARMPlatform__AppleARMCPU__) */
