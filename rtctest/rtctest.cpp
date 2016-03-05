#include <iostream>
#include <DS3231RealTimeClock.h>
#include <I2CSmbus.h>
#include <glog/logging.h>
#include <RealTimeClockPrivate.h>
#include <RtcDebugger.h>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>

#include <LocalTypes.h>

using namespace std;
using namespace rtc;
using namespace boost::posix_time;

DECLARE_string(set);
DECLARE_bool(am);
DECLARE_bool(pm);
DECLARE_bool(dump);

DEFINE_string(set, std::string(), "Set the date/time. Format YYY-MM-DD hh:mm:ss. Use -am or -pm to indicate a 12 hour clock.");
DEFINE_bool(am, false, "Indicates a 12 hour clock, AM");
DEFINE_bool(pm, false, "Indicates a 12 hour clock, PM");
DEFINE_bool(dump, false, "Dump registers");


I2CSmbus i2cBus(1, 0x68);
DS3231RealTimeClock rtclock(&i2cBus);

RtcDateTime MakeDateTime(ptime time, Meridiem period)
{
	auto d = time.time_of_day();
	
	auto hour = d.hours();
	auto minute = d.minutes();
	auto second = d.seconds();
	auto mode = period == Meridiem::None 
		? ClockMode::MilitaryClock
		: ClockMode::WallClock;
	
	if (period != Meridiem::None)
	{
		if (hour == 0 && minute <= 59)
		{
			hour += 12;
		}
		else if (hour >= 13 && hour <= 23)
		{
			hour -= 12;
		}
	}
	
    auto doy = byte(time.date().day_of_week());
    auto mon = byte(time.date().month());
    auto weekDay = pvt::ToDayOfWeek(doy);
    auto month = pvt::ToMonth(mon);
	auto day = int(time.date().day());
	auto year = int(time.date().year());
	
    printf("y: %d, m: %d, dy: %d, dt: %d, hh: %d, mm: %d, ss: %d\n",
           year, mon, doy, day, hour, minute, second);

	RtcDateTime result(RtcTime(period, hour, minute, second, mode),
		day,
		weekDay,
		month,
		year);
	
    printf("rtc datetime: %s\n", result.AsString().c_str());

	return result;
}

bool SetTime()
{
	if (FLAGS_set.empty())
	{
		return false;
	}
	
	auto time = time_from_string(FLAGS_set);
	if (time.is_not_a_date_time())
	{
		cout << "Invalid date/time format." << endl;
		exit(1);
	}
	
	return true;
}

void dump();

int main(int argc, char *argv[])
{
	google::ParseCommandLineFlags(&argc, &argv, true);	
	
	FLAGS_log_dir = "/home/pi/clock/logs";
	
	google::InitGoogleLogging(argv[0]);

    if (FLAGS_dump)
    {
        dump();
        return 0;
    }

    auto status = i2cBus.Initialize();
    if (!status)
    {
        std::cout << "Could not initialize the I2C bus instance" << std::endl;
        return 1;
    }

    status = rtclock.Initialize();
    if (!status)
    {
        std::cout << "Could not initialize the rtc instance" << std::endl;
        return 1;
    }

	if ((FLAGS_am || FLAGS_pm) &&
		FLAGS_set.empty())
	{
		std::cout << "-am or -pm requires -settime" << std::endl;
		return 1;
	}
	
	if (FLAGS_set.empty())
	{
		RtcDateTime dt;
		rtclock.GetDateTime(dt);
	
		std::cout << dt.AsString(true) << std::endl;
	}
	else
	{
		cout << "set time to: " << FLAGS_set << endl;
		
		auto time = time_from_string(FLAGS_set);
		if (time.is_not_a_date_time())
		{
			cout << "Invalid date/time format." << endl;
		}
		else
		{
			auto period = Meridiem::None;
			if (FLAGS_am)
			{
				period = Meridiem::Am;
			}
			else if (FLAGS_pm)
			{
				period = Meridiem::Pm;
			}
			
			auto rtdatetime = MakeDateTime(time, period);
			rtclock.SetDateTime(rtdatetime);
		}
	}

	rtclock.Shutdown();
	
	return 0;
}

void dump()
{
    I2CSmbus bus(1, 0x68);
    bus.Initialize();

    RtcDebugger dbg(&bus);

    dbg.ShowRegister(DS3231RegisterId::Seconds);
    dbg.ShowRegister(DS3231RegisterId::Minutes);
    dbg.ShowRegister(DS3231RegisterId::Hours);
    dbg.ShowRegister(DS3231RegisterId::WeekDay);
    dbg.ShowRegister(DS3231RegisterId::Date);
    dbg.ShowRegister(DS3231RegisterId::Month);
    dbg.ShowRegister(DS3231RegisterId::Year);

    return;

    dbg.ShowRegister(DS3231RegisterId::Alarm1Seconds);
    dbg.ShowRegister(DS3231RegisterId::Alarm1Minutes);
    dbg.ShowRegister(DS3231RegisterId::Alarm1Hours);
    dbg.ShowRegister(DS3231RegisterId::Alarm1Day);

    dbg.ShowRegister(DS3231RegisterId::Alarm2Minutes);
    dbg.ShowRegister(DS3231RegisterId::Alarm2Hours);
    dbg.ShowRegister(DS3231RegisterId::Alarm2Day);

    dbg.ShowRegister(DS3231RegisterId::Control);
    dbg.ShowRegister(DS3231RegisterId::Status);

    dbg.ShowRegister(DS3231RegisterId::AgingOffset);
    dbg.ShowRegister(DS3231RegisterId::TempMsb);
    dbg.ShowRegister(DS3231RegisterId::TempLsb);
}
