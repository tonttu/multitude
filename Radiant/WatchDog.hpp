/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_WATCHDOG_HPP
#define RADIANT_WATCHDOG_HPP

#include <Radiant/Export.hpp>
#include <Radiant/Singleton.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/Timer.hpp>

#include <map>
#include <functional>

#include <QByteArray>

namespace Radiant {

  /** A guard that is used to make sure that programs do not get stuck.

      If the program appears to be stuck (not calling @ref hostIsAlive for
      given time) then this class simply shuts down the application.
   */
  class RADIANT_API WatchDog FINAL : private Radiant::Thread
  {
    DECLARE_SINGLETON(WatchDog);

  public:
    /// Constructor
    WatchDog();
    /// Destructor
    ~WatchDog();

    /** Inform the watchdog that the host application is working. You can call this function
        at any time, and the call is fully thread-safe. After calling this method for the first time,
        you need to keep calling it periodically, to make sure that the watchdog knows you are there.

        @param key The identifier of the calling object. This is usually a point to some object
        which provides a handy way of generating unique keys in C/C++.
    */
    void hostIsAlive(void * key, const QByteArray & name);
    /** Instructs the Watchdog to forget some hosting object.
        @param key The identifier of the calling object.
    */
    void forgetHost(void * key);

    /// Sets the interval for checking if the host is alive.
    /// @param seconds Length of the interval in seconds
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

    /// Add a listener that is called just before the watchdog is shutting down the process
    /// @returns listener id
    long addListener(std::function<void()> callback);

    /// Remove listener
    /// @param id listener id
    void removeListener(long id);

    /// Is the watchdog disabled by the user
    /// @return true if watchdog is enabled
    /// @sa setEnabled
    static bool isEnabled();

    /// Enable or disable the watchdog. This function only sets the flag if the
    /// watchdog should be used. It is up to the user the check the flag.
    /// @param enabled should the watchdog be enabled?
    /// @sa isEnabled
    static void setEnabled(bool enabled);

  private:

    virtual void childLoop();

    struct Item
    {
      Timer lastAlive;
      QByteArray name;
    };

    typedef std::map<void *, Item> container;

    container  m_items;

    Radiant::Mutex m_mutex;
    volatile bool m_continue;
    volatile float m_intervalSeconds;
    volatile bool m_paused;

    std::map<long, std::function<void()>> m_listeners;
    long m_nextListenerId = 0;
  };

}

#endif
