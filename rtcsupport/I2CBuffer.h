#ifndef I2CBUFFER_H
#define I2CBUFFER_H

#include "IoBuffer.h"

class I2CBuffer : public IoBuffer
{
	enum class Commands : byte
	{
		End = 0,
		Escape = 1,
		Start = 2,
		Stop = 3,
		Address = 4,
		Flags = 5,
		Read = 6,
		Write = 7
	};
	

public:
	explicit I2CBuffer(int capacity) :
        IoBuffer(capacity)
	{
	}
	
	void Start()
	{
		Write(byte(Commands::Start));
	}

    //Same command as Start but the semantics are different
    void Restart()
    {
        Write(byte(Commands::Start));
    }

	void Stop()
	{
		Write(byte(Commands::Stop));
	}

	void End()
	{
		Write(byte(Commands::End));
	}

    void StopAndEnd()
    {
        Write(byte(Commands::Stop));
        Write(byte(Commands::End));
    }

	void Escape()
	{
		Write(byte(Commands::Escape));
	}

	void Send(int length)
	{
		if (length > 255)
		{
			Escape();
		}
		
		Write(byte(Commands::Write));
		WriteNumber(length);
	}
	
	void Receive(int receiveLength)
	{
		if (receiveLength > 255)
		{
			Escape();
		}
		
		Write(byte(Commands::Read));
		WriteNumber(receiveLength);
	}
	
	// currently only 127 bit addresses are supported
	void Address(byte address)
	{
		Write(byte(Commands::Address));
		Write(address);
	}
	
private:
	void WriteNumber(int value)
	{
		if (value > 255)
		{
			Write(byte(value & 0xFF));
			Write(byte((value >> 8) & 0xFF));
		}
		else
		{
			Write(byte(value & 0xFF));
		}
	}
};

#endif //I2CBUFFER_H
