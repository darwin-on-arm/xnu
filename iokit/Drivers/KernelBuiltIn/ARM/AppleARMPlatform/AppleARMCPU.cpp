//
//  AppleARMCPU.cpp
//  AppleARMPlatform
//
//  Created by rms on 5/22/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#include "AppleARMCPU.h"

#define super IOCPU
OSDefineMetaClassAndStructors(ARMCPU, IOCPU);

bool ARMCPU::init(OSDictionary *propTable) {
    PE_LOG("Initializing ARM CPU.\n");
    if (!super::init(propTable)) {
        panic("IOCPU failed to initialize");
    }
    return true;
}

void ARMCPU::ipiHandler(void* refCon, IOService* nub, int source) {
    if(ipi_handler)
        ipi_handler();
    return;
}

bool ARMCPU::start(IOService* provider) {
    PE_LOG("Starting ARM cpu provider\n");
    
    if(!super::start(provider)) {
        panic("Failed to start super IOCPU provider");
    }
    
    gIC = new ARMDumbInterruptController;
    if(!gIC) {
        panic("Failed to alloc class for dumb interrupt controller, we suck hard");
    }
    
    gIC->initCPUInterruptController(1);
    gIC->attach(this);
    gIC->registerCPUInterruptController();
    
    setCPUState(kIOCPUStateUninitalized);
    
    initCPU(true);

    registerService();
  
    return true;
}

void ARMCPU::initCPU(bool boot) {
    PE_LOG("Setting active cpu state\n");
    if(gIC) {
        gIC->enableCPUInterrupt(this);
    }
    setCPUState(kIOCPUStateRunning);
}

void ARMCPU::quiesceCPU(void) {
    PE_LOG("Quiescing processor\n");
    return;
}

kern_return_t ARMCPU::startCPU(vm_offset_t start_paddr, vm_offset_t parg_addr) {
    PE_LOG("Starting CPU, start_paddr: 0x%08x, parg_addr: 0x%08x", start_paddr, parg_addr);
    return KERN_FAILURE;
}

void ARMCPU::haltCPU(void) {
    PE_LOG("Halting processor\n");
    return;
}

const OSSymbol* ARMCPU::getCPUName(void) {
    return OSSymbol::withCStringNoCopy("Primary0");
}

/*
 * I AM THOR. THOR HUNGRY, THOR WANT EAT
 */
#undef super
#define super IOCPUInterruptController
OSDefineMetaClassAndStructors(ARMDumbInterruptController, IOCPUInterruptController);

IOReturn ARMDumbInterruptController::handleInterrupt(void *refCon, IOService* nub, int source) {
    PE_LOG("Attempting to dispatch an interrupt! (%p, %p, %d)\n", refCon, nub, source);
    return super::handleInterrupt(refCon, nub, source);
}
