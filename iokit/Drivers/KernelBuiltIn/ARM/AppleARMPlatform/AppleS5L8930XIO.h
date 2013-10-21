//
//  AppleS5L8930XIO.h
//  AppleS5L8930XDevice
//
//  Created by rms on 5/23/13.
//  Copyright (c) 2013 rms. All rights reserved.
//

#ifndef __AppleS5L8930XDevice__AppleS5L8930XIO__
#define __AppleS5L8930XDevice__AppleS5L8930XIO__

#include <mach/mach_types.h>
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include "AppleARMIO.h"

class AppleARMIO:public ARMIO {
    OSDeclareDefaultStructors(AppleARMIO);
 public:
    bool init(OSDictionary * propTable);
    IOService *probe(IOService * provider, SInt32 * score);
    bool start(IOService * provider);
};

#endif                          /* defined(__AppleS5L8930XDevice__AppleS5L8930XIO__) */
