#ifndef DS3231REGISTERSET_H
#define DS3231REGISTERSET_H

#include "I2CBus.h"
#include "LocalTypes.h"
#include "DS3231Registers.h"
#include <stdlib.h>

class DS3231RegisterSet
{
    static const int RegisterCount = byte(DS3231RegisterId::RegisterCount);

    byte _registers[RegisterCount];

public:
    DS3231RegisterSet()
    {
        Clear();
    }

    void Clear()
    {
        memset(static_cast<void*>(_registers), 0, RegisterCount);
    }

    byte* AddressOf(DS3231RegisterId id)
    {
        return ((byte*) _registers) + byte(id);
    }

    byte* OffsetOf(DS3231RegisterId id)
    {
        return &_registers[byte(id)];
    }

    byte operator[](DS3231RegisterId id) const
    {
        return _registers[byte(id)];
    }

    byte& operator[](DS3231RegisterId id)
    {
        return _registers[byte(id)];
    }

    byte Masked(DS3231RegisterId id, byte mask) const
    {
        return _registers[byte(id)] & mask;
    }

    bool IsBitSet(DS3231RegisterId id, byte mask) const
    {
        return (_registers[byte(id)] & mask) == mask;
    }

    bool IsBitClear(DS3231RegisterId id, byte mask) const
    {
        return (_registers[byte(id)] & mask) == 0;
    }

    void SetBit(DS3231RegisterId id, bool value, byte mask)
    {
        //if value is true then set mask
        //otherwise clear mask

        if (value)
        {
            _registers[int(id)] |= mask;
        }
        else
        {
            _registers[int(id)] &= ~mask;
        }
    }

    void ApplyBit(DS3231RegisterId id, byte mask)
    {
        _registers[byte(id)] |= mask;
    }

    void RemoveBit(DS3231RegisterId id, byte mask)
    {
        _registers[byte(id)] &= ~mask;
    }

    byte FromBcd(DS3231RegisterId id, byte msbMask, byte lsbMask) const
    {
        auto msb = Masked(id, msbMask);
        auto lsb = Masked(id, lsbMask);

        auto result = lsb + (msb >> 4) * 10;

        return result;
    }

    void ToBcd(DS3231RegisterId id, byte value, byte msbMask, byte lsbMask)
    {
        auto& r = _registers[byte(id)];

        //((val / 10) << 4) + val % 10;

        r |= byte(value / 10 << 4) & msbMask;
        r |= byte(value % 10) & lsbMask;
    }

    Status Read(I2CBus* bus, DS3231RegisterId id)
    {
        printf("Reading register %d\n", byte(id));

        byte value;
        auto result = bus->Receive((int)id, value);
        if (result == Status::Ok)
        {
            _registers[byte(id)] = value;
        }
        return result;
    }

    void Dump()
    {
        for(int i = 0; i < RegisterCount; i++)
        {
            printf("%02X ", *(_registers + i));
        }
    }

    Status Read(I2CBus* bus, DS3231RegisterId startId, int count)
    {
        if (int(startId) + count >= int(DS3231RegisterId::RegisterCount))
        {
            return Status::ReceiveFail;
        }

        printf("Reading start: %d, count: %d\n", byte(startId), byte(count));

        auto result = bus->Receive((int)startId, _registers + byte(startId), count);

        Dump();
        printf("\n");

        return result;
    }

    Status Read(I2CBus* bus, DS3231RegisterId startRegister, DS3231RegisterId endRegister)
    {
        auto count = (byte(endRegister) - byte(startRegister)) + 1;


        return Read(bus, startRegister, count);
    }

    Status Write(I2CBus* bus, DS3231RegisterId id)
    {
        return bus->Send((int)id, _registers[byte(id)]);
    }

    Status Write(I2CBus* bus, DS3231RegisterId startId, int count)
    {
        if (int(startId) + count >= int(DS3231RegisterId::RegisterCount))
        {
            return Status::SendFail;
        }

        printf("Writing start: %d, count: %d\n", byte(startId), byte(count));

        Dump();
        printf("\n");

        return bus->Send((int)startId, _registers + byte(startId), count);
    }

    Status Write(I2CBus* bus, DS3231RegisterId startRegister, DS3231RegisterId endRegister)
    {
        auto count = byte(endRegister) - byte(startRegister);

        return Write(bus, startRegister, count);
    }
};

#endif // DS3231REGISTERSET_H

