#if !defined(DS3231REALTIMECLOCK_H)
#define DS3231REALTIMECLOCK_H

#include "LocalTypes.h"
#include "RealTimeClock.h"

class I2CBus;

namespace rtc
{
    class DS3231RealTimeClock : public RealTimeClock
	{
		I2CBus* _bus;
	
	public:
        DS3231RealTimeClock(I2CBus* bus);
	
        ~DS3231RealTimeClock();
	
        bool Initialize();
        void Shutdown();
		
        RtcClockState GetClockState() const;
		
        void GetDateTime(RtcDateTime& value) const;

        void SetDateTime(const RtcDateTime& value);
		
		// Temperature function

        float GetTemperature() const;

		// Alarm functions

        void SetAlarm(AlarmId id, const RtcAlarm& alarm);
        void GetAlarm(AlarmId id, RtcAlarm& alarm) const;
		
        void SetAlarmInterruptState(AlarmInterruptState firstAlarmState, AlarmInterruptState secondAlarmState);
        AlarmInterruptStatus GetAlarmInterruptStatus() const;
		
        AlarmTriggerState GetAlarmTriggerState() const;

    private:
	};
} //namespace rtc

#endif //DS3231REALTIMECLOCK_H


