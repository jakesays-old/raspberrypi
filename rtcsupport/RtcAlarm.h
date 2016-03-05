#ifndef RTCALARM_H
#define RTCALARM_H

#include "RtcTime.h"
#include "LocalTypes.h"

namespace rtc
{
	enum class AlarmId
	{
		First,
		Second
	};
	
    enum class AlarmInterruptState
	{
		On,		
		Off		
	};
	
    struct AlarmInterruptStatus
    {
        AlarmInterruptState FirstAlarm;
        AlarmInterruptState SecondAlarm;
    };

	enum class AlarmInterval
	{
        EverySecond,
        Seconds,
        Minutes,
        Hours,
        WeekDay,
        Days,

        IntervalCount
    };

	
	//Identifies which alarms have been triggered
	struct AlarmTriggerState
	{
		bool FirstTriggered;
		bool SecondTriggered;
		
		bool Triggered()
		{
			return FirstTriggered || SecondTriggered;
		}
	};
	
	struct RtcAlarm : RtcTime
	{
		int Day;
		DayOfWeek Weekday;
        AlarmInterruptState State;
		AlarmInterval Interval;
		
		RtcAlarm() :
			RtcTime(),
			Day(0),
			Weekday(DayOfWeek::None),
            State(AlarmInterruptState::Off),
            Interval(AlarmInterval::Minutes)
		{
		}
		
		RtcAlarm(Meridiem period,
			int hour,
			int minute,
			int second,
			int day,
			AlarmInterval interval,
			DayOfWeek weekDay = DayOfWeek::None,
			ClockMode mode = ClockMode::WallClock,
            AlarmInterruptState state = AlarmInterruptState::Off) :
			RtcTime(period, hour, minute, second, mode),
			Day(day),
			Weekday(weekDay),
			State(state),
			Interval(interval)
		{
		}
		
		RtcAlarm(const RtcAlarm& other) :
			RtcTime(other),			
			Day(other.Day),			
			Weekday(other.Weekday),
			State(other.State),
			Interval(other.Interval)
		{
		}
	};
} //namespace rtc

#endif //RTCALARM_H
