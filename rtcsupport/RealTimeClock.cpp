#include "RealTimeClock.h"
#include "RealTimeClockPrivate.h"
#include "I2CBus.h"
#include "I2CTransaction.h"
#include "logsupport.h"
#include "DS3231RegisterSet.h"

using namespace rtc;
using namespace google;


bool RealTimeClock::Initialize() const
{
	return _bus->Initialize();
}

void RealTimeClock::Shutdown() const
{
	_bus->Close();
}

RtcClockState RealTimeClock::GetClockState() const
{
    DS3231RegisterSet r;
    DS3231RegisterSet registers;

    if (registers.Read(_bus, DS3231RegisterId::Control) != Status::Ok)
	{
		return RtcClockState::Unknown;
	}
	
    auto state = registers.IsBitSet(DS3231RegisterId::Status, DS3231RegisterMasks::StatusOscStopped)
		? RtcClockState::TimeInvalid
		: RtcClockState::Running;
	
	return state;
}

void RealTimeClock::GetDateTime(RtcDateTime& value) const
{
    DS3231RegisterSet registers;
	
    if (registers.Read(_bus, DS3231RegisterId::Seconds, 7) != Status::Ok)
	{
		return;
	}
	
    value.Time.Second = registers.FromBcd(DS3231RegisterId::Seconds, DS3231RegisterMasks::SecondsMsb, DS3231RegisterMasks::SecondsLsb);
    value.Time.Minute = registers.FromBcd(DS3231RegisterId::Minutes, DS3231RegisterMasks::MinutesMsb, DS3231RegisterMasks::MinutesLsb);
	
    value.Time.Mode = registers.IsBitSet(DS3231RegisterId::Hours, DS3231RegisterMasks::HourMode)
		? ClockMode::WallClock
		: ClockMode::MilitaryClock;
	
	value.Time.Period = Meridiem::None;
	if (value.Time.Mode == ClockMode::WallClock)
	{
        value.Time.Period = registers.IsBitSet(DS3231RegisterId::Hours, DS3231RegisterMasks::HourPeriod)
			? Meridiem::Pm
			: Meridiem::Am;
        value.Time.Hour = registers.FromBcd(DS3231RegisterId::Hours, DS3231RegisterMasks::HourWallMsb, DS3231RegisterMasks::HourLsb);
	}
	else
	{
        value.Time.Hour = registers.FromBcd(DS3231RegisterId::Hours, DS3231RegisterMasks::HourLinearMsb, DS3231RegisterMasks::HourLsb);
	}	
	
	
    value.WeekDay = pvt::ToDayOfWeek(registers.Masked(DS3231RegisterId::WeekDay, DS3231RegisterMasks::WeekDay));
    value.Day = registers.FromBcd(DS3231RegisterId::Date, DS3231RegisterMasks::DateMsb, DS3231RegisterMasks::DateLsb);
    value.Month = pvt::ToMonth(registers.FromBcd(DS3231RegisterId::Month, DS3231RegisterMasks::MonthMsb, DS3231RegisterMasks::MonthLsb));
    value.Year = registers.FromBcd(DS3231RegisterId::Year, DS3231RegisterMasks::YearMsb, DS3231RegisterMasks::YearLsb);
	
    value.Year += registers.IsBitSet(DS3231RegisterId::Month, DS3231RegisterMasks::MonthCentury)
		? 2000
		: 1900;
}

