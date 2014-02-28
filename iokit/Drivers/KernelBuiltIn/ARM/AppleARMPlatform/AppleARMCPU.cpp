//
//  AppleARMCPU.cpp
//  AppleARMPlatform
//
//  Created by rms on 5/22/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#include "AppleARMCPU.h"

#define super IOCPU
OSDefineMetaClassAndStructors(AppleARMCPU, IOCPU);

bool AppleARMCPU::init(OSDictionary * propTable)
{
    if (!super::init(propTable)) {
        panic("IOCPU failed to initialize");
    }
    return true;
}

void AppleARMCPU::ipiHandler(void *refCon, IOService * nub, int source)
{
    if (ipi_handler)
        ipi_handler();
    return;
}

bool AppleARMCPU::start(IOService * provider)
{
    IOLog("AppleARMCPU::start: Starting ARM CPU IOKit provider...\n");

    if (!super::start(provider)) {
        panic("Failed to start super IOCPU provider");
    }

    gIC = new AppleARMGrandCentral;
    if (!gIC) {
        panic("Failed to alloc class for dumb interrupt controller, we suck hard");
    }

    gIC->initCPUInterruptController(1);
    gIC->attach(this);
    gIC->registerCPUInterruptController();

    setCPUState(kIOCPUStateUninitalized);

    initCPU(true);

    registerService();


    ml_processor_info_t info;
    info.cpu_id = this;
    info.boot_cpu = 1;
    info.l2cr_value = 0;
    info.start_paddr = 0x100;
    info.supports_nap = 0;
    info.time_base_enable = NULL;

    kern_return_t result = ml_processor_register(&info, &machProcessor, &ipi_handler);
    assert(result == KERN_SUCCESS);
    IOLog("AppleARMCPU::start: registered processor with Mach subsystem.\n");
    processor_start(machProcessor);  
  

    return true;
}

void AppleARMCPU::initCPU(bool boot)
{
    IOLog("AppleARMCPU::initCPU(%p): we are here to serve!\n", this);
    if (gIC) {
        gIC->enableCPUInterrupt(this);
    }
    setCPUState(kIOCPUStateRunning);
}

void AppleARMCPU::quiesceCPU(void)
{
    return;
}

kern_return_t AppleARMCPU::startCPU(vm_offset_t start_paddr, vm_offset_t parg_addr)
{
    return KERN_FAILURE;
}

void AppleARMCPU::haltCPU(void)
{
    return;
}

const OSSymbol *AppleARMCPU::getCPUName(void)
{
    return OSSymbol::withCStringNoCopy("Primary0");
}

/*
 * I AM THOR. THOR HUNGRY, THOR WANT EAT
 */
#undef super
#define super IOCPUInterruptController
OSDefineMetaClassAndStructors(AppleARMGrandCentral, IOCPUInterruptController);

IOReturn AppleARMGrandCentral::handleInterrupt(void *refCon, IOService * nub, int source)
{
    PE_LOG("Attempting to dispatch an interrupt! (%p, %p, %d)\n", refCon, nub, source);
    return super::handleInterrupt(refCon, nub, source);
}
