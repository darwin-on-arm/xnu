//
//  AppleARMIO.cpp
//  AppleARMPlatform
//
//  Created by rms on 5/22/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#include "AppleARMIO.h"
#include <IOKit/IOLib.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IODeviceMemory.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define super IOService

OSDefineMetaClassAndAbstractStructors(AppleARMIOBus, IOService);
OSMetaClassDefineReservedUnused(AppleARMIOBus, 0);
OSMetaClassDefineReservedUnused(AppleARMIOBus, 1);
OSMetaClassDefineReservedUnused(AppleARMIOBus, 2);
OSMetaClassDefineReservedUnused(AppleARMIOBus, 3);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

extern const IORegistryPlane *gIODTPlane;

bool AppleARMIOBus::start(IOService * provider)
{
    if (!super::start(provider))
        return (false);

    fNub = provider;
    fMemory = provider->mapDeviceMemoryWithIndex(0);
    if (0 == fMemory)
        IOLog("%s: unexpected ranges\n", getName());

    PMinit();                   // initialize for power management
    temporaryPowerClampOn();    // hold power on till we get children
    return (true);
}

IOService *AppleARMIOBus::createNub(IORegistryEntry * from)
{
    IOService *nub;

    nub = new ARMIODevice;

    if (nub && !nub->init(from, gIODTPlane)) {
        nub->free();
        nub = 0;
    }

    return (nub);
}

void AppleARMIOBus::processNub(IOService * /*nub */ )
{
}

const char *AppleARMIOBus::deleteList(void)
{
    return ("('sd', 'st', 'disk', 'tape', 'pram', 'rtc', 'mouse')");
}

const char *AppleARMIOBus::excludeList(void)
{
    return (0);
}

void AppleARMIOBus::publishBelow(IORegistryEntry * root)
{
    OSCollectionIterator *kids;
    IORegistryEntry *next;
    IOService *nub;

    // infanticide
    kids = IODTFindMatchingEntries(root, kIODTRecursive, deleteList());
    if (kids) {
        while ((next = (IORegistryEntry *) kids->getNextObject())) {
            next->detachAll(gIODTPlane);
        }
        kids->release();
    }
    // publish everything below, minus excludeList
    kids = IODTFindMatchingEntries(root, kIODTRecursive | kIODTExclusive, excludeList());
    if (kids) {
        while ((next = (IORegistryEntry *) kids->getNextObject())) {

            if (0 == (nub = createNub(next)))
                continue;

            nub->attach(this);

            processNub(nub);

            nub->registerService();
        }
        kids->release();
    }
}

bool AppleARMIOBus::compareNubName(const IOService * nub, OSString * name, OSString ** matched) const const
{
    return (IODTCompareNubName(nub, name, matched)
            || nub->IORegistryEntry::compareName(name, matched));
}

IOReturn AppleARMIOBus::getNubResources(IOService * nub)
{
    if (nub->getDeviceMemory())
        return (kIOReturnSuccess);

    IODTResolveAddressing(nub, "reg", fNub->getDeviceMemoryWithIndex(0));

    return (kIOReturnSuccess);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#undef super
#define super IOService

OSDefineMetaClassAndStructors(AppleARMIODevice, IOService);
OSMetaClassDefineReservedUnused(AppleARMIODevice, 0);
OSMetaClassDefineReservedUnused(AppleARMIODevice, 1);
OSMetaClassDefineReservedUnused(AppleARMIODevice, 2);
OSMetaClassDefineReservedUnused(AppleARMIODevice, 3);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool AppleARMIODevice::compareName(OSString * name, OSString ** matched) const const
{
    return (IODTCompareNubName(this, name, matched) || IORegistryEntry::compareName(name, matched));
}

IOService *AppleARMIODevice::matchLocation(IOService * /* client */ )
{
    return this;
}

IOReturn AppleARMIODevice::getResources(void)
{
    IOService *macIO = this;

    if (getDeviceMemory() != 0)
        return kIOReturnSuccess;

    while (macIO && ((macIO = macIO->getProvider()) != 0))
        if (strcmp("arm-io", macIO->getName()) == 0)
            break;

    if (macIO == 0)
        return kIOReturnError;

    IODTResolveAddressing(this, "reg", macIO->getDeviceMemoryWithIndex(0));

    return kIOReturnSuccess;
}
