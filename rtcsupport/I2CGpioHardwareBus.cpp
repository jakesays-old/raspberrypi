#include "LogSupport.h"
#include "I2CGpioHardwareBus.h"
#include <stdio.h>
#include "gpio/pigpio.h"
#include "gpio/i2c.h"
#include "IoError.h"

#define check(_v) \
{ \
    if (_v < 0) \
    { \
        LOG(ERROR) << "err: " << _v << " - " << getErrorMessage(_v); \
        return false; \
    } \
} \

#define checkStatus(_v, _s) \
{ \
    if (_v < 0) \
    { \
        auto e = errno; \
        LOG(ERROR) << "err: " << e << " - " << strerror(e); \
        return _s; \
    } \
} \

I2CGpioHardwareBus::I2CGpioHardwareBus(int adapter, int address)
    : _adapter(adapter),
      _address(address),
	_i2cHandle(-1)
{
}

bool I2CGpioHardwareBus::Initialize()
{
    check(InitializeGpioBus());

    _i2cHandle = i2cOpen(1, _address, 0);
    check(_i2cHandle);

    return true;
}

void I2CGpioHardwareBus::Close()
{
    i2cClose(_i2cHandle);
    ShutdownGpioBus();
}


Status I2CGpioHardwareBus::Send(byte value)
{
    checkStatus(i2cWriteByte(_i2cHandle, value), Status::SendFail);

    return Status::Ok;
}

Status I2CGpioHardwareBus::Send(byte cmd, byte value)
{
    checkStatus(i2cWriteByteData(_i2cHandle, cmd, value), Status::SendFail);

    return Status::Ok;
}

#define I2C_BLOCK_SIZE 32

Status I2CGpioHardwareBus::Send(byte cmd, byte* data, int dataLen)
{
    int result;

    if (dataLen <= I2C_BLOCK_SIZE)
    {
        result = i2cWriteBlockData(_i2cHandle, uint32_t(cmd), (char*) data, dataLen);
        checkStatus(result, Status::SendFail);
    }
    else
    {
        int suboffset = 0;

        while (suboffset < dataLen)
        {
            result = i2cWriteBlockData(_i2cHandle,
                cmd + suboffset,
                (char*) (data + suboffset),
                std::min(I2C_BLOCK_SIZE, dataLen - suboffset));
            checkStatus(result, Status::SendFail);

            suboffset += I2C_BLOCK_SIZE;
        }
    }

    return Status::Ok;
}

Status I2CGpioHardwareBus::Send(byte cmd, word value)
{
    checkStatus(i2cWriteWordData(_i2cHandle, cmd, value), Status::SendFail);

    return Status::Ok;
}

Status I2CGpioHardwareBus::Receive(byte& value)
{
    auto result = i2cReadByte(_i2cHandle);
    checkStatus(result, Status::ReceiveFail);

    value = byte(result & 0x000000FF);

    return Status::Ok;
}

Status I2CGpioHardwareBus::Receive(byte cmd, byte& value)
{
    auto result = i2cReadByteData(_i2cHandle, cmd);
    checkStatus(result, Status::ReceiveFail);

    value = byte(result & 0x000000FF);

    return Status::Ok;
}

Status I2CGpioHardwareBus::Receive(byte cmd, byte* data, int dataLen)
{
    int result;

    if (dataLen <= I2C_BLOCK_SIZE)
    {
        result = i2cReadI2CBlockData(_i2cHandle,
            cmd,
            (char*) data,
            dataLen);
        checkStatus(result, Status::ReceiveFail);
    }
    else
    {
        auto readCount = 0;

        while (readCount < dataLen)
        {
            result = i2cReadI2CBlockData(_i2cHandle,
                cmd + readCount,
                (char*) (data + readCount),
                std::min(I2C_BLOCK_SIZE, dataLen - readCount));
            checkStatus(result, Status::ReceiveFail);

            readCount += I2C_BLOCK_SIZE;
        }
    }

    return Status::Ok;
}

Status I2CGpioHardwareBus::Receive(byte cmd, word& value)
{
    auto result = i2cReadWordData(_i2cHandle, cmd);
    checkStatus(result, Status::ReceiveFail);

    value = word(result & 0x0000FFFF);

    return Status::Ok;
}

