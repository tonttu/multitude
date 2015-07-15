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

#include <chrono>

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

    /// Start the timer
    /// Starts the timer by resetting its clock to the current time
    inline void start();
    /// Get start time
    /// Returns the time of the last @ref start call.
    /// @return start time in seconds since an arbitrary (but fixed) time point in the past.
    inline double startTime() const;
    /// Get elapsed time
    /// Returns the elapsed time in seconds since last @ref start call.
    /// @return elapsed time in seconds
    inline double time() const;

  private:
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;
  };

  Timer::Timer()
  {
    start();
  }

  void Timer::start()
  {
    m_startTime = std::chrono::steady_clock::now();
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
}

#endif // RADIANT_TIMER_HPP
