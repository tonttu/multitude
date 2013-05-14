/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_TIMERW32_HPP
#define RADIANT_TIMERW32_HPP

#ifndef RADIANT_TIMER_HPP
#error "You shouldn't include TimerW32.hpp directly. Use Timer.hpp instead"
#endif

# define WIN32_LEAN_AND_MEAN
# include <Windows.h>

namespace Radiant
{
  /// A timer.
  /// Timer gives better timing resolution than Radiant::TimeStamp. It is useful
  /// for measuring how long certain tasks take to complete. Typical usage is as
  /// follows:
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
    /// Starts the timer by resetting its clock to zero
    inline void start();
    /// Get start time
    /// Returns the time of the last @ref start call.
    /// @return start time in seconds
    inline double startTime() const;
    /// Get elapsed time
    /// Returns the elapsed time in seconds since last @ref start call.
    /// @return elapsed time in seconds
    inline double time() const;
    /// Get the timer resolution
    /// Returns the number of timer ticks per second.
    /// @return resolution in ticks per second
    inline int resolution() const;

    /// Casts timer to double
    /// @return current time in seconds (equivalent to calling time())
    inline operator double() const;
  private:
    LARGE_INTEGER m_performanceFrequency;
    double m_performanceReciprocal;
    LARGE_INTEGER m_startTime;
  };

  Timer::Timer()
  {
    QueryPerformanceFrequency(&m_performanceFrequency);
    m_performanceReciprocal = 1.0 / static_cast<double> (m_performanceFrequency.QuadPart);
    start();
  }

  void Timer::start()
  {
    QueryPerformanceCounter(&m_startTime);
  }

  int Timer::resolution() const
  {
    return static_cast<int> (m_performanceFrequency.QuadPart);
  }

  double Timer::startTime() const
  {
    return m_startTime.QuadPart * m_performanceReciprocal;
  }

  double Timer::time() const
  {
    LARGE_INTEGER endTime;
    QueryPerformanceCounter(&endTime);
    return double((endTime.QuadPart - m_startTime.QuadPart) * m_performanceReciprocal);
  }

  Timer::operator double() const
  {
    return time();
  }
}

#endif // RADIANT_TIMERW32_HPP
