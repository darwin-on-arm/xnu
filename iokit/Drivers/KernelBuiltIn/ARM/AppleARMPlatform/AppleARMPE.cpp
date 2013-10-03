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
    IOLog("ARMPlatformExpert::start: Welcome to the NeXT generation.\n");

    if(!super::start(provider)) {
        panic("IOPlatformExpert failed to start");
    }

    removeProperty(kIOPlatformMapperPresentKey);
    assert(IOService::getPlatform() == this);

    registerService();
   
    /* Let these time out to let everything else initialize right. */
#if 0
    publishResource("IONVRAM");
    publishResource("IORTC");
#endif
    
    populate_model_name("Felix");

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
