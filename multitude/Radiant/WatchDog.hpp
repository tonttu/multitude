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

    /** Inform the watchdog that the host application is working. You can call this function
        at any time, and the call is fully thread-safe. After calling this method for the first time,
        you need to keep calling it periodically, to make sure that the watchdog knows you are there.

        @param key The identifier of the calling object. This is usually a point to some object
        which provides a handy way of generating unique keys in C/C++.
    */
    void hostIsAlive(void * key);
    /** Instructs the Watchdog to forger some hosting object.
        @param key The identifier of the calling object.
    */
    void forgetHost(void * key);

    /// Sets the interval for checking if the host is alive.
    void setInterval(float seconds) { m_intervalSeconds = seconds; }
    
    /** Stops the watchdog. */
    void stop();

    /// Pauses the watchdog
    void pause() { m_paused = true; }
    /// Unpause the watchdog
    void unpause() { m_paused = false; }

    /// Check if the watchdog is paused
    /// @return true if paused; otherwise false
    bool paused() const { return m_paused; }

    /// Gets the first watchdog instance.
    static WatchDog * instance();

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

    Radiant::Mutex m_mutex;
    volatile bool m_continue;
    float m_intervalSeconds;
    bool m_paused;

    static WatchDog *m_instance;

  };
  
}

#endif
