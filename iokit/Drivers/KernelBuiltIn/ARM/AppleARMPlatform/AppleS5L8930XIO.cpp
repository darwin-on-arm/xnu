//
//  AppleS5L8930XIO.cpp
//  AppleS5L8930XDevice
//
//  Created by rms on 5/23/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#include "AppleS5L8930XIO.h"

#define super IOService
OSDefineMetaClassAndStructors(AppleGenericARMIO, ARMIO);

#define S5L_LOG \
    IOLog("[%s] ", __PRETTY_FUNCTION__), IOLog

bool AppleGenericARMIO::init(OSDictionary* propTable) {
    if(!super::init(propTable)) {
        panic("failed to initialize super IOService\n");
    }
    S5L_LOG("Initializing AppleGenericARMIO service\n");
    return true;
}

IOService* AppleGenericARMIO::probe(IOService* provider, SInt32* score) {
    return this;
}

bool AppleGenericARMIO::start(IOService* provider) {
    if(!super::start(provider)) {
        panic("failed to start super provider");
    }
    S5L_LOG("Publishing entries to IOService tree plane\n");
    publishBelow(provider);
    registerService();
    IOPrintPlane(gIOServicePlane);
    return true;
}
