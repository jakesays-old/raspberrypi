#include "RtcDebugger.h"
#include "RealTimeClockPrivate.h"
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>

using namespace std;
using namespace rtc;

static const char* RegisterName(DS3231RegisterId id);

static byte Mask(byte data, byte mask)
{
    return data & mask;
}

static bool TestBit(byte data, byte mask)
{
    return (data & mask) == mask;
}

static byte FromBcd(byte data, byte msbMask, byte lsbMask)
{
    auto msb = Mask(data, msbMask);
    auto lsb = Mask(data, lsbMask);

    auto result = lsb + (msb >> 4) * 10;

    return result;
}

void PrintByte(byte b)
{
    printf("%02X: ", b);
    byte mask = 0x80;

    for (; mask != 0; mask >>= 1)
    {
        printf((b & mask) ? "1" : "0");
    }
}

RtcDebugger::RtcDebugger(I2CBus* bus)
    : _bus(bus)
{
}

void RtcDebugger::ShowRegister(DS3231RegisterId reg)
{
    byte data;

    if (_bus->Receive((int)reg, data) != Status::Ok)
    {
        std::cout << "Cannot read register value" << std::endl;
        return;
    }

    printf("%s: ", RegisterName(reg));
    PrintByte(data);
    printf(" ");

    switch (reg)
    {
        case DS3231RegisterId::Seconds:
            ShowSeconds(data);
            break;
        case DS3231RegisterId::Minutes:
            ShowMinutes(data);
            break;
        case DS3231RegisterId::Hours:
            ShowHours(data);
            break;
        case DS3231RegisterId::WeekDay:
            ShowWeekDay(data);
            break;
        case DS3231RegisterId::Date:
            ShowDate(data);
            break;
        case DS3231RegisterId::Month:
            ShowMonth(data);
            break;
        case DS3231RegisterId::Year:
            ShowYear(data);
            break;
        case DS3231RegisterId::Alarm1Seconds:
            ShowAlarm1Seconds(data);
            break;
        case DS3231RegisterId::Alarm1Minutes:
            ShowAlarm1Minutes(data);
            break;
        case DS3231RegisterId::Alarm1Hours:
            ShowAlarm1Hours(data);
            break;
        case DS3231RegisterId::Alarm1Day:
            ShowAlarm1Day(data);
            break;
        case DS3231RegisterId::Alarm2Minutes:
            ShowAlarm2Minutes(data);
            break;
        case DS3231RegisterId::Alarm2Hours:
            ShowAlarm2Hours(data);
            break;
        case DS3231RegisterId::Alarm2Day:
            ShowAlarm2Day(data);
            break;
        case DS3231RegisterId::Control:
            ShowControl(data);
            break;
        case DS3231RegisterId::Status:
            ShowStatus(data);
            break;
        case DS3231RegisterId::AgingOffset:
            ShowAgingOffset(data);
            break;
        case DS3231RegisterId::TempMsb:
            ShowTempMsb(data);
            break;
        case DS3231RegisterId::TempLsb:
            ShowTempLsb(data);
            break;
        case DS3231RegisterId::RegisterCount:
            ShowRegisterCount(data);
            break;
        case DS3231RegisterId::Invalid:
            ShowInvalid(data);
            break;
    }

    printf("\n");
}

void RtcDebugger::ShowSeconds(byte data)
{
    auto sec = FromBcd(data, DS3231RegisterMasks::SecondsMsb, DS3231RegisterMasks::SecondsLsb);
    printf("%d", sec);
}

void RtcDebugger::ShowMinutes(byte data)
{
    auto min = FromBcd(data, DS3231RegisterMasks::MinutesMsb, DS3231RegisterMasks::MinutesLsb);
    printf("%d", min);
}

