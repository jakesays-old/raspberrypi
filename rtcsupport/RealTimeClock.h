#if !defined(REALTIMECLOCK_H)
#define REALTIMECLOCK_H

#include "LocalTypes.h"
#include "RtcTime.h"
#include "RtcAlarm.h"

class I2CBus;

namespace rtc
{
	enum class RtcClockState
	{
		Unknown,
		Running,
		TimeInvalid
	};
	
	class RealTimeClock
	{
    public:
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;
		
        virtual RtcClockState GetClockState() const = 0;
		
        virtual void GetDateTime(RtcDateTime& value) const = 0;
        virtual void SetDateTime(const RtcDateTime& value) = 0;
		
		// Temperature function

        virtual float GetTemperature() const = 0;

		// Alarm functions

        virtual void SetAlarm(AlarmId id, const RtcAlarm& alarm) = 0;
        virtual void GetAlarm(AlarmId id, RtcAlarm& alarm) const = 0;
		
        virtual void SetAlarmInterruptState(AlarmInterruptState firstAlarmState, AlarmInterruptState secondAlarmState) = 0;
        virtual AlarmInterruptStatus GetAlarmInterruptStatus() const = 0;
		
        virtual AlarmTriggerState GetAlarmTriggerState() const = 0;
	};
} //namespace rtc

#endif //REALTIMECLOCK_H