void RealTimeClock::SetDateTime(const RtcDateTime& value) const
{
    DS3231RegisterSet registers;
	
    registers.ToBcd(DS3231RegisterId::Seconds, value.Time.Second, DS3231RegisterMasks::SecondsMsb, DS3231RegisterMasks::SecondsLsb);
    registers.ToBcd(DS3231RegisterId::Minutes, value.Time.Minute, DS3231RegisterMasks::MinutesMsb, DS3231RegisterMasks::MinutesLsb);
	
	if (value.Time.Mode == ClockMode::WallClock)
	{
        registers.ApplyBit(DS3231RegisterId::Hours, DS3231RegisterMasks::HourMode);
		if (value.Time.Period == Meridiem::Pm)
		{
            registers.ApplyBit(DS3231RegisterId::Hours, DS3231RegisterMasks::HourPeriod);
		}
		
        registers.ToBcd(DS3231RegisterId::Hours, value.Time.Hour, DS3231RegisterMasks::HourWallMsb, DS3231RegisterMasks::HourLsb);
	}
    else
    {
        registers.ToBcd(DS3231RegisterId::Hours, value.Time.Hour, DS3231RegisterMasks::HourLinearMsb, DS3231RegisterMasks::HourLsb);
    }

    registers[DS3231RegisterId::WeekDay] = pvt::FromDayOfWeek(value.WeekDay);
    registers.ToBcd(DS3231RegisterId::Date, value.Day, DS3231RegisterMasks::DateMsb, DS3231RegisterMasks::DateLsb);

    auto year = value.Year;
    if (year >= 2000)
    {
        year -= 2000;
        registers.ApplyBit(DS3231RegisterId::Month, DS3231RegisterMasks::MonthCentury);
    }
    else
    {
        year -= 1900;
    }

    registers.ToBcd(DS3231RegisterId::Month, pvt::FromMonth(value.Month), DS3231RegisterMasks::MonthMsb, DS3231RegisterMasks::MonthLsb);

    registers.ToBcd(DS3231RegisterId::Year, year, DS3231RegisterMasks::YearMsb, DS3231RegisterMasks::YearLsb);

    registers.Write(_bus, DS3231RegisterId::Seconds, DS3231RegisterId::Year);

    registers.Read(_bus, DS3231RegisterId::Control, DS3231RegisterId::Status);

    auto oscDisabledOnBattery = registers.IsBitSet(DS3231RegisterId::Control, DS3231RegisterMasks::ControlDisableOscBat);
    auto oscStopped = registers.IsBitSet(DS3231RegisterId::Status, DS3231RegisterMasks::StatusOscStopped);
	
	//if the clock time was invalid, clear the invalid state
    if (oscStopped)
	{
        registers.RemoveBit(DS3231RegisterId::Status, DS3231RegisterMasks::StatusOscStopped);
        registers.Write(_bus, DS3231RegisterId::Status);
	}
	
    if (oscDisabledOnBattery)
	{
        registers.RemoveBit(DS3231RegisterId::Control, DS3231RegisterMasks::ControlDisableOscBat);
        registers.Write(_bus, DS3231RegisterId::Control);
	}
}

float RealTimeClock::GetTemperature() const
{
    DS3231RegisterSet registers;

    registers.Read(_bus, DS3231RegisterId::TempMsb, DS3231RegisterId::TempLsb);
	
    auto tempLsb = registers[DS3231RegisterId::TempLsb] >> 6;
    auto tempMsb = registers.Masked(DS3231RegisterId::TempMsb, DS3231RegisterMasks::TempMsb);
    auto isNegative = registers.IsBitSet(DS3231RegisterId::TempMsb, DS3231RegisterMasks::TempSign);
	
    if (isNegative)
	{
		tempMsb = ~tempMsb + 1;
	}
	
	auto temp = float(tempMsb) + (0.25 * tempLsb);
    if (isNegative)
	{
		temp = -temp;
	}
	
	return temp;
}

//determine if the AxM bit should be set
//in alarm value registers.
//NOTE: the AxM bits are active low, which means the AxM bit
//      should be set if the alarm value should NOT be included
//      in the match
bool ExcludeInterval(AlarmInterval target, AlarmInterval candidate)
{
    return target < candidate;
}


void SetAlarmValue(DS3231RegisterSet& registers, DS3231RegisterId reg, AlarmInterval intervals, AlarmInterval candidate,
                      byte value, byte msbMask, byte lsbMask)
{
    if (ExcludeInterval(intervals, candidate))
    {
        registers.ApplyBit(reg, DS3231RegisterMasks::AlarmExcludeInterval);
    }
    else
    {
        registers.ToBcd(reg, value, msbMask, lsbMask);
    }
}

