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
#include "Trace.hpp"

#if defined(RADIANT_WINDOWS)
  #include <Windows.h>
  #include <Mmsystem.h>
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
    struct EventHandle
    {
      EventHandle() : handle(CreateEvent(nullptr, false, false, nullptr)) {}
      ~EventHandle() { CloseHandle(handle); }
      HANDLE handle;
    };

    static TIMECAPS caps()
    {
      TIMECAPS tc = {1, 1000000};
      if (timeGetDevCaps(&tc, sizeof(tc)) != TIMERR_NOERROR) {
        Radiant::error("nativeSleep # timeGetDevCaps failed");
      }
      return tc;
    }

    static const TIMECAPS tc = caps();
    static __declspec(thread) EventHandle t_event;

    void nativeSleep(uint64_t usecs)
    {
      // sleep(0) == yield
      if (usecs == 0) {
        ::Sleep(0);
        return;
      }

      Radiant::Timer t;
      for (;;) {
        uint64_t elapsed = t.time() * 1000000;
        if (elapsed >= usecs) {
          break;
        }

        // ::Sleep on windows has typically 15 ms accuracy, and the function
        // usually sleeps at most 15 ms more than wanted. So if we want to
        // sleep 1 second, call ::Sleep(985) and use multimedia timers for the
        // rest. Generally ::Sleep is nicer than multimedia timers, since
        // multimedia timers might end up doing busy-looping.
        uint64_t sleepMs = (usecs - elapsed) / 1000;
        if (sleepMs > 15) {
          ::Sleep(sleepMs - 15);
          continue;
        }

        uint64_t sleepUs = std::min<uint64_t>(tc.wPeriodMax * 1000, usecs - elapsed);
        // We can't get under 1 ms accuracy using multimedia timers, use sleep(0)
        // on the computer, but on the other hand, ::Sleep(0) can take significantly
        // longer than 1 ms as well. There is no good solution for this.
        if (sleepUs < tc.wPeriodMin * 1000) {
          ::Sleep(0);
        } else {
          auto id = timeSetEvent(sleepUs / 1000, 0, (LPTIMECALLBACK)t_event.handle, 0,
                                 TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);

          // There can be at most 16 threads using time events. If this fails, fall
          // back to ::Sleep
          if (id == 0) {
            ::Sleep(sleepMs > 15 ? sleepMs - 15 : 1);
          } else {
            WaitForSingleObject(t_event.handle, INFINITE);
            timeKillEvent(id);
          }
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
#endif
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
#endif  // RADIANT_LINUX
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
