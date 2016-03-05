#ifndef I2CSOFTWAREBUS_H
#define I2CSOFTWAREBUS_H

#include "I2CBuffer.h"
#include "I2CGpioBus.h"

class I2CGpioSoftwareBus : public I2CGpioBus
{
	int _address;
	int _sdaPin;
	int _sclPin;
	int _baudRate;
	
	I2CBuffer _receiveBuffer;
	I2CBuffer _sendBuffer;

public:
    I2CGpioSoftwareBus(int address, int sdaPin, int sclPin, int baudRate,
		int receiveBufferSize = 1024, int sendBufferSize = 1024);
	
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

private:
    Status Transmit(int readCount = 0);
    void Prepare();
};

#endif //I2CSOFTWAREBUS_H
