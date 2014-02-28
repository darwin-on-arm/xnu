//
//  AppleS5L8930XIO.cpp
//  AppleS5L8930XDevice
//
//  Created by rms on 5/23/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#include "AppleS5L8930XIO.h"
#include <IOKit/IOPlatformExpert.h>

#define super IOService
OSDefineMetaClassAndStructors(AppleARMIO, ARMIO);

#define S5L_LOG \
    IOLog("[%s] ", __PRETTY_FUNCTION__), IOLog

bool AppleARMIO::init(OSDictionary * propTable)
{
    if (!super::init(propTable)) {
        panic("failed to initialize super IOService\n");
    }
    return true;
}

IOService *AppleARMIO::probe(IOService * provider, SInt32 * score)
{
    return this;
}

bool AppleARMIO::start(IOService * provider)
{
    if (!super::start(provider)) {
        panic("failed to start super provider");
    }
    IOLog("AppleARMIO::start: Publishing device tree entries to IOService plane...\n");
    publishBelow(provider);
    registerService();
    return true;
}
