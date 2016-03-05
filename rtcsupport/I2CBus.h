#ifndef I2CBUS_H
#define I2CBUS_H

#include "LocalTypes.h"
#include "IoError.h"
#include "IoBuffer.h"

class I2CBus
{
public:
    virtual Status Send(byte value) = 0;
    virtual Status Send(byte cmd, byte value) = 0;
    virtual Status Send(byte cmd, byte* data, int dataLen) = 0;
    virtual Status Send(byte cmd, word value) = 0;

    virtual Status Receive(byte& value) = 0;
    virtual Status Receive(byte cmd, byte& value) = 0;
    virtual Status Receive(byte cmd, byte* data, int dataLen) = 0;
    virtual Status Receive(byte cmd, word& value) = 0;
};

#endif //I2CBUS_H
