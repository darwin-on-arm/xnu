//
//  AppleARMPE.cpp
//  AppleARMPlatform
//
//  Created by rms on 5/21/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#include "AppleARMPE.h"
#include <kern/debug.h>

#define HIGH_SCORE  100000
#define super IOPlatformExpert

OSDefineMetaClassAndStructors(ARMPlatformExpert, IODTPlatformExpert);

bool ARMPlatformExpert::init(OSDictionary *propTable) {
    PE_LOG("Initializing ARM platform expert.\n");
    if (!super::init(propTable)) {
        panic("IOPlatformExpert failed to initialize");
    }
    return true;
}

extern const IORegistryPlane* gIODTPlane;

void ARMPlatformExpert::registerNVRAMController(IONVRAMController * caller)
{
    publishResource("IONVRAM");
}

bool ARMPlatformExpert::start(IOService *provider) {
    PE_LOG("Starting ARM platform expert, IOService at %p\n", provider);

    if(!super::start(provider)) {
        panic("IOPlatformExpert failed to start");
    }

    removeProperty(kIOPlatformMapperPresentKey);
    assert(IOService::getPlatform() == this);

    registerService();
    
    populate_model_name("Felix");

    PE_LOG("Registered device with IOKit\n");

    return true;
}

const char * ARMPlatformExpert::deleteList ( void )
{
    return "";
}

const char * ARMPlatformExpert::excludeList( void )
{
    return("'chosen', 'memory', 'options', 'aliases'");
}

IOService* ARMPlatformExpert::probe(IOService *provider, SInt32* score) {
    return this;
}

bool ARMPlatformExpert::getMachineName(char *name, int maxLength) {
    strncpy(name, "generic-arm", maxLength);
    return true;
}