void RtcDebugger::ShowHours(byte data)
{
    auto is12hour = TestBit(data, DS3231RegisterMasks::HourMode);

    if (is12hour)
    {
        auto isPm = TestBit(data, DS3231RegisterMasks::HourPeriod);
        auto hour = FromBcd(data, DS3231RegisterMasks::HourWallMsb, DS3231RegisterMasks::HourLsb);
        printf("12hr: %d %s", hour, isPm ? "PM" : "AM");
    }
    else
    {
        auto hour = FromBcd(data, DS3231RegisterMasks::HourLinearMsb, DS3231RegisterMasks::HourLsb);
        printf("24hr: %02d", hour);
    }
}

void RtcDebugger::ShowWeekDay(byte data)
{
    printf("%d", Mask(data, DS3231RegisterMasks::WeekDay));
}

void RtcDebugger::ShowDate(byte data)
{
    printf("%d", FromBcd(data, DS3231RegisterMasks::DateMsb, DS3231RegisterMasks::DateLsb));
}

void RtcDebugger::ShowMonth(byte data)
{
    auto century = TestBit(data, DS3231RegisterMasks::MonthCentury);
    auto month = FromBcd(data, DS3231RegisterMasks::MonthMsb, DS3231RegisterMasks::MonthLsb);

    printf("Century %s, month %d (%s)", century ? "YES" : "NO", month, pvt::MonthName(pvt::ToMonth(month)));
}

void RtcDebugger::ShowYear(byte data)
{
    auto year = FromBcd(data, DS3231RegisterMasks::YearMsb, DS3231RegisterMasks::YearLsb);
    printf("%d", year);
}

void RtcDebugger::ShowAlarm1Seconds(byte data)
{
}

void RtcDebugger::ShowAlarm1Minutes(byte data)
{
}

void RtcDebugger::ShowAlarm1Hours(byte data)
{
}

void RtcDebugger::ShowAlarm1Day(byte data)
{
}

void RtcDebugger::ShowAlarm2Minutes(byte data)
{
}

void RtcDebugger::ShowAlarm2Hours(byte data)
{
}

void RtcDebugger::ShowAlarm2Day(byte data)
{
}

void RtcDebugger::ShowControl(byte data)
{
}

void RtcDebugger::ShowStatus(byte data)
{
}

void RtcDebugger::ShowAgingOffset(byte data)
{
}

void RtcDebugger::ShowTempMsb(byte data)
{
}

void RtcDebugger::ShowTempLsb(byte data)
{
}

void RtcDebugger::ShowRegisterCount(byte data)
{
}

void RtcDebugger::ShowInvalid(byte data)
{
}

static const char* RegisterName(DS3231RegisterId id)
{
    switch (id)
    {
        case DS3231RegisterId::Seconds:
            return "Seconds";
        case DS3231RegisterId::Minutes:
            return "Minutes";
        case DS3231RegisterId::Hours:
            return "Hours";
        case DS3231RegisterId::WeekDay:
            return "WeekDay";
        case DS3231RegisterId::Date:
            return "Date";
        case DS3231RegisterId::Month:
            return "Month";
        case DS3231RegisterId::Year:
            return "Year";
        case DS3231RegisterId::Alarm1Seconds:
            return "Alarm1Seconds";
        case DS3231RegisterId::Alarm1Minutes:
            return "Alarm1Minutes";
        case DS3231RegisterId::Alarm1Hours:
            return "Alarm1Hours";
        case DS3231RegisterId::Alarm1Day:
            return "Alarm1Day";
        case DS3231RegisterId::Alarm2Minutes:
            return "Alarm2Minutes";
        case DS3231RegisterId::Alarm2Hours:
            return "Alarm2Hours";
        case DS3231RegisterId::Alarm2Day:
            return "Alarm2Day";
        case DS3231RegisterId::Control:
            return "Control";
        case DS3231RegisterId::Status:
            return "Status";
        case DS3231RegisterId::AgingOffset:
            return "AgingOffset";
        case DS3231RegisterId::TempMsb:
            return "TempMsb";
        case DS3231RegisterId::TempLsb:
            return "TempLsb";
        case DS3231RegisterId::RegisterCount:
            return "RegisterCount";
        default:
            return "Invalid";
    }
}
