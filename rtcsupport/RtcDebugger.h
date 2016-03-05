#ifndef RTCDEBUGGER_H
#define RTCDEBUGGER_H

#include "I2CBus.h"
#include "DS3231Registers.h"

class RtcDebugger
{
    I2CBus* _bus;

public:
    RtcDebugger(I2CBus* bus);

    void ShowRegister(DS3231RegisterId reg);

private:
    void ShowSeconds(byte data);
    void ShowMinutes(byte data);
    void ShowHours(byte data);
    void ShowWeekDay(byte data);
    void ShowDate(byte data);
    void ShowMonth(byte data);
    void ShowYear(byte data);
    void ShowAlarm1Seconds(byte data);
    void ShowAlarm1Minutes(byte data);
    void ShowAlarm1Hours(byte data);
    void ShowAlarm1Day(byte data);
    void ShowAlarm2Minutes(byte data);
    void ShowAlarm2Hours(byte data);
    void ShowAlarm2Day(byte data);
    void ShowControl(byte data);
    void ShowStatus(byte data);
    void ShowAgingOffset(byte data);
    void ShowTempMsb(byte data);
    void ShowTempLsb(byte data);
    void ShowRegisterCount(byte data);
    void ShowInvalid(byte data);
};

#endif // RTCDEBUGGER_H
