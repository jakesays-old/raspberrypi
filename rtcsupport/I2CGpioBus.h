#ifndef I2CGPIOBUS_H
#define I2CGPIOBUS_H

#include "I2CBus.h"

class I2CGpioBus : public I2CBus
{
public:
    I2CGpioBus(const I2CGpioBus&) = delete;

protected:
    I2CGpioBus();
    bool InitializeGpioBus();
    void ShutdownGpioBus();
};

#endif // I2CGPIOBUS_H
