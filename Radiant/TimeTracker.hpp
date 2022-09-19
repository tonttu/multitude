/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "Timer.hpp"

namespace Radiant
{
  /// This class can be used for debugging slow code paths. Don't use in
  /// production code. Use RADIANT_TRACK_TIME macro instead of this class
  /// directly. This class monitors wall-clock time instead of CPU cycles
  /// like most profilers.
  ///
  /// Example:
  /// {
  ///   // trigger a warning if the following takes more than 100 ms:
  ///   RADIANT_TRACK_TIME(0.1);
  ///   doSomethingMaybeSlow();
  /// }
  class TimeTracker
  {
  public:
    TimeTracker(const char * filename, int lineno, const char * func, double threshold)
      : m_filename(filename)
      , m_lineno(lineno)
      , m_func(func)
      , m_threshold(threshold)
    {}

    ~TimeTracker()
    {
      double t = m_timer.time();
      if (t >= m_threshold)
        Radiant::warning("%s:%d [%s]: %.3f s", m_filename, m_lineno, m_func, t);
    }

  private:
    Timer m_timer;
    const char * m_filename;
    const int m_lineno;
    const char * m_func;
    const double m_threshold;
  };

  /// Utility class for debugging performance. Don't use in production code.
  ///
  /// Example:
  /// void myRandomFunction()
  /// {
  ///   // Print how often this function is called
  ///   RADIANT_TRACK_FPS;
  /// }
  class FpsTracker
  {
  public:
    FpsTracker(const char * filename, int lineno, const char * func)
      : m_filename(filename)
      , m_lineno(lineno)
      , m_func(func)
    {}

    void update()
    {
      ++m_times;
      if (m_timer.time() >= 1.0) {
        double time = m_timer.start();
        int times = m_times;
        m_times = 0;
        Radiant::info("%s:%d [%s]: %.3f fps", m_filename, m_lineno, m_func, times / time);
      }
    }

  private:
    Radiant::Timer m_timer;
    int m_times = 0;
    const char * m_filename;
    const int m_lineno;
    const char * m_func;
  };
}

#define RADIANT_TRACK_TIME_CONCAT_(x, y) x ## y
#define RADIANT_TRACK_TIME_CONCAT(x, y) RADIANT_TRACK_TIME_CONCAT_(x, y)
#define RADIANT_TRACK_TIME(threshold) Radiant::TimeTracker RADIANT_TRACK_TIME_CONCAT(rtt, __LINE__)(__FILE__, __LINE__, __func__, (threshold))
#define RADIANT_TRACK_FPS static Radiant::FpsTracker RADIANT_TRACK_TIME_CONCAT(rft, __LINE__)(__FILE__, __LINE__, __func__); \
  RADIANT_TRACK_TIME_CONCAT(rft, __LINE__).update()
