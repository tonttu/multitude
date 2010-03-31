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

#ifndef RADIANT_WATCHDOG_HPP
#define RADIANT_WATCHDOG_HPP

#include <Radiant/Export.hpp>
#include <Radiant/Thread.hpp>

namespace Radiant {

  /** A guard that is used to make sure that programs do not get stuck.

      If the program appears to be stuck (not calling #hostIsAlive for
      given time) then this class simply shuts down the application.
   */
  class RADIANT_API WatchDog : public Radiant::Thread
  {
  public:
    WatchDog();
    virtual ~WatchDog();

    /** Inform the watchdog that the host application is working. */
    void hostIsAlive();

    /** Sets the interval for checking if the host is alive. */
    void setInterval(float seconds)
    { m_intervalSeconds = seconds; }
    
    /** Stops the watchdog. */
    void stop();

  private:
    
    virtual void childLoop();

    volatile bool m_check;
    volatile bool m_continue;
    float m_intervalSeconds;
  };
  
}

#endif
