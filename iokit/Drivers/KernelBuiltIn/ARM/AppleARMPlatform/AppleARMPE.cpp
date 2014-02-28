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

    OSData *        prop;
    bool        ok = false;

    prop = (OSData *) getProvider()->getProperty( gIODTModelKey );
    ok = (0 != prop);

    if( ok )
        populate_model_name((char *) prop->getBytesNoCopy());
    else
        populate_model_name("Power Macintosh");

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
    OSData *        prop;
    bool        ok = false;

    maxLength--;
    prop = (OSData *) getProvider()->getProperty( gIODTModelKey );
    ok = (0 != prop);

    if( ok )
        strlcpy( name, (const char *) prop->getBytesNoCopy(), maxLength );
    else 
        strlcpy( name, "Power Macintosh", maxLength );
    
    return(true);
}
