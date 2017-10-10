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

#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 4 || ( \
    __GNUC__ == 4 && ( \
      __GNUC_MINOR__ < 8 || (__GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ < 1) \
    ) \
  ))
#include <cmath>
#include <time.h>
#else
#define RADIANT_TIMER_USE_CHRONO
#include <chrono>
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
  ///
  /// The clock is monotonic except on visual studio <= 2013.
  /// See https://connect.microsoft.com/VisualStudio/feedback/details/753115/
  class Timer
  {
  public:
    /// Construct a new Timer and @ref start it
    inline Timer();

    /// Start the timer and return elapsed time
    /// Starts the timer by resetting its clock to the current time offset with fromNowSeconds
    /// @param fromNow offset for the start time, for example -1.0 is 1 second ago
    /// @return elapsed time in seconds since the timer was previously started
    inline double start(double fromNowSeconds=0.0);
    /// Get start time
    /// Returns the time of the last @ref start call.
    /// @return start time in seconds since an arbitrary (but fixed) time point in the past.
    inline double startTime() const;
    /// Get elapsed time
    /// Returns the elapsed time in seconds since last @ref start call.
    /// @return elapsed time in seconds
    inline double time() const;

  private:
#ifdef RADIANT_TIMER_USE_CHRONO
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;
#else
    timespec m_startTime;
#endif
  };

  Timer::Timer()
  {
    start();
  }

#ifdef RADIANT_TIMER_USE_CHRONO
  double Timer::start(double fromNowSeconds)
  {
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        now - m_startTime).count();

    m_startTime = now;
    if(fromNowSeconds != 0.0)
      m_startTime += std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(fromNowSeconds));

    return elapsed;
  }

  double Timer::startTime() const
  {
    return std::chrono::duration_cast<std::chrono::duration<double>>(m_startTime.time_since_epoch()).count();
  }

  double Timer::time() const
  {
    return std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now() - m_startTime).count();
  }
#undef RADIANT_TIMER_USE_CHRONO
#else
  double Timer::start(double fromNowSeconds)
  {
    // ignoring error checking on purpose, we should just use std::chrono in the future
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double elapsed = now.tv_sec - m_startTime.tv_sec + (now.tv_nsec - m_startTime.tv_nsec) / 1e9;
    m_startTime = now;

    if(fromNowSeconds != 0.0) {
      // std::round is not available on gcc 4.7
      time_t secs = round(fromNowSeconds);
      long nsecs = (fromNowSeconds - secs) * 1e9;
      m_startTime.tv_sec += secs;
      // might become more than one second, maybe even become negative if
      // nsecs is negative because of a rounding error, but we don't care,
      // it doesn't matter for the other functions.
      m_startTime.tv_nsec += nsecs;
    }

    return elapsed;
  }

  double Timer::startTime() const
  {
    return m_startTime.tv_sec + m_startTime.tv_nsec / 1e9;
  }

  double Timer::time() const
  {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec - m_startTime.tv_sec + (now.tv_nsec - m_startTime.tv_nsec) / 1e9;
  }
#endif
}

#endif // RADIANT_TIMER_HPP
