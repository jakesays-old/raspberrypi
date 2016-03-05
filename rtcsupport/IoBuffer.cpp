#include "IoBuffer.h"
#include <strstream>
#include <iostream>

static char* _hexChars = "0123456789ABCDEF";

std::string IoBuffer::AsString() const
{
	if (WriteLength() == 0)
	{
		return "";
	}
	
	std::strstream txt;
	txt << "[";
	
	auto counter = 0;
	
	for (auto idx = _buffer; idx < _writePtr; idx++)
	{
		if (counter++ > 15)
		{
			txt << std::endl;
			counter = 0;
		}
		
		if (counter > 1)
		{
			txt << " ";
		}
		
		auto c = *idx;
		
		txt << _hexChars[c >> 4];
		txt << _hexChars[c & 0x0F];
	}
	
	txt << "]\0";
	
	return std::string(txt.str(), 0, txt.pcount());
}
