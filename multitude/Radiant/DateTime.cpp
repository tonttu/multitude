/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Radiant/DateTime.hpp>

#include <cassert>

#include <string.h>
#include <strings.h>

#ifndef WIN32
#include <sys/time.h>
#endif

#include <time.h>
#include <stdlib.h>


namespace Radiant {

  DateTime::DateTime()
      : m_year(0),
      m_month(0),
      m_monthDay(0),
      m_weekDay(0),
      m_hour(0),
      m_minute(0),
      m_second(0),
      m_microsecond(0),
      m_summerTime(false)
  {}

  DateTime::DateTime(const TimeStamp & t)
  {
    time_t secs = t.seconds();
    struct tm tms;

#ifdef WIN32
    localtime_s(& tms, & secs);
#else
    localtime_r(& secs, & tms);
#endif

    m_year     = tms.tm_year + 1900;
    m_month    = tms.tm_mon;
    m_monthDay = tms.tm_mday - 1;
    m_weekDay  = tms.tm_wday;
    m_hour     = tms.tm_hour;
    m_minute   = tms.tm_min;
    m_second   = tms.tm_sec;
    m_summerTime = (tms.tm_isdst == 0) ? false : true;

    TimeStamp fract = t.fractions();
    m_microsecond = fract.secondsD() * 1000000.0;
  }

  DateTime::~DateTime()
  {}

  void DateTime::clearTime()
  {
    m_hour   = 0;
    m_minute = 0;
    m_second = 0;
  }

  void DateTime::toNextYear()
  {
    m_year++;
  }
  void DateTime::toNextMonth()
  {
    m_month++;
    if(m_month >= 12) {
      m_month -= 12;
      m_year++;
    }
  }

  void DateTime::toNextMonthDay()
  {
    m_monthDay++;
    if(m_monthDay >= daysInMonth()) {
      m_monthDay = 0;
      m_month++;
      if(m_month >= 12) {
        m_month = 0;
        m_year++;
      }
    }
  }

  bool DateTime::fromString(const std::string & s, DateFormat df)
  { 
    if (df == DATE_ISO) {
      if(s.length() < 8)
        return false;

      std::string yearstr(s, 0, 4);
      std::string monthstr(s, 5, 2);
      std::string daystr(s, 8, 4);

      m_year  = atoi(yearstr.c_str());
      m_month = atoi(monthstr.c_str()) - 1;
      m_monthDay = atoi(daystr.c_str()) - 1;

      m_hour = 0;
      m_minute = 0;
      m_second = 0;
      m_microsecond = 0;
      m_summerTime = false;
    } else {
      if(s.length() < 19)
        return false;

      std::string daystr(s, 0, 2);
      std::string monthstr(s, 3, 2);
      std::string yearstr(s, 6, 4);

      std::string hourstr(s, 11, 2);
      std::string minstr(s, 14, 2);
      std::string secstr(s, 17, 2);

      m_year  = atoi(yearstr.c_str());
      m_month = atoi(monthstr.c_str()) - 1;
      m_monthDay = atoi(daystr.c_str()) - 1;

      m_hour = atoi(hourstr.c_str());
      m_minute = atoi(minstr.c_str());
      m_second = atoi(secstr.c_str());
      m_microsecond = 0;
      m_summerTime = false;
    }

    return true;
  }

  int DateTime::daysInMonth(int month, int year)
  {
    static const int table[12] = {
      31, 28, 31, 30, 31, 30,
      31, 31, 30, 31, 30, 31
    };

    assert((unsigned) month < 12);

    int days = table[month];
    if(month == 1) {
      // February
      if((year & 0x3) == 0)
        days = 29;
    }

    return days;
  }

  int DateTime::daysInMonth()
  {
    return daysInMonth(month(), year());
  }

  TimeStamp DateTime::asTimeStamp() const
  {
    struct tm tms;

    bzero( & tms, sizeof(tms));

    tms.tm_year = m_year - 1900;
    tms.tm_mon  = m_month;
    tms.tm_mday = m_monthDay + 1;
    tms.tm_wday = m_weekDay;
    tms.tm_hour = m_hour;
    tms.tm_min  = m_minute;
    tms.tm_sec  = m_second;
    tms.tm_isdst= m_summerTime;
    time_t tval = mktime(&tms);

    //trace("tval as ctime = %s (%d %d %d)", ctime( & tval), year, month, day);

    return TimeStamp(tval * TimeStamp::ticksPerSecond());
  }

  void DateTime::print(char * buf, bool isotime)
  {
    if(isotime)
      sprintf(buf, "%.2d-%.2d-%.2dT%.2d-%.2d-%.2d",
              year(), month() + 1, monthDay() + 1, hour(), minute(), second());
    else
      sprintf(buf, "%.2d-%.2d-%.2d,%.2d:%.2d:%.2d",
              monthDay() + 1, month() + 1, year(), hour(), minute(), second());
  }
}
