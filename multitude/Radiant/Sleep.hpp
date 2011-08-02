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
#  include <unistd.h>
#  include <sys/time.h>
#endif

#define RADIANT_BILLION 1000000000
#define RADIANT_MILLION 1000000

namespace Radiant {

  class TimeStamp;

  /** Sleeping services. This class contains only static member
      functions. The constructor and destructor are included to prevent
      compiler warnings.*/
  class RADIANT_API Sleep
  {
  public:

    Sleep() { }

    ~Sleep() {}

    /// Sleep for n seconds.
    /// @param secs Number of seconds to sleep
    static void sleepS(uint32_t secs);

    /** Sleep for n milliseconds. You cannot sleep more than one second
    with this function. */
    /// @param msecs Number of milliseconds to sleep
    static void sleepMs(uint32_t msecs);

    /** Sleep for n microseconds. You cannot sleep more than one
        second with this function. The resolution of this function is
        unlikely to be better than one millisecond on any platform,
        even if the underlying APIs might imply this.
    */
    /// @param usecs Number of microseconds to sleep
    static void sleepUs(uint32_t usecs);
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

  private:

    TimeStamp m_initial;

  };

}

#endif
