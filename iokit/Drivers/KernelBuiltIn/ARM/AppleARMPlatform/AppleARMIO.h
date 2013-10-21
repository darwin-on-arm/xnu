//
//  AppleARMIO.h
//  AppleARMPlatform
//
//  Created by rms on 5/22/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#ifndef __AppleARMPlatform__AppleARMIO__
#define __AppleARMPlatform__AppleARMIO__

#include <IOKit/IOService.h>

/*
 * Very plaigirized from xnu-1228
 */

#define ARMIODevice AppleARMIODevice
#define ARMIO AppleARMIOBus

class AppleARMIODevice:public IOService {
    OSDeclareDefaultStructors(AppleARMIODevice);

 private:
    struct ExpansionData {
    };
    ExpansionData *reserved;

 public:
     virtual bool compareName(OSString * name, OSString ** matched = 0) const;
    virtual IOService *matchLocation(IOService * client);
    virtual IOReturn getResources(void);

     OSMetaClassDeclareReservedUnused(AppleARMIODevice, 0);
     OSMetaClassDeclareReservedUnused(AppleARMIODevice, 1);
     OSMetaClassDeclareReservedUnused(AppleARMIODevice, 2);
     OSMetaClassDeclareReservedUnused(AppleARMIODevice, 3);
};

class AppleARMIOBus:public IOService {
    OSDeclareAbstractStructors(AppleARMIOBus);

    IOService *fNub;
    IOMemoryMap *fMemory;

    struct ExpansionData {
    };
    ExpansionData *fReserved;

 public:
     virtual bool start(IOService * provider);

    virtual IOService *createNub(IORegistryEntry * from);

    virtual void processNub(IOService * nub);

    virtual void publishBelow(IORegistryEntry * root);

    virtual const char *deleteList(void);
    virtual const char *excludeList(void);

    virtual bool compareNubName(const IOService * nub, OSString * name, OSString ** matched = 0) const;

    virtual IOReturn getNubResources(IOService * nub);

     OSMetaClassDeclareReservedUnused(AppleARMIOBus, 0);
     OSMetaClassDeclareReservedUnused(AppleARMIOBus, 1);
     OSMetaClassDeclareReservedUnused(AppleARMIOBus, 2);
     OSMetaClassDeclareReservedUnused(AppleARMIOBus, 3);
};

#endif                          /* defined(__AppleARMPlatform__AppleARMIO__) */
