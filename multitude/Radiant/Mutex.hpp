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

#ifndef RADIANT_MUTEX_HPP
#define RADIANT_MUTEX_HPP

#include <Patterns/NotCopyable.hpp>

#include <Radiant/Export.hpp>

namespace Radiant {

  /** Mutex class. The mutex must be initialized explicitly. */
  class RADIANT_API Mutex : public Patterns::NotCopyable
  {
  public:
    Mutex(bool recursive = false);
    ~Mutex();

    /// Lock the mutex, optionally blocking.
    bool lock(bool block = true)
    {
      if(!m_initialized) {
        initialize(m_recursive);
        m_initialized = true;
      }

      return internalLock(block);
    }

    /** Tries to lock the mutex. Does not block. */
    bool tryLock()
    {
       if(!m_initialized) {
         initialize(m_recursive);
         m_initialized = true;
       }

       return internalTryLock();
    }

    /// Unlocks the mutex.
    bool unlock();

  private:    
    bool initialize(bool recursive);

    bool internalLock(bool block);
    bool internalTryLock();
    bool internalUnlock();

    bool m_initialized;
    bool m_recursive;

    class D;
    D * m_d;

    friend class Condition;
  };

  /** A guard class. This class is used to automatically lock and
    unlock a mutex within some function. This is useful when trying
    to avoid situations where there are several "return"-statements
    in a function, and one easily forget to unlock the mutex that
    one is using.

    <pre>

    int MyClass::doSomething()
    {
    // Mutex is locked here:
    Guarg g(mutex());

    // Mutex is freed reliably on each return path
    if(foo())
    return 0;
    if(fee())
    return 1;
    return 2;
    }

    </pre>

    @see ReleaseGuard
    */
  class Guard : public Patterns::NotCopyable
  {
  public:
    Guard(Mutex & mutex) : m_mutex(mutex) { m_mutex.lock(); }
    ~Guard() { m_mutex.unlock(); }

  private:
    Mutex & m_mutex;
  };

  /** A guard class that only releases a locked mutex. This class is
    used to automatically unlock a mutex within some function.

    @see Guard
    */
  class ReleaseGuard : public Patterns::NotCopyable
  {
  public:
    /// Constructs a new guard
    explicit ReleaseGuard(Mutex & mutex) : m_mutex(mutex) {}
    /// Unlocks the mutex
    ~ReleaseGuard() { m_mutex.unlock(); }

  private:
    Mutex & m_mutex;
  };

}

#endif
