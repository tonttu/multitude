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

namespace Radiant {

  WatchDog * WatchDog::m_instance = 0;

  WatchDog::WatchDog()
    : Radiant::Thread("Watchdog")
    , m_continue(true)
    , m_intervalSeconds(60.0f)
    , m_paused(false)
  {
    if(!m_instance)
      m_instance = this;
  }

  WatchDog::~WatchDog()
  {
    stop();
    if(m_instance == this)
      m_instance = 0;
  }

  void WatchDog::hostIsAlive(void * key)
  {
    Radiant::Guard g(m_mutex);
    m_items[key].m_check = true;
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
    m_continue = true;

    while(m_continue) {
      int n = (int) ceilf(m_intervalSeconds * 10.0f);

      // If paused, just sleep and try again
      if(m_paused) {
        Radiant::Sleep::sleepS(1);
        continue;
      }

      /* A single long sleep might get interrupted by system calls and
	 return early. The method below should be more robust. */

      for(int i = 0; i < n && m_continue; i++)
        Radiant::Sleep::sleepMs(100);

      bool ok = true;
      {
        Radiant::Guard g(m_mutex);
        for(container::iterator it = m_items.begin(); it != m_items.end(); it++) {
          Item & item = it->second;
          if(item.m_check == false)
            ok = false;

          item.m_check = false;
        }
      }

      if(!ok && m_continue) {
        error("WATCHDOG: THE APPLICATION HAS BEEN UNRESPONSIVE FOR %.0f\n"
              "SECONDS. IT HAS PROBABLY LOCKED, SHUTTING DOWN NOW.\n"
              "TO DISABLE THIS FEATURE, DISABLE THE WATCHDOG WITH:\n\n"
              "export NO_WATCHDOG=1;\n", (float) m_intervalSeconds);

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
    if(isRunning())
      waitEnd();
  }

  WatchDog * WatchDog::instance()
  {
    return m_instance;
  }

}
