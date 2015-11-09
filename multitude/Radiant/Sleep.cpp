/*Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Sleep.hpp"

#include "Mutex.hpp"
#include "Timer.hpp"

#if defined(RADIANT_WINDOWS)
  #include <Windows.h>
#else
  #include <unistd.h>
  #include <sched.h>
#endif

#include <chrono>
#include <errno.h>
#include <time.h>

namespace Radiant {

  namespace
  {
    void nativeSleep(uint64_t usecs)
    {
#ifdef RADIANT_WINDOWS
      /// @todo this is just copied from old code, use multimedia timers to achieve <10ms resolution?
      if(usecs < 10*1000) {
        Radiant::Timer t;
        while (t.time() < (double)usecs * 0.000001) {
          nativeSleep(0);
        }
      } else {
        uint32_t ms = usecs / 1000;
        usecs = usecs % 1000;
        ::Sleep(ms);
      }
#else
      if(usecs == 0) {
        sched_yield();
      } else {
        timespec req;

        req.tv_sec = static_cast<std::time_t>(usecs / 1000000);
        req.tv_nsec = static_cast<long>(1000*(usecs % 1000000));
        timespec rem = {0, 0};

        while(req.tv_sec || req.tv_nsec) {
          if(nanosleep(&req, &rem) != 0 && errno == EINTR) {
            std::swap(req, rem);
          } else {
            break;
          }
        }
      }
#endif
    }
  }

  void Sleep::sleepS(uint32_t secs)
  {
    nativeSleep(1e6*secs);
  }

  void Sleep::sleepMs(uint32_t msecs)
  {
    nativeSleep(1e3*msecs);
  }


  void Sleep::sleepUs(uint64_t usecs)
  {
    nativeSleep(usecs);
  }

  /** Sleep in synchronous mode. The argument value is added to current time value.*/
  void SleepSync::sleepSynchroUs(long us)
  {
    double target = m_initial.startTime() + (us * 0.000001);
    Timer now;

    if(now.startTime() < target) {
      double secs = target - now.startTime();
      Sleep::sleepUs(secs * 1000000.0);
    }

    m_initial.start();
  }

}