void RealTimeClock::SetAlarm(AlarmId id, const RtcAlarm& alarm) const
{
    DS3231RegisterSet registers;

    DS3231RegisterId minuteRegister;
    DS3231RegisterId hourRegister;
    DS3231RegisterId dateRegister;

    if (id == AlarmId::First)
    {
        minuteRegister = DS3231RegisterId::Alarm1Minutes;
        hourRegister = DS3231RegisterId::Alarm1Hours;
        dateRegister = DS3231RegisterId::Alarm1Day;

        SetAlarmValue(registers, DS3231RegisterId::Alarm1Seconds, alarm.Interval, AlarmInterval::Seconds,
                 alarm.Second, DS3231RegisterMasks::Alarm1SecondsLsb, DS3231RegisterMasks::Alarm1SecondsMsb);
    }
    else
    {
        minuteRegister = DS3231RegisterId::Alarm2Minutes;
        hourRegister = DS3231RegisterId::Alarm2Hours;
        dateRegister = DS3231RegisterId::Alarm2Day;
    }

    SetAlarmValue(registers, minuteRegister, alarm.Interval, AlarmInterval::Minutes,
             alarm.Minute, DS3231RegisterMasks::AlarmMinutesMsb, DS3231RegisterMasks::AlarmMinutesLsb);

    if (ExcludeInterval(alarm.Interval, AlarmInterval::Hours))
    {
        registers.ApplyBit(hourRegister, DS3231RegisterMasks::AlarmExcludeInterval);
    }
    else
    {
        if (alarm.Mode == ClockMode::WallClock)
        {
            registers.ApplyBit(hourRegister, DS3231RegisterMasks::AlarmHourMode);
            if (alarm.Period == Meridiem::Pm)
            {
                registers.ApplyBit(hourRegister, DS3231RegisterMasks::AlarmHourPeriod);
            }

            registers.ToBcd(hourRegister, alarm.Hour, DS3231RegisterMasks::HourWallMsb, DS3231RegisterMasks::HourLsb);
        }
        else
        {
            registers.ToBcd(hourRegister, alarm.Hour, DS3231RegisterMasks::HourLinearMsb, DS3231RegisterMasks::HourLsb);
        }
    }

    const AlarmInterval DowOrDay = AlarmInterval::Days;

    if (ExcludeInterval(alarm.Interval, DowOrDay))
    {
        registers.ApplyBit(dateRegister, DS3231RegisterMasks::AlarmExcludeInterval);
    }
    else
    {
        if (alarm.Interval == AlarmInterval::WeekDay)
        {
            registers[dateRegister] = pvt::FromDayOfWeek(alarm.Weekday);
            registers.ApplyBit(dateRegister, DS3231RegisterMasks::AlarmDateMode);
        }
        else
        {
            registers.ToBcd(dateRegister, alarm.Day, DS3231RegisterMasks::AlarmDateMsb, DS3231RegisterMasks::AlarmDateLsb);
        }
    }

    if (id == AlarmId::First)
    {
        registers.Write(_bus, DS3231RegisterId::Alarm1Seconds, DS3231RegisterId::Alarm1Day);
    }
    else
    {
        registers.Write(_bus, DS3231RegisterId::Alarm2Minutes, DS3231RegisterId::Alarm2Day);
    }
}

static AlarmInterval DetermineInterval(const DS3231RegisterSet& registers, AlarmId id,
                                       DS3231RegisterId minuteReg, DS3231RegisterId hourReg, DS3231RegisterId dateReg)
{
    auto interval = AlarmInterval::EverySecond;

    if (id == AlarmId::First &&
            registers.IsBitClear(DS3231RegisterId::Alarm1Seconds, DS3231RegisterMasks::AlarmExcludeInterval))
    {
        interval = AlarmInterval::Seconds;
    }

    if (registers.IsBitClear(minuteReg, DS3231RegisterMasks::AlarmExcludeInterval))
    {
        interval = AlarmInterval::Minutes;
    }

    if (registers.IsBitClear(hourReg, DS3231RegisterMasks::AlarmExcludeInterval))
    {
        interval = AlarmInterval::Hours;
    }

    if (registers.IsBitClear(dateReg, DS3231RegisterMasks::AlarmExcludeInterval))
    {
        interval = AlarmInterval::Days;
    }

    return interval;
}

