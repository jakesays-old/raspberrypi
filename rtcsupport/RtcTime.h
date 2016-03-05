#if !defined(RTCTIME_H)
#define RTCTIME_H

#include <sstream>
#include <string>
#include <iomanip>

namespace rtc
{
	enum class Meridiem
	{
		None,
		Am,
		Pm
	};

	enum class ClockMode
	{
		WallClock,
		MilitaryClock
	};

	struct RtcTime
	{
		ClockMode Mode;
	
		Meridiem Period;
		int Hour;
		int Minute;
		int Second;

		RtcTime()
		{}

		RtcTime(Meridiem period, int hour, int minute, int second, ClockMode mode = ClockMode::WallClock) :
			Mode(mode),
			Period(period),
			Hour(hour),
			Minute(minute),
			Second(second)
		{}
	
		RtcTime(const RtcTime& src) :
			Mode(src.Mode),
			Period(src.Period),
			Hour(src.Hour),
			Minute(src.Minute),
			Second(src.Second)
		{
		}
		
		std::string AsString() const
		{
			std::stringstream txt;
			
			if (Mode == ClockMode::WallClock)
			{
				txt << Hour << ":" << std::setw(2) << std::setfill('0')
					<< Minute << ":" 
					<< std::setw(2) << std::setfill('0') << Second;
				
				switch (Period)
				{
				default: 
					txt << " ?";
					break;
				case Meridiem::Am: 
					txt << " AM";
					break;
				case Meridiem::Pm: 
					txt << " PM";
					break;
				}
			}
			else
			{
				txt << std::setw(2) << std::setfill('0') << Hour << ":" 
					<< std::setw(2) << std::setfill('0') << Minute << ":" 
					<< std::setw(2) << std::setfill('0') << Second;
			}
			
			return txt.str();
		}
	};

	enum class Months
	{
		None,
		Jan,
		Feb,
		Mar,
		Apr,
		May,
		Jun,
		Jul,
		Aug,
		Sep,
		Oct,
		Nov,
		Dec
	};

	enum class DayOfWeek
	{
		None,
		Sun,
		Mon,
		Tue,
		Wed,
		Thu,
		Fri,
		Sat
	};

	
	struct RtcDateTime
	{
		RtcTime Time;
	
		int Day;
		DayOfWeek WeekDay;
		Months Month;
		int Year;
	
		RtcDateTime() :
			Time(),
			Day(0),
			WeekDay(DayOfWeek::None),
			Month(Months::None),
			Year(0)
		{
		}
		
		RtcDateTime(const RtcTime& time, int day = 0, 
			DayOfWeek weekDay = DayOfWeek::None, 
			Months month = Months::None, 
			int year = 0) :
			Time(time),
			Day(day),
			WeekDay(weekDay),
			Month(month),
			Year(year)
		{
		}
	
		RtcDateTime(const RtcDateTime& other) :
			Time(other.Time),
			Day(other.Day),
			WeekDay(other.WeekDay),
			Month(other.Month),
			Year(other.Year)
		{
		}
		
		std::string AsString(bool withWeekDay = false) const
		{
			std::stringstream txt;
						
			txt << Day;
			
			if (withWeekDay)
			{
				switch (WeekDay)
				{
				default:
					txt << " (?)";
					break;
				case DayOfWeek::Sun: 
					txt << " (Sun)";
					break;
				case DayOfWeek::Mon: 
					txt << " (Mon)";
					break;
				case DayOfWeek::Tue: 
					txt << " (Tue)";
					break;
				case DayOfWeek::Wed: 
					txt << " (Wed)";
					break;
				case DayOfWeek::Thu: 
					txt << " (Thu)";
					break;
				case DayOfWeek::Fri: 
					txt << " (Fri)";
					break;
				case DayOfWeek::Sat: 
					txt << " (Sat)";
					break;
				}
			}
			
			switch (Month)
			{
			default: 
				txt << " ?";
				break;
			case Months::Jan: 
				txt << " Jan";
				break;
			case Months::Feb: 
				txt << " Feb";
				break;
			case Months::Mar: 
				txt << " Mar";
				break;
			case Months::Apr: 
				txt << " Apr";
				break;
			case Months::May: 
				txt << " May";
				break;
			case Months::Jun: 
				txt << " Jun";
				break;
			case Months::Jul: 
				txt << " Jul";
				break;
			case Months::Aug: 
				txt << " Aug";
				break;
			case Months::Sep: 
				txt << " Sep";
				break;
			case Months::Oct: 
				txt << " Oct";
				break;
			case Months::Nov: 
				txt << " Nov";
				break;
			case Months::Dec: 
				txt << " Dec";
				break;
			}
			
			txt << " " << Year;
			txt << " " << Time.AsString();
			
			return txt.str();
		}
	};
} //namespace rtc

#endif //RTCTIME_H