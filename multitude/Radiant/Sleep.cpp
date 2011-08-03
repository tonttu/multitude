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

  void Sleep::sleepS(uint32_t secs)
  {
    // Apparently this weird construct is necessary because signal handlers and such will interrupt sleeps
    for(uint32_t i = 0; i < secs; i++) {
      sleepMs(500);
      sleepMs(500);
    }
  }

  void Sleep::sleepMs(uint32_t msecs)
  {
    sleepUs(msecs * 1000);
  }

  void Sleep::sleepUs(uint32_t usecs)
  {
#ifdef WIN32
    ::Sleep(usecs / 1000);
#else
    usleep(usecs);
#endif
  }

  /** Sleep in synchronous mode. The argument value is added to current time value.*/
  void SleepSync::sleepSynchroUs(long us)
  {
    TimeStamp target = m_initial + TimeStamp::createSecondsD(us * 0.000001);
    TimeStamp now = TimeStamp::getTime();

    if(now < target) {
      double secs = TimeStamp(target - now).secondsD();
      Sleep::sleepUs(secs * 1000000.0);
    }

    m_initial = target;
  }

}
