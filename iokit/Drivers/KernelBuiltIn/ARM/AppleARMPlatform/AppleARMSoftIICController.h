//
// AppleARMSoftIICController
//
// rms
//

#ifndef _AppleARMSoftIICController_H_
#define _AppleARMSoftIICController_H_

#include <IOKit/IOLib.h>

class AppleARMSoftIICController:IOService {
    OSDeclareDefaultStructors(AppleARMSoftIICController);

 public:
    virtual bool start(IOService * provider);
    virtual IOService *probe(IOService * provider, SInt32 * score);
};

#endif
