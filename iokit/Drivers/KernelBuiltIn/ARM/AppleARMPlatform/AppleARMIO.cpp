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

OSDefineMetaClassAndAbstractStructors(ARMIO, IOService);
OSMetaClassDefineReservedUnused(ARMIO,  0);
OSMetaClassDefineReservedUnused(ARMIO,  1);
OSMetaClassDefineReservedUnused(ARMIO,  2);
OSMetaClassDefineReservedUnused(ARMIO,  3);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

extern const IORegistryPlane* gIODTPlane;

bool ARMIO::start( IOService * provider )
{
    if( !super::start( provider))
        return( false);
    
    fNub = provider;
    fMemory = provider->mapDeviceMemoryWithIndex( 0 );
    if( 0 == fMemory)
        IOLog("%s: unexpected ranges\n", getName());
    
    PMinit();		// initialize for power management
    temporaryPowerClampOn();	// hold power on till we get children
    return( true);
}

IOService * ARMIO::createNub( IORegistryEntry * from )
{
    IOService *	nub;
    
    nub = new ARMIODevice;
    
    if( nub && !nub->init( from, gIODTPlane )) {
        nub->free();
        nub = 0;
    }
    
    return( nub);
}

void ARMIO::processNub(IOService * /*nub*/)
{
}

const char * ARMIO::deleteList ( void )
{
    return( "('sd', 'st', 'disk', 'tape', 'pram', 'rtc', 'mouse')" );
}

const char * ARMIO::excludeList( void )
{
    return( 0 );
}

void ARMIO::publishBelow( IORegistryEntry * root )
{
    OSCollectionIterator *	kids;
    IORegistryEntry *		next;
    IOService *			nub;
    
    // infanticide
    kids = IODTFindMatchingEntries( root, kIODTRecursive, deleteList() );
    if( kids) {
        while( (next = (IORegistryEntry *)kids->getNextObject())) {
            next->detachAll( gIODTPlane);
        }
        kids->release();
    }
    
    // publish everything below, minus excludeList
    kids = IODTFindMatchingEntries( root, kIODTRecursive | kIODTExclusive,
                                   excludeList());
    if( kids) {
        while( (next = (IORegistryEntry *)kids->getNextObject())) {
            
            if( 0 == (nub = createNub( next )))
                continue;
            
            nub->attach( this );
            
            processNub(nub);
            
            nub->registerService();
        }
        kids->release();
    }
}

bool ARMIO::compareNubName( const IOService * nub,
                                OSString * name, OSString ** matched ) const
{
    return( IODTCompareNubName( nub, name, matched )
           ||  nub->IORegistryEntry::compareName( name, matched ) );
}

IOReturn ARMIO::getNubResources( IOService * nub )
{
    if( nub->getDeviceMemory())
        return( kIOReturnSuccess );
    
    IODTResolveAddressing( nub, "reg", fNub->getDeviceMemoryWithIndex(0) );
    
    return( kIOReturnSuccess);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#undef super
#define super IOService

OSDefineMetaClassAndStructors(ARMIODevice, IOService);
OSMetaClassDefineReservedUnused(ARMIODevice,  0);
OSMetaClassDefineReservedUnused(ARMIODevice,  1);
OSMetaClassDefineReservedUnused(ARMIODevice,  2);
OSMetaClassDefineReservedUnused(ARMIODevice,  3);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool ARMIODevice::compareName( OSString * name,
                                   OSString ** matched ) const
{
    return (IODTCompareNubName(this, name, matched) ||
            IORegistryEntry::compareName(name, matched));
}

IOService * ARMIODevice::matchLocation( IOService * /* client */ )
{
    return this;
}

IOReturn ARMIODevice::getResources( void )
{
    IOService *macIO = this;

    if (getDeviceMemory() != 0) return kIOReturnSuccess;
    
    while (macIO && ((macIO = macIO->getProvider()) != 0))
        if (strcmp("arm-io", macIO->getName()) == 0) break;
    
    if (macIO == 0) return kIOReturnError;
    
    IODTResolveAddressing(this, "reg", macIO->getDeviceMemoryWithIndex(0));
    
    return kIOReturnSuccess;
}

