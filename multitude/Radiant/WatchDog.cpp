/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "WatchDog.hpp"
#include "Radiant.hpp"

#include "Platform.hpp"
#include "Sleep.hpp"
#include "Trace.hpp"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef RADIANT_LINUX
#include "DateTime.hpp"
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <QStringList>

namespace {
  static bool s_watchdogEnabled = true;
}

namespace Radiant {

  WatchDog::WatchDog()
    : Radiant::Thread("Watchdog")
    , m_continue(true)
    , m_intervalSeconds(60.0f)
    , m_paused(false)
  {
    run();
  }

  WatchDog::~WatchDog()
  {
    stop();
  }

  void WatchDog::hostIsAlive(void * key, const QByteArray & name)
  {
    Radiant::Guard g(m_mutex);
    auto it = m_items.find(key);
    if (it == m_items.end()) {
      m_items[key].name = name;
    } else {
      it->second.lastAlive.start();
    }
  }

  void WatchDog::forgetHost(void * key)
  {
    Radiant::Guard g(m_mutex);
    m_items.erase(key);
  }

  void WatchDog::childLoop()
  {
    double nextInterval = m_intervalSeconds;
    while(m_continue) {
      int n = (int) ceilf(std::max(1.0, nextInterval) * 10.0f);

      // If paused, just sleep and try again
      if(m_paused) {
        Radiant::Sleep::sleepSome(1);
        continue;
      }

      /* A single long sleep might get interrupted by system calls and
       * return early. The method below should be more robust. */

      for(int i = 0; i < n && m_continue && !m_paused; i++)
        Radiant::Sleep::sleepMs(100);

      QStringList errorItems;
      nextInterval = m_intervalSeconds;
      if (isEnabled()) {
        Radiant::Guard g(m_mutex);
        for(container::iterator it = m_items.begin(); it != m_items.end(); ++it) {
          Item & item = it->second;
          double timeRemaining = m_intervalSeconds - item.lastAlive.time();
          if (timeRemaining <= 0)
            errorItems << QString::fromUtf8(item.name);
          nextInterval = std::min(nextInterval, timeRemaining);
        }
      }

      if (m_paused)
        continue;

      if(!errorItems.isEmpty() && m_continue) {
        error("WATCHDOG: THE APPLICATION HAS BEEN UNRESPONSIVE FOR %.0f\n"
              "SECONDS. IT HAS PROBABLY LOCKED, SHUTTING DOWN NOW.",
              (float) m_intervalSeconds);
        error("WATCHDOG: Unresponsive items: %s", errorItems.join(", ").toUtf8().data());

        std::map<long, std::function<void()>> listeners;
        {
          Radiant::Guard g(m_mutex);
          listeners = m_listeners;
        }
        for (auto & p: listeners) {
          p.second();
        }

        // Stop the app
        abort();
      }

      debugRadiant("WATCHDOG CHECK");

    }
  }

  void WatchDog::stop()
  {
    if(!m_continue)
      return;

    m_continue = false;
    while(isRunning())
      waitEnd(100);
  }

  long WatchDog::addListener(std::function<void ()> callback)
  {
    Radiant::Guard g(m_mutex);
    long id = m_nextListenerId++;
    m_listeners[id] = std::move(callback);
    return id;
  }

  void WatchDog::removeListener(long id)
  {
    Radiant::Guard g(m_mutex);
    m_listeners.erase(id);
  }

  bool WatchDog::isEnabled()
  {
    return s_watchdogEnabled;
  }

  void WatchDog::setEnabled(bool enabled)
  {
    s_watchdogEnabled = enabled;
  }

  DEFINE_SINGLETON(WatchDog);
}
