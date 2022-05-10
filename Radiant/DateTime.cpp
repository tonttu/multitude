/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include <Radiant/DateTime.hpp>

#include <cassert>

#include <string.h>

#ifndef WIN32
#include <sys/time.h>
#endif

#include <time.h>
#include <stdlib.h>

#include <QRegExp>

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

    TimeStamp fract = TimeStamp(t.fractions());
    m_microsecond = static_cast<int>(fract.secondsD() * 1000000.0);
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

  bool DateTime::fromString(const QString & s, DateFormat df)
  { 
    if (df == DATE_ISO) {
      if(s.length() < 8)
        return false;

      QRegExp r("^(\\d{4}).?(\\d{2}).?(\\d{2})");
      if(r.indexIn(s) >= 0) {
        int m = r.cap(2).toInt() - 1;
        int d = r.cap(3).toInt() - 1;
        if(m >= 12 || d >= 31 || m < 0 || d < 0) return false;
        *this = DateTime();
        m_year = r.cap(1).toInt();
        m_month = m;
        m_monthDay = d;
      } else return false;
    } else {
      if(s.length() < 14)
        return false;

      QRegExp r("^(\\d{4}).?(\\d{2}).?(\\d{2}).?(\\d{2}).?(\\d{2}).?(\\d{2})");
      if(r.indexIn(s) >= 0) {
        int m = r.cap(2).toInt() - 1;
        int d = r.cap(3).toInt() - 1;
        int h = r.cap(4).toInt();
        int min = r.cap(5).toInt();
        int sec = r.cap(6).toInt();
        if(m >= 12 || d >= 31 || m < 0 || d < 0 || h > 23 || min > 59 || sec > 59)
          return false;

        m_year  = r.cap(1).toInt();
        m_month = m;
        m_monthDay = d;

        m_hour = h;
        m_minute = min;
        m_second = sec;
        m_microsecond = 0;
        m_summerTime = false;
      } else return false;
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
      if ( year % 400 == 0) {
        days = 29;
      }
      else if ( year % 100 == 0) {
        ;
      }
      else if (year % 4 == 0) {
        days = 29;
      }
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

    memset( & tms, 0, sizeof(tms));

    tms.tm_year = m_year - 1900;
    tms.tm_mon  = m_month;
    tms.tm_mday = m_monthDay + 1;
    tms.tm_wday = m_weekDay;
    tms.tm_hour = m_hour;
    tms.tm_min  = m_minute;
    tms.tm_sec  = m_second;
    tms.tm_isdst= m_summerTime;
    time_t tval = mktime(&tms);

    return TimeStamp(tval * TimeStamp::ticksPerSecond().value());
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
