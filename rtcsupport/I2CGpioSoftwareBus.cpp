#include "LogSupport.h"
#include "I2CGpioSoftwareBus.h"
#include <stdio.h>
#include "gpio/pigpio.h"
#include "gpio/i2c.h"

I2CGpioSoftwareBus::I2CGpioSoftwareBus(int address, int sdaPin, int sclPin, int baudRate,
	int receiveBufferSize /*= 1024*/, int sendBufferSize /*= 1024*/) 
	: _address(address),
	_sdaPin(sdaPin),
	_sclPin(sclPin),
	_baudRate(baudRate),
	_receiveBuffer(receiveBufferSize),
    _sendBuffer(sendBufferSize)
{
}

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

#define checkStatusReturn(_s) \
{ \
    auto __result = _s; \
    if (__result != Status::Ok) \
    { \
        return __result; \
    } \
} \

bool I2CGpioSoftwareBus::Initialize()
{
    if (!InitializeGpioBus())
    {
        return false;
    }
	
    check(bbI2COpen(_sdaPin, _sclPin, _baudRate));

    return true;
}

void I2CGpioSoftwareBus::Close()
{
    bbI2CClose(_sdaPin);
    ShutdownGpioBus();
}

void I2CGpioSoftwareBus::Prepare()
{
    _sendBuffer.Clear();
    _receiveBuffer.Clear();

    _sendBuffer.Address(_address);
}


Status I2CGpioSoftwareBus::Transmit(int readCount /* = 0*/)
{
    if (_sendBuffer.WriteLength() == 0)
    {
        return Status::SendFail;
    }

    _sendBuffer.End();
    _receiveBuffer.Clear();

    vlogMed << "SENDING: " << _sendBuffer.AsString();

    auto result = bbI2CZip(_sdaPin,
        reinterpret_cast<char*>(_sendBuffer.Data()),
        _sendBuffer.WriteLength(),
        reinterpret_cast<char*>(_receiveBuffer.Data()),
        readCount);
    if (result < 0)
    {
        logError
            << "bbI2CZip failed with (" << result << ")"
            << getErrorMessage(result);

        return Status::SendFail;
    }

    _receiveBuffer.SetWriteLength(result);

    vlogMed << "RECEIVED: " << _receiveBuffer.AsString();

    return Status::Ok;
}

Status I2CGpioSoftwareBus::Send(byte value)
{
    Prepare();

    _sendBuffer.Start();
    _sendBuffer.Send(1);
    _sendBuffer.Write(value);
    _sendBuffer.StopAndEnd();

    return Transmit();
}

Status I2CGpioSoftwareBus::Send(byte cmd, byte value)
{
    Prepare();

    _sendBuffer.Start();
    _sendBuffer.Send(2);
    _sendBuffer.Write(cmd);
    _sendBuffer.Write(value);
    _sendBuffer.StopAndEnd();

    return Transmit();
}

Status I2CGpioSoftwareBus::Send(byte cmd, byte* data, int dataLen)
{
    Prepare();

    _sendBuffer.Start();
    _sendBuffer.Send(dataLen + 1);
    _sendBuffer.Write(cmd);
    _sendBuffer.Write(data, dataLen);
    _sendBuffer.StopAndEnd();

    return Transmit();
}

Status I2CGpioSoftwareBus::Send(byte cmd, word value)
{
    Prepare();

    _sendBuffer.Start();
    _sendBuffer.Send(3);
    _sendBuffer.Write(cmd);
    _sendBuffer.Write(byte(value >> 4));
    _sendBuffer.Write(byte(value & 0x0F));
    _sendBuffer.StopAndEnd();

    return Transmit();
}

#define checkReceiveLen(_l) \
do { \
    if (_receiveBuffer.ReadLength() < _l) \
    { \
        return Status::ReceivedLessThanExpected; \
    } \
} while (0)

Status I2CGpioSoftwareBus::Receive(byte& value)
{
    Prepare();

    _sendBuffer.Start();
    _sendBuffer.Receive(1);
    _sendBuffer.StopAndEnd();

    Transmit(1);
    checkReceiveLen(1);

    value = _receiveBuffer.Read8();

    return Status::Ok;
}

Status I2CGpioSoftwareBus::Receive(byte cmd, byte& value)
{
    Prepare();

    _sendBuffer.Start();
    _sendBuffer.Send(1);
    _sendBuffer.Write(cmd);
    _sendBuffer.Restart();
    _sendBuffer.Receive(1);
    _sendBuffer.StopAndEnd();

    Transmit(1);
    checkReceiveLen(1);

    value = _receiveBuffer.Read8();

    return Status::Ok;
}

Status I2CGpioSoftwareBus::Receive(byte cmd, byte* data, int dataLen)
{
    Prepare();

    _sendBuffer.Start();
    _sendBuffer.Send(1);
    _sendBuffer.Write(cmd);
    _sendBuffer.Restart();
    _sendBuffer.Receive(dataLen);
    _sendBuffer.StopAndEnd();

    Transmit(dataLen);
    checkReceiveLen(dataLen);

    _receiveBuffer.Read(data, dataLen);

    return Status::Ok;
}

Status I2CGpioSoftwareBus::Receive(byte cmd, word& value)
{
    Prepare();

    _sendBuffer.Start();
    _sendBuffer.Send(1);
    _sendBuffer.Write(cmd);
    _sendBuffer.Restart();
    _sendBuffer.Receive(2);
    _sendBuffer.StopAndEnd();

    Transmit(2);
    checkReceiveLen(2);

    value = word(_receiveBuffer.Read8() << 4 | _receiveBuffer.Read8());

    return Status::Ok;
}
