/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef RADIANT_SLEEP_HPP
#define RADIANT_SLEEP_HPP

#include <Radiant/Export.hpp>
#include <Radiant/Types.hpp>
#include <Radiant/TimeStamp.hpp>

#include <stdint.h>
#include <time.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#else

//#include <WinPort.h>		// for sleep() and nanosleep()
//#include <pthread.h>		// for struct timespec
#endif

#define RADIANT_BILLION 1000000000
#define RADIANT_MILLION 1000000

namespace Radiant {

  class Mutex;
  class TimeStamp;

  enum Timing {
    TIMING_GOOD,
    TIMING_LATE,
    TIMING_EARLY
  };

  // POSIX-specific functions:
#ifndef WIN32
  /// @todo Check if these functions are needed.
  inline void addTimeNs(struct timespec *tspec, long ns)
  {
    tspec->tv_nsec += ns;
    if(tspec->tv_nsec >= RADIANT_BILLION) {
      long tmp = tspec->tv_nsec / RADIANT_BILLION;
      tspec->tv_sec += tmp;
      tspec->tv_nsec = tspec->tv_nsec - tmp * RADIANT_BILLION;
    }
  }

  inline void addTimeUs(struct timeval *tspec, long us)
  {
    tspec->tv_usec += us;
    if(tspec->tv_usec >= RADIANT_MILLION) {
      long tmp = tspec->tv_usec / RADIANT_MILLION;
      tspec->tv_sec += tmp;
      tspec->tv_usec = tspec->tv_usec - tmp * RADIANT_MILLION;
    }
  }

  inline void addTime(struct timeval *tspec, const struct timeval *tspecAdd)
  {
    tspec->tv_sec  += tspecAdd->tv_sec;
    tspec->tv_usec += tspecAdd->tv_usec;
    if(tspec->tv_usec >= RADIANT_MILLION) {
      long tmp = tspec->tv_usec / RADIANT_MILLION;
      tspec->tv_sec += tmp;
      tspec->tv_usec = tspec->tv_usec - tmp * RADIANT_MILLION;
    }
  }

  inline long timeDiffNs(const struct timespec *tspecOld,
             const struct timespec *tspecNew)
  {
    return long((tspecNew->tv_sec - tspecOld->tv_sec) * RADIANT_BILLION +
      tspecNew->tv_nsec - tspecOld->tv_nsec);
  }

  inline long timeDiffUs(const struct timeval *tspecOld,
             const struct timeval *tspecNew)
  {
    return long((tspecNew->tv_sec - tspecOld->tv_sec) * RADIANT_MILLION +
      tspecNew->tv_usec - tspecOld->tv_usec);
  }

#endif // !WIN32

  /** Sleeping services. This class contains only static member
      functions. The constructor and destructor are included to prevent
      compiler warnings.*/
  class RADIANT_API Sleep
  {
  public:

    Sleep() { }

    ~Sleep() {}

    /// Sleep for n seconds.
    static bool sleepS(uint32_t secs);

    /** Sleep for n milliseconds. You cannot sleep more than one second
    with this function. */
    static bool sleepMs(uint32_t msecs);

    /** Sleep for n microseconds. You cannot sleep more than one
        second with this function. The resolution of this function is
        unlikely to be better than one millisecond on any platform,
        even if the underlying APIs might imply this.

    */
    static bool sleepUs(uint32_t usecs);
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// Synchronized sleeping.
  /** This class can be used to time the execution of a thread. For
      example if you want a thread not to execute too often.*/

  /// @todo Add example(s)
  class RADIANT_API SleepSync
  {
  public:
    /// The constructor resets the timing.
    SleepSync() { resetTiming(); }

    /// Resets the reference time to current time.
    void resetTiming()
    {
      m_initial = TimeStamp::getTime();
    }

    /// Sleep for n microseconds
    /** This function calculates how much time has passed since the
    last sleep and sleeps to fulfill the required time period. */
    long sleepSynchroUs(long us);

    // @todo Unused(?), remove
    long sleepTo(const TimeStamp *stamp, Mutex *mutex = 0);

  private:

    TimeStamp m_initial;

  };

}

#endif
