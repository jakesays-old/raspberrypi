#include "I2CGpioBus.h"
#include "LogSupport.h"
#include <stdio.h>
#include "gpio/pigpio.h"
#include "gpio/i2c.h"

#define check(_v) \
{ \
    if (_v < 0) \
    { \
        LOG(ERROR) << "err: " << _v << " - " << getErrorMessage(_v); \
        return false; \
    } \
} \

bool I2CGpioBus::InitializeGpioBus()
{
    int result;

    gpioCfgInterfaces(PI_DISABLE_SOCK_IF | PI_DISABLE_FIFO_IF);
    gpioCfgMemAlloc(PI_MEM_ALLOC_PAGEMAP);

    result = gpioInitialise();
    check(result);

    return true;
}

void I2CGpioBus::ShutdownGpioBus()
{
    gpioTerminate();
}
