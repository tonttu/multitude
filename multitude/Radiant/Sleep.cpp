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
#include <assert.h>

namespace Radiant {

  namespace
  {
#ifdef RADIANT_WINDOWS
    void nativeSleep(uint64_t usecs)
    {
      /// @todo this is just copied from old code, use multimedia timers to achieve <10ms resolution?
      if(usecs < 10*1000) {
        Radiant::Timer t;
        while (t.time() < (double)usecs * 0.000001) {
          nativeSleep(0);
        } else {
          uint32_t ms = usecs / 1000;
          usecs = usecs % 1000;
          ::Sleep(ms);
        }
      }
    }
#else

#ifdef RADIANT_LINUX
    /// Assumes arguments are well formed - have nsec in [0, 999999999]
    timespec addTimespecs(const timespec & a, const timespec & b)
    {
      timespec result;
      result.tv_sec = a.tv_sec + b.tv_sec;
      result.tv_nsec = a.tv_nsec + b.tv_nsec;
      if(result.tv_nsec >= 1000*1000*1000) {
        result.tv_sec++;
        result.tv_nsec-=1000*1000*1000;
      }
      // TODO - detect overflow in tv_sec. It's signed so it's a bit annoying to do.
      return result;
    }

    void sleepTimespec(timespec req)
    {
      timespec now;
      int res = clock_gettime(CLOCK_MONOTONIC, &now);
      assert(res == 0);
      timespec then = addTimespecs(now, req);
      do {
        res = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &then, nullptr);
      } while(res == EINTR);
      assert(res == 0);
    }
#else
    void sleepTimespec(timespec req)
    {
      timespec rem = {0, 0};
      while(req.tv_sec || req.tv_nsec) {
        if(nanosleep(&req, &rem) != 0 && errno == EINTR) {
          std::swap(req, rem);
        } else {
          break;
        }
      }
    }
#endif  // RADIANT_LINUX

    void nativeSleep(uint64_t usecs)
    {
      if(usecs == 0) {
        sched_yield();
      } else {
        timespec req;
        req.tv_sec = static_cast<std::time_t>(usecs / 1000000);
        req.tv_nsec = static_cast<long>(1000*(usecs % 1000000));
        sleepTimespec(req);
      }
    }
#endif  // RADIANT_WINDOWS
  }  // unnamed namespace

  void Sleep::sleepS(uint32_t secs)
  {
    nativeSleep(1000*1000*(uint64_t)secs);
  }

  void Sleep::sleepMs(uint32_t msecs)
  {
    nativeSleep(1000*(uint64_t)msecs);
  }

  void Sleep::sleepUs(uint64_t usecs)
  {
    nativeSleep(usecs);
  }

  void Sleep::sleepSome(double seconds)
  {
#ifdef WIN32
    ::Sleep(seconds * 1000);
#else
    usleep(seconds * 1000*1000);
#endif
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
