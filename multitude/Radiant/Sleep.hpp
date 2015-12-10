/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_SLEEP_HPP
#define RADIANT_SLEEP_HPP

#include <Radiant/Export.hpp>
#include <Radiant/Types.hpp>
#include <Radiant/Timer.hpp>

#include <cstdint>

namespace Radiant {

  class TimeStamp;

  /// Sleeping services. This class contains only static member
  /// functions.
  class RADIANT_API Sleep
  {
  public:   
    /// Sleep for n seconds.
    /// @param secs Number of seconds to sleep
    static void sleepS(uint32_t secs);

    /// Sleep for n milliseconds.
    /// @param msecs Number of milliseconds to sleep
    static void sleepMs(uint32_t msecs);

    /// Sleep for n microseconds.
    /// The resolution of this function is
    /// unlikely to be better than one millisecond on any platform,
    /// even if the underlying APIs might imply this.
    /// @param usecs Number of microseconds to sleep
    static void sleepUs(uint64_t usecs);

    /// Wrapper for Windows Sleep and Linux usleep. In windows this version has
    /// probably no better than 10ms accuracy, in Linux this might
    /// spontaneously return before timeout has passed. This function will
    /// always save some CPU resources, unlike some other functions in this
    /// class that might eventually end up using busy loops.
    /// @param seconds seconds to sleep
    static void sleepSome(double seconds);
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// Synchronized sleeping.
  /** This class can be used to execute a piece of code in fixed intervals.
      Here's a simple example where a loop cycle is executed every 100ms:

      \code
      const long loopCycleUs = 100*1000;//100ms
      Radiant::SleepSync sleep;
      while (true) {
         sleep.resetTiming();

         // do something that differs in length between [0..loopCycleUs]
         // ...

         sleep.sleepSynchroUs(loopCycleUs);
      }
      \endcode

  */
  class RADIANT_API SleepSync
  {
  public:
    /// The constructor resets the timing.
    SleepSync() { resetTiming(); }

    /// Resets the reference time to current time.
    void resetTiming()
    {
      m_initial.start();
    }

    /// Sleep for n microseconds
    /** This function calculates how much time has passed since the
    last sleep and sleeps to fulfill the required time period.
    @param us microseconds to sleep
    */
    void sleepSynchroUs(long us);

  private:

    Timer m_initial;

  };

}

#endif
