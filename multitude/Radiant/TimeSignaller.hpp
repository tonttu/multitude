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

#ifndef RADIANT_TIME_SIGNALLER_HPP
#define RADIANT_TIME_SIGNALLER_HPP

#include <Radiant/Timer.hpp>
#include <Radiant/Trace.hpp>

#include <signal.h>
#include <time.h>

namespace Radiant
{

  # define TIME_SIGNAL (SIGRTMIN)

  /**
    * @class TimeSignaller
    * For sending regular, timed signal calls to a custom handler function.
    */
  class TimeSignaller
  {
    public:

      /// Construction / destruction.

      /**
        * Constructor.
        * @param timeInterval Period in seconds between signals.
        * @param signalHandler Pointer to custom signal handler function. 
        */
      TimeSignaller(const float timeInterval = 0.0f,
        void (* signalHandler) (int, siginfo_t *, void *) = 0);

      /// Destructor.
      virtual ~TimeSignaller();

    private:

      /// Initialize and enable the timer.
      bool setTimer(const float timeInterval,
        void (* signalHandler) (int, siginfo_t *, void *));

      /// The timer ID.
      timer_t     m_timerID;
  };

}

#endif
