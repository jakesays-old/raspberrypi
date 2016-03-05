#include "LogSupport.h"
#include "I2CBus.h"
#include "I2CTransaction_.h"
#include <stdio.h>
//#include "gpio/pigpio.h">"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
//#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <algorithm>

I2CBus::I2CBus(int adapter, int address, int sdaPin, int sclPin, int baudRate, 
	int receiveBufferSize /*= 1024*/, int sendBufferSize /*= 1024*/) 
	: _adapter(adapter),
	_address(address),
	_sdaPin(sdaPin),
	_sclPin(sclPin),
	_baudRate(baudRate),
	_receiveBuffer(receiveBufferSize),
	_sendBuffer(sendBufferSize),
	_transactionDepth(0),
	_i2cHandle(-1)
{
}

#define check(_v) \
{ \
	if (_v < 0) \
	{ \
		auto e = errno; \
		LOG(ERROR) << "err: " << e << " - " << strerror(e); \
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

bool I2CBus::Initialize()
{
	char filename[20];

	snprintf(filename, 19, "/dev/i2c-%d", _adapter);
	_i2cHandle = open(filename, O_RDWR | O_NONBLOCK);
	check(_i2cHandle);
	
    ioctl(_i2cHandle, I2C_SLAVE, _address);
	check(_i2cHandle);

	return true;
}

void I2CBus::Close()
{
	close(_i2cHandle);
}

void I2CBus::BeginTransmission()
{
}

void I2CBus::EndTransmission()
{
}

Status I2CBus::Send(int cmd, byte value)
{
    auto result = i2c_smbus_write_byte_data(_i2cHandle, byte(cmd), value);
	checkStatus(result, Status::SendFail);
	
	return Status::Ok;
}

Status I2CBus::Send(int cmd, byte* data, int dataLen)
{
	int result;
	
	if (dataLen <= I2C_SMBUS_BLOCK_MAX)
	{
		result = i2c_smbus_write_i2c_block_data(_i2cHandle,
            byte(cmd),
			dataLen,
			data);
		checkStatus(result, Status::SendFail);
	}
	else
	{
		int suboffset = 0;
		
		while (suboffset < dataLen)
		{
			result = i2c_smbus_write_i2c_block_data(_i2cHandle,
                byte(cmd) + suboffset,
				std::min(I2C_SMBUS_BLOCK_MAX, dataLen - suboffset),
				data + suboffset);
			checkStatus(result, Status::SendFail);

			suboffset += I2C_SMBUS_BLOCK_MAX;
		}
	}
	
	return Status::Ok;
}

IoBuffer& I2CBus::ReceivedData()
{	
	return _receiveBuffer;
}

Status I2CBus::Receive(int cmd, byte& value)
{
    auto result = i2c_smbus_read_byte_data(_i2cHandle, byte(cmd));
	checkStatus(result, Status::ReceiveFail);
	
	value = byte(result);
	
	return Status::Ok;
}

Status I2CBus::Receive(int id, int dataLen)
{
	int result;
	
	_receiveBuffer.Reset();	
	_receiveBuffer.SetWriteLength(dataLen);

	if (dataLen <= I2C_SMBUS_BLOCK_MAX)
	{
		result = i2c_smbus_read_i2c_block_data(_i2cHandle,
			byte(id),
			dataLen,
			_receiveBuffer.Data());
		checkStatus(result, Status::ReceiveFail);
	}
	else
	{
		int suboffset = 0;

		while (suboffset < dataLen)
		{		
			result = i2c_smbus_read_i2c_block_data(_i2cHandle,
				byte(id) + suboffset,
				std::min(I2C_SMBUS_BLOCK_MAX, dataLen - suboffset),
				_receiveBuffer.Data() + suboffset);
			checkStatus(result, Status::ReceiveFail);

			suboffset += I2C_SMBUS_BLOCK_MAX;
		}
	}	
	return Status::Ok;
}

Status I2CBus::Receive(int id, byte* data, int dataLen)
{
	int result;	

	if (dataLen <= I2C_SMBUS_BLOCK_MAX)
	{
		result = i2c_smbus_read_i2c_block_data(_i2cHandle,
			byte(id),
			dataLen,
			data);
		checkStatus(result, Status::ReceiveFail);
	}
	else
	{
		auto readCount = 0;

		while (readCount < dataLen)
		{		
			result = i2c_smbus_read_i2c_block_data(_i2cHandle,
				byte(id) + readCount,
				std::min(I2C_SMBUS_BLOCK_MAX, dataLen - readCount),
				data + readCount);
			checkStatus(result, Status::ReceiveFail);

			readCount += I2C_SMBUS_BLOCK_MAX;
		}
	}
	
	return Status::Ok;
}
