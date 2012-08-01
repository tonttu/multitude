#ifndef RADIANT_TIMERPOSIX_HPP
#define RADIANT_TIMERPOSIX_HPP

#ifndef RADIANT_TIMER_HPP
#error "You shouldn't include TimerPosix.hpp directly. Use Timer.hpp instead"
#endif

# include <unistd.h>
# include <sys/time.h>

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
    /// Construct a new Timer and start it
    inline Timer();

    /// Start the timer
    /// Starts the timer by resetting its clock to zero
    inline void start();
    /// Get start time
    /// Returns the time of the last start() call.
    /// @return start time in seconds
    inline double startTime() const;
    /// Get elapsed time
    /// Returns the elapsed time in seconds since last start() call.
    /// @return elapsed time in seconds
    inline double time() const;
    /// Get the timer resolution
    /// Returns the number of timer ticks per second.
    /// @return resolution in ticks per second
    inline int resolution() const;

    /// Operators
    inline double operator-(const Timer & rhs) const;

    /// @return current time in seconds (equivalent to calling time())
    inline operator double() const;
  private:
    struct timeval m_startTime;
  };

  Timer::Timer()
  {
    start();
  }

  void Timer::start()
  {
    gettimeofday(&m_startTime, 0);
  }

  int Timer::resolution() const
  {
    /// gettimeofday gives microsecond resolution (in theory)
    return static_cast<int> (1e6);
  }

  double Timer::startTime() const
  {
    double ds  = m_startTime.tv_sec;
    double dus = m_startTime.tv_usec;

    return ds + 1e-6 * dus;
  }

  double Timer::time() const
  {
    struct timeval endTime;

    gettimeofday(&endTime, 0);

    double ds  = endTime.tv_sec - m_startTime.tv_sec;
    double dus = endTime.tv_usec - m_startTime.tv_usec;

    return ds + 1e-6 * dus;
  }

  double Timer::operator-(const Timer & rhs) const
  {
    return startTime() - rhs.startTime();
  }

  Timer::operator double() const
  {
    return time();
  }
}

#endif // RADIANT_TIMERPOSIX_HPP
