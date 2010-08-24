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
#include <Radiant/Mutex.hpp>
#include <Radiant/Thread.hpp>

#include <map>

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
    void hostIsAlive(void * key);
    void forgetHost(void * key);

    /** Sets the interval for checking if the host is alive. */
    void setInterval(float seconds)
    { m_intervalSeconds = seconds; }
    
    /** Stops the watchdog. */
    void stop();

    /** Gets the first watchdog instance. */
    static WatchDog * instance() { return m_instance; }

  private:
    
    virtual void childLoop();

    class Item
    {
    public:
      Item() : m_check(true) {}
      volatile bool m_check;
    };

    typedef std::map<void *, Item> container;

    container  m_items;

    Radiant::MutexAuto m_mutex;
    volatile bool m_continue;
    float m_intervalSeconds;

    static WatchDog *m_instance;

  };
  
}

#endif
