#include "LogSupport.h"
#include "I2CSmbus.h"
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <algorithm>

I2CSmbus::I2CSmbus(int adapter, int address)
	: _adapter(adapter),
	_address(address),
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

bool I2CSmbus::Initialize()
{
	char filename[20];

	snprintf(filename, 19, "/dev/i2c-%d", _adapter);
	_i2cHandle = open(filename, O_RDWR | O_NONBLOCK);
	check(_i2cHandle);
	
    ioctl(_i2cHandle, I2C_SLAVE, _address);
	check(_i2cHandle);

	return true;
}

void I2CSmbus::Close()
{
	close(_i2cHandle);
}

Status I2CSmbus::Send(byte cmd, byte value)
{
    auto result = i2c_smbus_write_byte_data(_i2cHandle, cmd, value);
	checkStatus(result, Status::SendFail);
	
	return Status::Ok;
}

Status I2CSmbus::Send(byte cmd, byte* data, int dataLen)
{
	int result;
	
	if (dataLen <= I2C_SMBUS_BLOCK_MAX)
	{
		result = i2c_smbus_write_i2c_block_data(_i2cHandle,
            cmd,
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
                cmd + suboffset,
				std::min(I2C_SMBUS_BLOCK_MAX, dataLen - suboffset),
				data + suboffset);
			checkStatus(result, Status::SendFail);

			suboffset += I2C_SMBUS_BLOCK_MAX;
		}
	}
	
	return Status::Ok;
}

Status I2CSmbus::Receive(byte cmd, byte& value)
{
    auto result = i2c_smbus_read_byte_data(_i2cHandle, cmd);
	checkStatus(result, Status::ReceiveFail);
	
	value = byte(result);
	
	return Status::Ok;
}

Status I2CSmbus::Receive(byte cmd, byte* data, int dataLen)
{
	int result;	

	if (dataLen <= I2C_SMBUS_BLOCK_MAX)
	{
		result = i2c_smbus_read_i2c_block_data(_i2cHandle,
            cmd,
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
                cmd + readCount,
				std::min(I2C_SMBUS_BLOCK_MAX, dataLen - readCount),
				data + readCount);
			checkStatus(result, Status::ReceiveFail);

			readCount += I2C_SMBUS_BLOCK_MAX;
		}
	}
	
	return Status::Ok;
}

Status I2CSmbus::Send(byte value)
{
    auto result = i2c_smbus_write_byte(_i2cHandle, value);
    checkStatus(result, Status::SendFail);

    return Status::Ok;
}

Status I2CSmbus::Send(byte cmd, word value)
{
    auto result = i2c_smbus_write_word_data(_i2cHandle, cmd, value);
    checkStatus(result, Status::SendFail);

    return Status::Ok;
}

Status I2CSmbus::Receive(byte& value)
{
    auto result = i2c_smbus_read_byte(_i2cHandle);
    checkStatus(result, Status::SendFail);

    value = byte(result & 0x000000FF);

    return Status::Ok;
}

Status I2CSmbus::Receive(byte cmd, word& value)
{
    auto result = i2c_smbus_read_word_data(_i2cHandle, cmd);

    checkStatus(result, Status::SendFail);

    value = word(result & 0x0000FFFF);

    return Status::Ok;
}
