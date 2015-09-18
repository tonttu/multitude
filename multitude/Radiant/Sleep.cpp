/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
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
#endif

#include <chrono>
#include <errno.h>
#include <time.h>

namespace Radiant {

  namespace
  {

    void nativeSleep(uint32_t usecs)
    {
#ifdef WIN32
      ::Sleep(usecs / 1000);
#else
      timespec req = {0, 1000*usecs}, rem = {0, 0};
      while(req.tv_nsec) {
        if(nanosleep(&req, &rem) != 0 && errno == EINTR) {
          std::swap(req, rem);
        } else {
          break;
        }
      }
#endif
    }

  }

  void Sleep::sleepS(uint32_t secs)
  {
    // This weird construct is necessary because signal handlers and such will interrupt sleeps
    auto start = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed(0);

    while(elapsed.count() < (secs - 0.2f)) {
      /* Sleep in 150ms increments*/
      nativeSleep(150 * 1000);

      auto end = std::chrono::high_resolution_clock::now();
      elapsed = end - start;
    }

    if(elapsed.count() < secs) {
      nativeSleep((secs - elapsed.count()) * 1000000);
    }
  }

  void Sleep::sleepMs(uint32_t msecs)
  {
    while(msecs > 1000) {
      sleepS(1);
      msecs -= 1000;
    }
    if(msecs)
      sleepUs(msecs * 1000);
  }


  void Sleep::sleepUs(uint32_t usecs)
  {
    if(usecs < 10*1000) {
      Radiant::Timer t;
      while (t.time() < (double)usecs * 0.000001) {
        nativeSleep(0);
      }
    } else {
      nativeSleep(usecs);
    }
  }

  void Sleep::sleepSome(double seconds)
  {
#ifdef WIN32
    ::Sleep(seconds * 1000);
#else
    usleep(seconds * 1000000);
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
