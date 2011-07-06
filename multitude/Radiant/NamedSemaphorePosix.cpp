/* COPYRIGHT
 *
 * This file is part of MultiWidgets.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitouch.fi
 *
 * All rights reserved, 2007-2010
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "MultiWidgets.hpp" for authors and more details.
 *
 */

#include "NamedSemaphore.hpp"
#include "Trace.hpp"

#include <string>
#include <cassert>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

namespace Radiant
{
  // Named sempahore implementation for Windows
  class NamedSemaphore_Impl
  {
  public:
    NamedSemaphore_Impl(const char * name, int locks)
      : m_name(name)
      , m_sem(NULL)
    {
      assert(locks > 0);

      // acquire a semaphore
      m_sem = sem_open(name, O_CREAT);
      assert(m_sem != SEM_FAILED);

      // Lock automatically
      lock();
    }

    ~NamedSemaphore_Impl()
    {
      unlock();
    }

    bool lock()
    {
      timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 0;

      m_locked = (sem_timedwait(m_sem, &ts) == 0);
      return m_locked;
    }

    void unlock()
    {
      if (sem_post(m_sem) != 0)
        Radiant::error("Failed to release named semaphore %s", m_name.c_str());
    }

    bool isLocked() const
    {
      return m_locked;
    }

  private:
    sem_t *m_sem;
    bool m_locked;
    std::string m_name;
  };

  NamedSemaphore::NamedSemaphore(const char * name, int locks)
  {
    m_impl = new NamedSemaphore_Impl(name, locks);
  }

  NamedSemaphore::~NamedSemaphore()
  {
    delete m_impl;
  }

  bool NamedSemaphore::lock()
  {
    return m_impl->lock();
  }

  bool NamedSemaphore::isLocked() const
  {
    return m_impl->isLocked();
  }
}
