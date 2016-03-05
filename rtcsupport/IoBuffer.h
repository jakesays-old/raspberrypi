#ifndef IOBUFFER_H
#define IOBUFFER_H

#include "LocalTypes.h"
#include <string>
#include <cstring>
#include <exception>

#include "IoError.h"

class IoBuffer
{
	byte* _buffer;
	byte* _bufferEnd;
	byte* _readPtr;
	byte* _writePtr;
	
	int _capacity;

public:
	IoBuffer(int capacity) :
		_capacity(capacity)
	{
		_buffer = new byte[capacity];
		_bufferEnd = _buffer + capacity;
		_readPtr = _buffer;
		_writePtr = _buffer;
	}
	
	~IoBuffer()
	{
		delete _buffer;
	}
	
	byte* Data() const
	{
		return _buffer;
	}
	
	void Reset()
	{
		_readPtr = _buffer;
		_writePtr = _buffer;
	}
	
	void Clear()
	{
		Reset();
		std::memset(_buffer, '\0', _capacity);
	}
	
	int ReadLength() const
	{
		return _readPtr - _buffer;
	}
	
	int ReadRemaining() const
	{
		return _writePtr - _readPtr;
	}
	
	int WriteLength() const
	{
		return  _writePtr - _buffer;
	}
	
	void SetWriteLength(int length)
	{
		_writePtr = _buffer + length;
	}
	
	int WriteRemaining() const
	{
		return _bufferEnd - _writePtr;
	}
	
	bool Eor() const
	{
		return _readPtr >= _writePtr;
	}
	
	bool Eow() const
	{
		return _writePtr >= _bufferEnd;
	}
	
	byte Read8()
	{
		if (!Eor())
		{
			auto value = *_readPtr;
			_readPtr += 1;
			return value;
		}

		throw IoError(Status::ReadUnderflow);
	}
	
	void Read(byte* dest, int destLength)
	{
		auto bytesRead = ReadLength();
		if (bytesRead < destLength)
		{
			throw IoError(Status::ReadUnderflow);
		}
		
		if (bytesRead > 0)
		{
			std::memcpy(static_cast<void*>(dest), static_cast<void*>(_readPtr), bytesRead);
		}
	}
	
	void Write(byte value)
	{
		if (Eow())
		{
			throw IoError(Status::WriteOverflow);
		}
		
		*_writePtr = value;
		_writePtr += 1;
	}
	
	void Write(byte* data, int dataLength)
	{
		if (dataLength > WriteLength())
		{
			throw IoError(Status::WriteOverflow);
		}
		
		std::memcpy(static_cast<void*>(_writePtr), static_cast<void*>(data), dataLength);
		
		_writePtr += dataLength;
	}
	
	std::string AsString() const;
};

#endif //IOBUFFER_H
