/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_TIMER_HPP
#define RADIANT_TIMER_HPP

#include "Export.hpp"
#include "Radiant/TimeStamp.hpp"

#if defined(RADIANT_WINDOWS)
    #include <Windows.h>
#else
    #include <unistd.h>
#endif

namespace Radiant
{

  /// A timer.
  /// Timer is useful for measuring how long certain tasks take to complete.
  /// Typical usage is as follows:
  /// @code
  /// Radiant::Timer t;
  /// // <Do something>
  /// double elapsed = t.time();
  /// @endcode
  /// @sa Radiant::TimeStamp
  class Timer
  {
  public:
    /// Construct a new Timer and @ref start it
    inline Timer();

    /// Start the timer
    /// Starts the timer by resetting its clock to the current time
    inline void start();
    /// Get start time
    /// Returns the time of the last @ref start call.
    /// @return start time in seconds since unix epoch time.
    inline double startTime() const;
    /// Get elapsed time
    /// Returns the elapsed time in seconds since last @ref start call.
    /// @return elapsed time in seconds
    inline double time() const;
    /// Get the timer resolution
    /// Returns the number of timer ticks per second.
    /// @return resolution in ticks per second
    inline int resolution() const;

  private:
    double m_startTime;
  };

  Timer::Timer()
  {
    start();
  }

  void Timer::start()
  {
      m_startTime = TimeStamp::currentTime().secondsD();
  }

  int Timer::resolution() const
  {
      return (int)TimeStamp::ticksPerSecond().value();
  }

  double Timer::startTime() const
  {
    return m_startTime;
  }

  double Timer::time() const
  {
      return TimeStamp::currentTime().secondsD() - m_startTime;
  }
}

#endif // RADIANT_TIMER_HPP
