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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>

namespace Radiant
{
  // Named sempahore implementation for Windows
  class NamedSemaphore_Impl
  {
  public:
    NamedSemaphore_Impl(const char * name, int locks)
      : m_name(name)
    {
      assert(locks > 0);

      // acquire a semaphore
      m_sem = CreateSemaphoreA( NULL, locks, locks, name);
      assert(m_sem != NULL);

      // Lock automatically
      lock();
    }

    ~NamedSemaphore_Impl()
    {
      unlock();
    }

    bool lock()
    {
      m_locked = (WaitForSingleObject(m_sem, 0) == WAIT_OBJECT_0);
      return m_locked;
    }

    void unlock()
    {
      if (! ReleaseSemaphore( m_sem, 1, NULL ) )
        Radiant::error("Failed to release named semaphore %s", m_name.c_str());
    }

    bool isLocked() const
    {
      return m_locked;
    }

  private:
    HANDLE m_sem;
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