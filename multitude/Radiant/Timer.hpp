#ifndef RADIANT_TIMER_HPP
#define RADIANT_TIMER_HPP

#include "Export.hpp"

namespace Radiant
{

  /// A timer.
  /// Timer gives better timing resolution than Radiant::TimeStamp. It is useful
  /// for measuring how long certain tasks take to complete. Typical usage is as
  /// follows:
  /// @code
  /// Radiant::Timer t;
  /// // <Do something>
  /// float elapsed = t.time();
  /// @endcode
  /// @sa Radiant::TimeStamp
  class RADIANT_API Timer
  {
  public:
    /// Construct a new Timer and start it
    Timer();

    /// Start the timer
    /// Starts the timer by resetting its clock to zero
    void start();
    /// Get elapsed time
    /// Returns the elapsed time in seconds since last start() call.
    /// @return elapsed time in seconds
    float time() const;
    /// Get the timer resolution
    /// Returns the number of timer ticks per second.
    /// @return resolution in ticks per second
    int resolution() const;

  private:
    class D;
    D * m_d;
  };

}

#endif
