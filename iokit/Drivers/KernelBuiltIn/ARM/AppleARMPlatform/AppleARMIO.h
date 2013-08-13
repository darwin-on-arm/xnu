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

class ARMIODevice : public IOService
{
    OSDeclareDefaultStructors(ARMIODevice);
    
private:
    struct ExpansionData { };
    ExpansionData *reserved;
    
public:
    virtual bool compareName( OSString * name, OSString ** matched = 0 ) const;
    virtual IOService *matchLocation(IOService *client);
    virtual IOReturn getResources( void );
    
    OSMetaClassDeclareReservedUnused(ARMIODevice,  0);
    OSMetaClassDeclareReservedUnused(ARMIODevice,  1);
    OSMetaClassDeclareReservedUnused(ARMIODevice,  2);
    OSMetaClassDeclareReservedUnused(ARMIODevice,  3);
};


class ARMIO : public IOService
{
    OSDeclareAbstractStructors(ARMIO);
    
    IOService *		fNub;
    IOMemoryMap *	fMemory;
    
    struct ExpansionData { };
    ExpansionData *fReserved;
    
public:
    virtual bool start(	IOService * provider );
    
    virtual IOService * createNub( IORegistryEntry * from );
    
    virtual void processNub( IOService * nub );
    
    virtual void publishBelow( IORegistryEntry * root );
    
    virtual const char * deleteList( void );
    virtual const char * excludeList( void );
    
    virtual bool compareNubName( const IOService * nub, OSString * name,
                                OSString ** matched = 0 ) const;
    
    virtual IOReturn getNubResources( IOService * nub );
    
    OSMetaClassDeclareReservedUnused(ARMIO,  0);
    OSMetaClassDeclareReservedUnused(ARMIO,  1);
    OSMetaClassDeclareReservedUnused(ARMIO,  2);
    OSMetaClassDeclareReservedUnused(ARMIO,  3);
};


#endif /* defined(__AppleARMPlatform__AppleARMIO__) */
