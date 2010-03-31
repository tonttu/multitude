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

#include <Radiant/TimeSignaller.hpp>

#include <cstring>

#include <assert.h>

namespace Radiant
{

  // Class TimeSignaller.

  TimeSignaller::TimeSignaller(const float timeInterval,
    void (* signalHandler) (int, siginfo_t *, void *))
  : m_timerID(0)
  {
    trace("TimeSignaller::TimeSignaller");

    // Activate timer

    if(!setTimer(timeInterval, signalHandler))
    {
      error("TimeSignaller::TimeSignaller: setTimer()");
    }
  }

  TimeSignaller::~TimeSignaller()
  {
    trace("TimeSignaller::~TimeSignaller");

    timer_delete(m_timerID);
  }

  bool TimeSignaller::setTimer(const float timeInterval,
    void (* signalHandler) (int, siginfo_t *, void *))
  {
    trace("TimeSignaller::setTimer");

    assert(timeInterval > 0.0f);

    // Specify unique signal to be sent on timer expiration

    struct sigevent   signalEvent;
    signalEvent.sigev_notify = SIGEV_SIGNAL;
    signalEvent.sigev_signo = TIME_SIGNAL;
    signalEvent.sigev_value.sival_ptr = & m_timerID;

    // Setup the signal handler

    struct sigaction  signalAction;
    signalAction.sa_sigaction = signalHandler;
    signalAction.sa_flags = 0;
    sigemptyset(& signalAction.sa_mask);

    if(sigaction(TIME_SIGNAL, & signalAction, 0) == -1)
    {
      error("TimeSignaller::setTimer: sigaction()");
      return false;
    }

    // Create POSIX timer

    if(timer_create(CLOCK_REALTIME, & signalEvent, & m_timerID) == -1)
    {
      error("TimeSignaller::setTimer: timer_create()");
      return false;
    }

    // Set time until initial expiration

    struct timespec     spec;
    // seconds
    spec.tv_sec   = int(timeInterval);
    // + nanoseconds
    spec.tv_nsec  = long((timeInterval - spec.tv_sec) * 1.0e9 + 0.5f);

    struct itimerspec   value;
    value.it_value = spec;

    // Set interval for periodic timer

    value.it_interval = spec;

    // 'Arm' the timer

    if(timer_settime(m_timerID, 0, & value, 0) == -1)
    {
      error("TimeSignaller::setTimer: timer_settime()");
      return false;
    }

    return true;
  }

}
