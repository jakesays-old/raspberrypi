#ifndef REALTIMECLOCKPRIVATE_H
#define REALTIMECLOCKPRIVATE_H

#include "LocalTypes.h"
#include "I2CBus.h"
#include "RtcAlarm.h"

// ReSharper disable CppPossiblyUninitializedMember

namespace rtc
{
	namespace pvt
	{	
		static inline byte FromBcd(byte val)
		{
            // Convert decimal to binary coded decimal
            byte result = (val & 0x0F) + (val >> 4) * 10;
			return result;
		}

		static inline byte ToBcd(byte val)
		{
            // Convert binary coded decimal to decimal
			return ((val / 10) << 4) + val % 10;
		}
	
		static inline Months ToMonth(byte value)
		{
            switch (value)
			{
            case 1:
				return Months::Jan;
            case 2:
				return Months::Feb;
            case 3:
				return Months::Mar;
            case 4:
				return Months::Apr;
            case 5:
				return Months::May;
            case 6:
				return Months::Jun;
            case 7:
				return Months::Jul;
            case 8:
				return Months::Aug;
            case 9:
				return Months::Sep;
            case 10:
				return Months::Oct;
            case 11:
				return Months::Nov;
            case 12:
				return Months::Dec;
			}
		
			return Months::None;
		}
	
        static inline const char* MonthName(Months value)
        {
            switch (value)
            {
                case Months::None:
                    return "None";
                case Months::Jan:
                    return "Jan";
                case Months::Feb:
                    return "Feb";
                case Months::Mar:
                    return "Mar";
                case Months::Apr:
                    return "Apr";
                case Months::May:
                    return "May";
                case Months::Jun:
                    return "Jun";
                case Months::Jul:
                    return "Jul";
                case Months::Aug:
                    return "Aug";
                case Months::Sep:
                    return "Sep";
                case Months::Oct:
                    return "Oct";
                case Months::Nov:
                    return "Nov";
                case Months::Dec:
                    return "Dec";                 
            }

            return "Invalid";
        }

		static inline byte FromMonth(Months value)
		{
			switch (value)
			{
			default:
				return -1;
			case Months::Jan:
                return 1;
			case Months::Feb:
                return 2;
			case Months::Mar:
                return 3;
			case Months::Apr:
                return 4;
			case Months::May:
                return 5;
			case Months::Jun:
                return 6;
			case Months::Jul:
                return 7;
			case Months::Aug:
                return 8;
			case Months::Sep:
                return 9;
			case Months::Oct:
                return 10;
			case Months::Nov:
                    return 11;
                    //return byte(0b00010001); //bcd 11
            case Months::Dec:
                    return 12;
                //return byte(0b00010010); //bcd 12
			}
		}
	
		static inline rtc::DayOfWeek ToDayOfWeek(byte value)
		{
			switch (value)
			{
			default:
				return rtc::DayOfWeek::None;
            case 1:
				return rtc::DayOfWeek::Mon;
            case 2:
				return rtc::DayOfWeek::Tue;
            case 3:
				return rtc::DayOfWeek::Wed;
            case 4:
				return rtc::DayOfWeek::Thu;
            case 5:
				return rtc::DayOfWeek::Fri;
            case 6:
				return rtc::DayOfWeek::Sat;						
            case 7:
                return rtc::DayOfWeek::Sun;
            }
		}
	
		static inline byte FromDayOfWeek(rtc::DayOfWeek value)
		{
			switch (value)
			{
			default:
				return -1;
			case rtc::DayOfWeek::Mon:
                return 1;
			case rtc::DayOfWeek::Tue:
                return 2;
			case rtc::DayOfWeek::Wed:
                return 3;
			case rtc::DayOfWeek::Thu:
                return 4;
			case rtc::DayOfWeek::Fri:
                return 5;
			case rtc::DayOfWeek::Sat:
                return 6;
            case rtc::DayOfWeek::Sun:
                return 7;
            }
		}		
	} //pvt
} //namespace rtc

#endif //REALTIMECLOCKPRIVATE_H
