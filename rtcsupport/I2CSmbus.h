#ifndef I2CSMBUS_H
#define I2CSMBUS_H

#include "I2CBus.h"
#include "I2CBuffer.h"

class I2CSmbus : public I2CBus
{
	int _adapter;
	int _address;
	
	int _i2cHandle;

public:
    I2CSmbus(int adapter, int address);
	
	bool Initialize();
	void Close();
	
    // I2CBus interface
public:
    Status Send(byte cmd, byte value) override;
    Status Send(byte cmd, byte* data, int dataLen) override;
    Status Send(byte value) override;
    Status Send(byte cmd, word value) override;

    Status Receive(byte cmd, byte& value) override;
    Status Receive(byte cmd, byte* data, int dataLen) override;
    Status Receive(byte& value) override;
    Status Receive(byte cmd, word& value) override;
};

#endif //I2CSMBUS_H
