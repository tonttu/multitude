/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */


#include "TimeStamp.hpp"

#include "StringUtils.hpp"

#include <string.h>
#include <time.h>

#include <QDateTime>

#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#include <winsock2.h>

namespace {
  class PreciseTimeWrapper
  {
  public:
    typedef void (WINAPI *PWINFUNC)(LPFILETIME);
    typedef ULONGLONG (*PFUNC)();
    static LONGLONG performanceCounter()
    {
        LARGE_INTEGER li;
        QueryPerformanceCounter(&li);
        return li.QuadPart;
    }

    static LONGLONG performanceFrequency()
    {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        return li.QuadPart;
    }

    static ULONGLONG dullFileTime()
    {
        FILETIME ft;
        ULONGLONG ullft;
        GetSystemTimeAsFileTime(&ft);
        ullft = (ULONGLONG)ft.dwHighDateTime<<32|ft.dwLowDateTime;
        return ullft;
    }

    static ULONGLONG fakePreciseFileTime()
    {
        ULONGLONG delta = (ULONGLONG)((double)(performanceCounter() - s_base_performance_time)/s_performance_frequency*s_filetime_ticks_per_sec);
        return s_base_file_time + delta;
    }

    static ULONGLONG realPreciseFileTime()
    {
      FILETIME ft;
      ULONGLONG ullft;
      s_winFunc(&ft);
      ullft = (ULONGLONG)ft.dwHighDateTime<<32|ft.dwLowDateTime;
      return ullft;
    }

    static PFUNC preciseFuncPointer()
    {
        s_winFunc = (PWINFUNC) GetProcAddress(
              GetModuleHandle(TEXT("kernel32.dll")),
              "GetSystemTimePreciseAsFileTime");
        if (s_winFunc)
        {
        return &PreciseTimeWrapper::realPreciseFileTime;
        }
        else
        {
        return &PreciseTimeWrapper::fakePreciseFileTime;
        }
    }

    static PWINFUNC s_winFunc;
    static ULONGLONG s_base_file_time;
    static LONGLONG s_base_performance_time;
    static LONGLONG s_performance_frequency;
    static int s_filetime_ticks_per_sec;
    static PFUNC s_precise_time_func;
  };

  PreciseTimeWrapper::PWINFUNC PreciseTimeWrapper::s_winFunc = nullptr;
  ULONGLONG PreciseTimeWrapper::s_base_file_time = PreciseTimeWrapper::dullFileTime();
  LONGLONG PreciseTimeWrapper::s_base_performance_time = PreciseTimeWrapper::performanceCounter();
  LONGLONG PreciseTimeWrapper::s_performance_frequency = PreciseTimeWrapper::performanceFrequency();
  int PreciseTimeWrapper::s_filetime_ticks_per_sec = 10000000;
  PreciseTimeWrapper::PFUNC PreciseTimeWrapper::s_precise_time_func = PreciseTimeWrapper::preciseFuncPointer();
}
#endif //WIN32

#include <QStringList>

namespace Radiant {

#ifdef WIN32

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

  struct timezone
  {
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
  };
 
  int gettimeofday(struct timeval *tv, struct timezone *tz)
  {
    unsigned __int64 tmpres = 0;
    static int tzflag;
    
    if (NULL != tv)
      {
    tmpres = PreciseTimeWrapper::s_precise_time_func();
	
	/*converting file time to unix epoch*/
	tmpres /= 10;  /*convert into microseconds*/
	tmpres -= DELTA_EPOCH_IN_MICROSECS;
	tv->tv_sec = (long)(tmpres / 1000000UL);
	tv->tv_usec = (long)(tmpres % 1000000UL);
      }
  
    if (NULL != tz)
      {
	if (!tzflag)
	  {
	    _tzset();
	    tzflag++;
	  }
	tz->tz_minuteswest = _timezone / 60;
	tz->tz_dsttime = _daylight;
      }
    
    return 0;
  }

#endif // WIN32

