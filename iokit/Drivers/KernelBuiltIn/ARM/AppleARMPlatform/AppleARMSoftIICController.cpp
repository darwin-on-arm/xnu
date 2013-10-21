//
// AppleARMSoftIICController
//
// rms
//

#include "AppleARMIO.h"
#include "AppleARMSoftIICController.h"

#define super IOService
OSDefineMetaClassAndStructors(AppleARMSoftIICController, IOService)

bool AppleARMSoftIICController::start(IOService * provider)
{
    IOLog("AppleARMSoftIICController::start: Starting software I2C controller\n");

    if (!super::start(provider)) {
        panic("Failed to start super IOService provider");
    }

    registerService();

    return true;
}

IOService *AppleARMSoftIICController::probe(IOService * provider, SInt32 * score)
{
    return this;
}
