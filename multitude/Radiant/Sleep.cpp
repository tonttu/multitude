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

#include "Sleep.hpp"

#include "Mutex.hpp"
#include "TimeStamp.hpp"

// #include <QThread>

#ifdef WIN32
extern "C" {
#include <windows.h>
#include <winbase.h>
}
#endif

#include <time.h>

namespace Radiant {

  bool Sleep::sleepS(uint32_t secs)
  {
    for(uint32_t i = 0; i < secs; i++) {
      sleepMs(500);
      sleepMs(500);
    }

    return true;
  }

  bool Sleep::sleepMs(uint32_t msecs)
  {
    sleepUs(msecs * 1000);
    return true;
  }

  bool Sleep::sleepUs(uint32_t usecs)
  {
#ifdef WIN32
    ::Sleep(usecs / 1000);
#else
    usleep(usecs);
#endif

    return true;
  }

  /** Sleep until time indicated in the stamp object is passed.  The
      initialization time of this object is added to the stamp, before
      calculating sleeping period.

      The second argument should contain a locked mutex. The mutex
      will be freed while the (potential) sleeping is taking place and
      locked again.  */
  long SleepSync::sleepTo(const TimeStamp *stamp, Mutex *mutex)
  {
    TimeStamp now = TimeStamp::getTime();

    TimeStamp left = *stamp - now;

    if(left < 0)
      return left;

    mutex->unlock();

    Sleep::sleepUs(left.secondsD() * 1000000.0);

    mutex->lock();

    return 0;
  }

  /** Sleep in synchronous mode. The argument value is added to
      current time value. The return value tells how many microseconds
      early or late the function call returns. Zero is optimal,
      negative values indicate the call returned early and positive
      values indicate the call returned too late. */
  long SleepSync::sleepSynchroUs(long us)
  {
    TimeStamp target = m_initial + TimeStamp::createSecondsD(us * 0.000001);
    TimeStamp now = TimeStamp::getTime();

    if(now < target) {
      double secs = TimeStamp(target - now).secondsD();
      Sleep::sleepUs(secs * 1000000.0);
    }

    m_initial = target;

    return 0;
  }

}
