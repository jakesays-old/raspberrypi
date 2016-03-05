#ifndef DS3231RTCREGISTERS_H
#define DS3231RTCREGISTERS_H

#include "LocalTypes.h"

enum class DS3231RegisterId
{
    //Date/time
    Seconds = 0x00,
    Minutes = 0x01,
    Hours = 0x02,
    WeekDay = 0x03,
    Date = 0x04,
    Month = 0x05,
    Year = 0x06,

        //Alarm 1
    Alarm1Seconds = 0x07,
    Alarm1Minutes = 0x08,
    Alarm1Hours = 0x09,
    Alarm1Day = 0x0A,

        //Alarm 2
    Alarm2Minutes = 0x0B,
    Alarm2Hours = 0x0C,
    Alarm2Day = 0x0D,

        //Special purpose
    Control = 0x0E,
    Status = 0x0F,

    AgingOffset = 0x10,
    TempMsb = 0x11,
    TempLsb = 0x12,

    RegisterCount = 0x13,
    Invalid = 0xFF
};

struct DS3231RegisterMasks
{
    static const byte SecondsMsb                 = 0b01110000;
    static const byte SecondsLsb                 = 0b00001111;

    static const byte MinutesMsb                 = 0b01110000;
    static const byte MinutesLsb                 = 0b00001111;

    static const byte HourMode                   = 0b01000000;
    static const byte HourPeriod                 = 0b00100000;
    static const byte HourLinearMsb              = 0b00110000;
    static const byte HourWallMsb                = 0b00010000;
    static const byte HourLsb                    = 0b00001111;

    static const byte WeekDay                    = 0b00000111;

    static const byte DateMsb                    = 0b00110000;
    static const byte DateLsb                    = 0b00001111;

    static const byte MonthCentury               = 0b10000000;
    static const byte MonthMsb                   = 0b00010000;
    static const byte MonthLsb                   = 0b00001111;

    static const byte YearMsb                    = 0b11110000;
    static const byte YearLsb                    = 0b00001111;

    static const byte Alarm1SecondsMsb           = 0b01110000;
    static const byte Alarm1SecondsLsb           = 0b00001111;

    static const byte AlarmExcludeInterval      = 0b10000000;

    static const byte AlarmMinutesMsb           = 0b01110000;
    static const byte AlarmMinutesLsb           = 0b00001111;

    static const byte AlarmHourMode             = 0b01000000;
    static const byte AlarmHourPeriod           = 0b00100000;
    static const byte AlarmHourLinearMsb        = 0b00110000;
    static const byte AlarmHourWallMsb          = 0b00010000;
    static const byte AlarmHourLsb              = 0b00001111;

    static const byte AlarmDateMode             = 0b01000000;
    static const byte AlarmDateMsb              = 0b00110000;
    static const byte AlarmDateLsb              = 0b00001111;
    static const byte AlarmWeekDay              = 0b00000111;

    static const byte ControlDisableOscBat       = 0b10000000;
    static const byte ControlBatterySqWaveEnable = 0b01000000;
    static const byte ControlTempConv            = 0b00100000;
    static const byte ControlSqFreq              = 0b00011000;
    static const byte ControlInterruptCtl        = 0b00000100;
    static const byte ControlAlarm2IntEnable     = 0b00000010;
    static const byte ControlAlarm1IntEnable     = 0b00000001;

    static const byte StatusOscStopped           = 0b10000000;
    static const byte StatusEnable32kOutput      = 0b00001000;
    static const byte StatusTempConvBusy         = 0b00000100;
    static const byte StatusAlarm2Trigger        = 0b00000010;
    static const byte StatusAlarm1Trigger        = 0b00000001;

    static const byte AgingOffsetSign            = 0b10000000;
    static const byte AgingOffsetData            = 0b01111111;

    static const byte TempSign                   = 0b10000000;
    static const byte TempMsb                    = 0b01110000;
    static const byte TempLsb                    = 0b00001111;
};


#endif // DS3231RTCREGISTERS_H

