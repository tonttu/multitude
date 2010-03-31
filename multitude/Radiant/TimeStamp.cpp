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


#include "TimeStamp.hpp"

#include "StringUtils.hpp"

#include <string.h>
#include <strings.h>
#include <time.h>

#ifndef WIN32
#include <sys/time.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#endif

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
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag;
    
    if (NULL != tv)
      {
	GetSystemTimeAsFileTime(&ft);
	
	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;
	
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

  TimeStamp TimeStamp::createDate(const char * date,
				  const char * delim,
				  bool yearfirst)
  {
    if(!date)
      return 0;

    StringUtils::StringList segments;
    StringUtils::split(date, delim, segments);

    if(segments.size() != 3) {
      return TimeStamp(0);
    }

    int vals[3];

    StringUtils::StringList::iterator it = segments.begin();

    for(int i = 0; i < 3; i++) {
      vals[i] = atoi((*it).c_str());
      it++;
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

    bzero(& tms, sizeof(tms));

    tms.tm_year = year - 1900;
    tms.tm_mon = month - 1;
    tms.tm_mday = day;
    
    time_t tval = mktime(&tms);

    //trace("tval as ctime = %s (%d %d %d)", ctime( & tval), year, month, day);

    return TimeStamp(tval * ticksPerSecond());
  }

  TimeStamp TimeStamp::createTime(const char * time,
				  const char * delim)
  {
    if(!time)
      return 0;

    StringUtils::StringList segments;
    StringUtils::split(time, delim, segments);
    
    if(segments.size() != 3) {
      return TimeStamp(0);
    }

    int vals[3];

    StringUtils::StringList::iterator it = segments.begin();

    for(int i = 0; i < 3; i++) {
      vals[i] = atoi((*it).c_str());
      it++;
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

  TimeStamp::type TimeStamp::getTime()
  {
	  struct timeval tv;
	  gettimeofday(& tv, 0);
	  int64_t tmp = tv.tv_sec;
	  tmp <<= 24;
	  tmp |= (int64_t) (tv.tv_usec * (FRACTIONS_PER_SECOND * 0.000001));
	  return tmp;
  }

  std::string TimeStamp::asString() const {
	  time_t t = (m_val >> 24);

#ifdef WIN32
	  const int   bufSize = 32;
	  char  buf[bufSize];
	  ctime_s(buf, bufSize, & t);
	  buf[strlen(buf) - 1] = '\0';
	  return std::string(buf);
#else
	// Convert to char* and remove \n
	char * str = ctime(&t);
	str[strlen(str) - 1] = '\0';

	return std::string(str);
#endif

  }



}
