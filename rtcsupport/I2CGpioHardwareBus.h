#ifndef I2CHARDWAREBUS_H
#define I2CHARDWAREBUS_H

#include "I2CBuffer.h"
#include "I2CGpioBus.h"

class I2CGpioHardwareBus : public I2CGpioBus
{
    int _adapter;
	int _address;
	
	int _i2cHandle;
	
public:
    I2CGpioHardwareBus(int _adapter, int address);
	
    bool Initialize();
	void Close();
	

    // I2CBus interface
public:
    Status Send(byte value) override;
    Status Send(byte cmd, byte value) override;
    Status Send(byte cmd, byte* data, int dataLen) override;
    Status Send(byte cmd, word value) override;
    Status Receive(byte& value) override;
    Status Receive(byte cmd, byte& value) override;
    Status Receive(byte cmd, byte* data, int dataLen) override;
    Status Receive(byte cmd, word& value) override;
};

#endif //I2CHARDWAREBUS_H
