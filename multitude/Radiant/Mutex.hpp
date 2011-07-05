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

#include "Export.hpp"

#include <Patterns/NotCopyable.hpp>

#if defined(_MSC_VER)
#include <intrin.h> // For _ReadBarrier/_WriteBarrier
#endif

#if defined(__APPLE__)
#include <libkern/OSAtomic.h>
#endif

namespace Radiant {

  /** Mutex class. The mutex must be initialized explicitly. */
  class RADIANT_API Mutex : public Patterns::NotCopyable
  {
  public:
    Mutex(bool recursive = false);
    ~Mutex();

    /// Lock the mutex, optionally blocking.
    bool lock(bool block = true);

    /** Tries to lock the mutex. Does not block. */
    bool tryLock() { return lock(false); }

    /// Unlocks the mutex.
    bool unlock();

  private:

    friend class Condition;

    class D;
    D * m_d;
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
  class RADIANT_API Guard : public Patterns::NotCopyable
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

  /** A guard class, that does not get disturbed by lack of a mutex.

      If the mutex is NULL, then nothing happens.

   */

  class OptionalMutex : public Patterns::NotCopyable
  {
  public:
    OptionalMutex() : m_mutex(0) {}
    OptionalMutex(Mutex & mutex) : m_mutex( & mutex)
    {
      if(m_mutex)
        m_mutex->lock();
    }

    void init(Mutex & mutex)
    {
      if(&mutex == m_mutex)
        return;

      if(m_mutex) {
        m_mutex->unlock();
      }

      m_mutex = & mutex;

      if(m_mutex)
        m_mutex->lock();
    }

    ~OptionalMutex()
    {
      if(m_mutex)
        m_mutex->unlock();
    }
  private:
    Mutex * m_mutex;
  };

  extern RADIANT_API Mutex s_onceMutex;
}

/**
 * Implementation of Double-Checked Locking pattern.
 *
 * Example usage:
 * @code
 *   void doStuff() {
 *     MULTI_ONCE(initializeStuff();)
 *     useStuff();
 *   }
 * @endcode
 *
 * Another example:
 * @code
 *   void doStuff() {
 *     MULTI_ONCE_BEGIN
 *       initializeStuff();
 *       initializeSomeMoreStuff();
 *       sentSend("initialized");
 *     MULTI_ONCE_END
 *     useStuff();
 *   }
 * @endcode
 */
#ifdef __GLIBC__
#define MULTI_ONCE_BEGIN                                          \
  static bool s_multi_once = false;                               \
  /* hardware memory barrier */                                   \
  __sync_synchronize();                                           \
  /* compiler memory barrier */                                   \
  /** @todo is this implicit when using __sync_synchronize()? */  \
  __asm __volatile ("":::"memory");                               \
  if(!s_multi_once) {                                             \
    Radiant::Guard g(Radiant::s_onceMutex);                       \
    if(!s_multi_once) {
#define MULTI_ONCE_END                                            \
      __sync_synchronize();                                       \
      __asm __volatile ("":::"memory");                           \
      s_multi_once = true;                                        \
    }                                                             \
  }
#elif defined(_MSC_VER)
#define MULTI_ONCE_BEGIN                                          \
  /* s_multi_once is volatile, so msvc won't reorder stuff */     \
  static bool volatile s_multi_once = false;                      \
  /* hardware memory barrier */                                   \
  _ReadBarrier();                                                 \
  if(!s_multi_once) {                                             \
    Radiant::Guard g(Radiant::s_onceMutex);                       \
    if(!s_multi_once) {
#define MULTI_ONCE_END                                            \
      _WriteBarrier();                                            \
      s_multi_once = true;                                        \
    }                                                             \
  }
#elif defined(__APPLE__)
#define MULTI_ONCE_BEGIN                                          \
  static bool s_multi_once = false;                               \
  /* hardware memory barrier */                                   \
  OSMemoryBarrier();                                              \
  /* compiler memory barrier */                                   \
  /** @todo is this implicit when using __sync_synchronize()? */  \
  __asm __volatile ("":::"memory");                               \
  if(!s_multi_once) {                                             \
    Radiant::Guard g(Radiant::s_onceMutex);                       \
    if(!s_multi_once) {
#define MULTI_ONCE_END                                            \
      OSMemoryBarrier();                                          \
      __asm __volatile ("":::"memory");                           \
      s_multi_once = true;                                        \
    }                                                             \
  }
#endif

#define MULTI_ONCE(code)                                          \
  MULTI_ONCE_BEGIN                                                \
    code                                                          \
  MULTI_ONCE_END

#endif