  TimeStamp::TimeStamp(const QDateTime & datetime)
    : m_val(std::lround(datetime.toMSecsSinceEpoch() * (ticksPerSecond().value() / 1000.0)))
  {
  }

  TimeStamp TimeStamp::createDate(const char * date,
				  const char * delim,
				  bool yearfirst)
  {
    if(!date)
      return TimeStamp(0);

    QStringList segments = QString::fromUtf8(date).split(delim);

    if(segments.size() != 3) {
      return TimeStamp(0);
    }

    int vals[3];

    for(int i = 0; i < 3; i++) {
      vals[i] = segments[i].toInt();
    }

    int year, month, day;

    month = vals[1]; // Month is always in the middle

    if(yearfirst) {
      year = vals[0];
      day = vals[2];
    }
    else {
      year = vals[2];
      day = vals[0];
    }

    struct tm tms;

    memset(& tms, 0, sizeof(tms));

    tms.tm_year = year - 1900;
    tms.tm_mon = month - 1;
    tms.tm_mday = day;
    
    time_t tval = mktime(&tms);

    return TimeStamp(tval * ticksPerSecond().value());
  }

  TimeStamp TimeStamp::createTime(const char * time,
				  const char * delim)
  {
    if(!time)
      return TimeStamp(0);

    QStringList segments = QString::fromUtf8(time).split(delim);
    
    if(segments.size() != 3) {
      return TimeStamp(0);
    }

    int vals[3];

    for(int i = 0; i < 3; i++) {
      vals[i] = segments[i].toInt();
    }

    int hour = vals[0];
    int minute = vals[1];
    int second = vals[2];
    
    return createDHMS(0, hour, minute, second);
  }

  TimeStamp TimeStamp::createDateTime(const char * date,
				      const char * delim,
				      bool yearfirst,
				      const char * time,
				      const char * timedelim)
  {
    return createDate(date, delim, yearfirst) + createTime(time, timedelim);
  }

  TimeStamp TimeStamp::currentTime()
  {
    struct timeval tv;
    gettimeofday(& tv, 0);
    int64_t tmp = tv.tv_sec;
    tmp <<= 24;
    tmp |= (int64_t) (tv.tv_usec * (FRACTIONS_PER_SECOND * 0.000001));
    return TimeStamp(tmp);
  }

  QString TimeStamp::asString() const {
	  time_t t = (m_val >> 24);

#ifdef WIN32
	  const int   bufSize = 32;
	  char  buf[bufSize];
	  ctime_s(buf, bufSize, & t);
	  buf[strlen(buf) - 1] = '\0';
	  return QString(buf);
#else
	// Convert to char* and remove \n
	char * str = ctime(&t);
	str[strlen(str) - 1] = '\0';

	return QString(str);
#endif

  }

  QString TimeStamp::asStringISO8601() const
  {
    QDateTime datetime = QDateTime::fromMSecsSinceEpoch(milliseconds()).toUTC();
    QDate date = datetime.date();
    QTime time = datetime.time();

    QChar fill = QLatin1Char('0');

    /// YYYY-MM-DDTHH:mm:ss.SSSZ
    QString str = QString("%1-%2-%3T%4:%5:%6.%7Z")
        .arg(date.year(), 4, 10, fill)
        .arg(date.month(), 2, 10, fill)
        .arg(date.day(), 2, 10, fill)
        .arg(time.hour(), 2, 10, fill)
        .arg(time.minute(), 2, 10, fill)
        .arg(time.second(), 2, 10, fill)
        .arg(time.msec(), 3, 10, fill);

    return str;
  }

  QDateTime TimeStamp::asQDateTime() const
  {
    return QDateTime::fromMSecsSinceEpoch(m_val / (ticksPerSecond().value() / 1000.0), Qt::UTC);
  }

  std::ostream & operator<<(std::ostream & os, const TimeStamp & ts)
  {
    return os << ts.value();
  }

  std::istream & operator>>(std::istream & is, TimeStamp & ts)
  {
    TimeStamp::type t;
    is >> t;
    ts.setValue(t);
    return is;
  }

}
