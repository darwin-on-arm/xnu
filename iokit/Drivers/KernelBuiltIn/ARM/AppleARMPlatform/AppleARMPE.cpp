//
//  AppleARMPE.cpp
//  AppleARMPlatform
//
//  Created by rms on 5/21/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#include "AppleARMPE.h"
#include <kern/debug.h>
#include <pexpert/device_tree.h>

#define HIGH_SCORE  100000
#define super IOPlatformExpert

OSDefineMetaClassAndStructors(AppleARMPE, IODTPlatformExpert);

bool AppleARMPE::init(OSDictionary * propTable)
{
    if (!super::init(propTable)) {
        panic("IOPlatformExpert failed to initialize");
    }
    return true;
}

extern const IORegistryPlane *gIODTPlane;

void AppleARMPE::registerNVRAMController(IONVRAMController * caller)
{
    publishResource("IONVRAM");
}

bool AppleARMPE::start(IOService * provider)
{
    DTEntry entry;
    char *dtype;
    unsigned int size;

    IOLog("AppleARMPE::start: Welcome to the NeXT generation.\n");

    if (!super::start(provider)) {
        panic("IOPlatformExpert failed to start");
    }

    removeProperty(kIOPlatformMapperPresentKey);
    assert(IOService::getPlatform() == this);

    registerService();

    /*
     * Let these time out to let everything else initialize right. 
     */
#if 0
    publishResource("IONVRAM");
    publishResource("IORTC");
#endif

    if (kSuccess == DTLookupEntry(NULL, "/", &entry)) {
        /*
         * What's the device name? 
         */
        if (kSuccess == DTGetProperty(entry, "compatible", (void **) &dtype, &size)) {
            populate_model_name(dtype);
        } else {
            populate_model_name("Generic ARM Device");
        }
    } else {
        populate_model_name("Generic ARM Device");
    }

    return true;
}

const char *AppleARMPE::deleteList(void)
{
    return "";
}

const char *AppleARMPE::excludeList(void)
{
    return ("'chosen', 'memory', 'options', 'aliases'");
}

IOService *AppleARMPE::probe(IOService * provider, SInt32 * score)
{
    return this;
}

bool AppleARMPE::getMachineName(char *name, int maxLength)
{
    strncpy(name, "generic-arm", maxLength);
    return true;
}