void RealTimeClock::GetAlarm(AlarmId id, RtcAlarm& alarm) const
{
    DS3231RegisterSet registers;

    DS3231RegisterId minuteRegister;
    DS3231RegisterId hourRegister;
    DS3231RegisterId dateRegister;

    if (id == AlarmId::First)
    {
        minuteRegister = DS3231RegisterId::Alarm1Minutes;
        hourRegister = DS3231RegisterId::Alarm1Hours;
        dateRegister = DS3231RegisterId::Alarm1Day;

        registers.Read(_bus, DS3231RegisterId::Alarm1Seconds, DS3231RegisterId::Alarm1Day);

        if (alarm.Interval >= AlarmInterval::Seconds)
        {
            alarm.Second = registers.FromBcd(DS3231RegisterId::Alarm1Seconds, DS3231RegisterMasks::Alarm1SecondsMsb, DS3231RegisterMasks::Alarm1SecondsLsb);
        }
    }
    else
    {
        minuteRegister = DS3231RegisterId::Alarm2Minutes;
        hourRegister = DS3231RegisterId::Alarm2Hours;
        dateRegister = DS3231RegisterId::Alarm2Day;

        registers.Read(_bus, DS3231RegisterId::Alarm2Minutes, DS3231RegisterId::Alarm2Day);
    }

    alarm.Interval = DetermineInterval(registers, id, minuteRegister, hourRegister, dateRegister);

    if (alarm.Interval >= AlarmInterval::Minutes)
    {
        alarm.Minute = registers.FromBcd(minuteRegister, DS3231RegisterMasks::AlarmMinutesMsb, DS3231RegisterMasks::AlarmMinutesLsb);
    }

    if (alarm.Interval >= AlarmInterval::Hours)
    {
        alarm.Mode = registers.IsBitSet(hourRegister, DS3231RegisterMasks::AlarmHourMode)
                ? ClockMode::WallClock
                : ClockMode::MilitaryClock;

        if (alarm.Mode == ClockMode::WallClock)
        {
            alarm.Period = registers.IsBitSet(hourRegister, DS3231RegisterMasks::AlarmHourPeriod)
                    ? Meridiem::Pm
                    : Meridiem::Am;
            alarm.Hour = registers.FromBcd(hourRegister, DS3231RegisterMasks::AlarmHourWallMsb, DS3231RegisterMasks::AlarmHourLsb);
        }
        else
        {
            alarm.Hour = registers.FromBcd(hourRegister, DS3231RegisterMasks::AlarmHourLinearMsb, DS3231RegisterMasks::AlarmHourLsb);
        }
    }

    const AlarmInterval DowOrDay = AlarmInterval::Days;

    if (alarm.Interval == DowOrDay)
    {
        alarm.Interval = registers.IsBitSet(dateRegister, DS3231RegisterMasks::AlarmDateMode)
                ? AlarmInterval::WeekDay
                : AlarmInterval::Days;

        if (alarm.Interval == AlarmInterval::Days)
        {
            alarm.Day = registers.FromBcd(dateRegister, DS3231RegisterMasks::AlarmDateMsb, DS3231RegisterMasks::AlarmDateLsb);
        }
        else
        {
            alarm.Weekday = pvt::ToDayOfWeek(registers.Masked(dateRegister, DS3231RegisterMasks::AlarmWeekDay));
        }
    }
}

void RealTimeClock::SetAlarmInterruptState(AlarmInterruptState firstAlarmState, AlarmInterruptState secondAlarmState) const
{
    DS3231RegisterSet registers;

    registers.Read(_bus, DS3231RegisterId::Control);

    registers.SetBit(DS3231RegisterId::Control, firstAlarmState == AlarmInterruptState::On, DS3231RegisterMasks::ControlAlarm1IntEnable);
    registers.SetBit(DS3231RegisterId::Control, secondAlarmState == AlarmInterruptState::On, DS3231RegisterMasks::ControlAlarm2IntEnable);

    registers.Write(_bus, DS3231RegisterId::Control);
}

AlarmInterruptStatus RealTimeClock::GetAlarmInterruptStatus() const
{
    DS3231RegisterSet registers;
    registers.Read(_bus, DS3231RegisterId::Control);

    AlarmInterruptStatus state;
    state.FirstAlarm = registers.IsBitSet(DS3231RegisterId::Control, DS3231RegisterMasks::ControlAlarm1IntEnable)
            ? AlarmInterruptState::On
            : AlarmInterruptState::Off;

    state.SecondAlarm = registers.IsBitSet(DS3231RegisterId::Control, DS3231RegisterMasks::ControlAlarm2IntEnable)
            ? AlarmInterruptState::On
            : AlarmInterruptState::Off;

    return state;
}

AlarmTriggerState RealTimeClock::GetAlarmTriggerState() const
{
    DS3231RegisterSet registers;

    registers.Read(_bus, DS3231RegisterId::Status);

	AlarmTriggerState state;
    state.FirstTriggered = registers.IsBitSet(DS3231RegisterId::Status, DS3231RegisterMasks::StatusAlarm1Trigger);
    state.SecondTriggered = registers.IsBitSet(DS3231RegisterId::Status, DS3231RegisterMasks::StatusAlarm2Trigger);
	
	if (state.Triggered())
	{
		//clear the alarm status bits if an alarm was triggered

        registers.RemoveBit(DS3231RegisterId::Status, DS3231RegisterMasks::StatusAlarm1Trigger);
        registers.RemoveBit(DS3231RegisterId::Status, DS3231RegisterMasks::StatusAlarm2Trigger);

        registers.Write(_bus, DS3231RegisterId::Status);
	}
	
    return state;
}
