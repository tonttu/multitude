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
    m_items[key].m_check = true;
    m_items[key].m_name = name;
  }

  void WatchDog::forgetHost(void * key)
  {
    Radiant::Guard g(m_mutex);

    container::iterator it = m_items.find(key);
    if(it != m_items.end())
      m_items.erase(it);
  }


  void WatchDog::childLoop()
  {
    while(m_continue) {
      int n = (int) ceilf(m_intervalSeconds * 10.0f);

      // If paused, just sleep and try again
      if(m_paused) {
        Radiant::Sleep::sleepS(1);
        continue;
      }

      /* A single long sleep might get interrupted by system calls and
	 return early. The method below should be more robust. */

      for(int i = 0; i < n && m_continue && !m_paused; i++)
        Radiant::Sleep::sleepMs(100);

      QStringList errorItems;
      if(isEnabled())
      {
        Radiant::Guard g(m_mutex);
        for(container::iterator it = m_items.begin(); it != m_items.end(); ++it) {
          Item & item = it->second;
          if(item.m_check == false)
            errorItems << QString::fromUtf8(item.m_name);

          item.m_check = false;
        }
      }

      if (m_paused)
        continue;

      if(!errorItems.isEmpty() && m_continue) {
        error("WATCHDOG: THE APPLICATION HAS BEEN UNRESPONSIVE FOR %.0f\n"
              "SECONDS. IT HAS PROBABLY LOCKED, SHUTTING DOWN NOW.\n"
              "TO DISABLE THIS FEATURE, DISABLE THE WATCHDOG WITH:\n\n"
              "export NO_WATCHDOG=1;\n", (float) m_intervalSeconds);
        error("WATCHDOG: Unresponsive items: %s", errorItems.join(", ").toUtf8().data());

#ifdef RADIANT_LINUX
        struct rlimit limit;
        getrlimit(RLIMIT_CORE, &limit);

        if(limit.rlim_max > 0) {
          // Set the maximum core size limit
          limit.rlim_cur = limit.rlim_max;
          setrlimit(RLIMIT_CORE, &limit);

          FILE * pattern = fopen("/proc/sys/kernel/core_pattern", "r");
          char buffer[5] = {0};
          char * foo1 = fgets(buffer, sizeof(buffer), pattern);
          (void) foo1;
          fclose(pattern);

          // If there isn't any fancy core naming rule,
          // put the core in /tmp/core-timestamp directory
          if(strcmp(buffer, "core") == 0) {
            DateTime dt(TimeStamp::currentTime());
            char filename[255];
            sprintf(filename, "/tmp/core-%d.%04d-%02d-%02d", getpid(), dt.year(), dt.month()+1, dt.monthDay()+1);
            mkdir(filename, 0700);
            int foo = chdir(filename);
            (void) foo; // Silence the GCC warning
            info("Changing working directory to %s", filename);
          }
        }
#endif

        // Stop the app:
        abort();

        _exit(1);

        // Stop it again:
        Sleep::sleepS(1);
        Sleep::sleepS(1);
        int * bad = 0;
        *bad = 123456;

        // And again:
        exit(0);
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
