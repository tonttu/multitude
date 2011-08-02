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

// Required for __GLIBC__
#include <cstdlib>

#include <vector>

#if defined(_MSC_VER)
#include <intrin.h> // For _ReadBarrier/_WriteBarrier
#endif

#if defined(__APPLE__)
#include <libkern/OSAtomic.h>
#endif

namespace Radiant {

  /** A mutex.*/
  class RADIANT_API Mutex : public Patterns::NotCopyable
  {
  public:
    Mutex(bool recursive = false);
    ~Mutex();

    /// Lock the mutex
    void lock();

    /** Tries to lock the mutex. Does not block. */
    bool tryLock();

    /// Unlocks the mutex.
    void unlock();

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
    /// Construct guard
    /// @param mutex mutex to guard
    Guard(Mutex & mutex) : m_mutex(mutex) { m_mutex.lock(); }
    ~Guard() { m_mutex.unlock(); }

  private:
    Mutex & m_mutex;
  };

  /// A guard class that can handle locking and unlocking of multiple mutexes.
  class GuardArray : public Patterns::NotCopyable
  {
  public:
    /// Construct guard array
    /// @param reserve pre-allocates memory for at least \b reserve mutexes
    GuardArray(int reserve = 4)
    {
      m_mutexArray.reserve(reserve);
    }

    /// Unlocks all locked mutexes
    ~GuardArray()
    {
      for(size_t i = 0, N = m_mutexArray.size(); i < N; ++i)
        if(m_mutexArray[i])
          m_mutexArray[i]->unlock();
    }

    /// Locks and adds one new mutex to the array
    /// @param mutex mutex to guard
    void lock(Mutex * mutex)
    {
      if(!mutex) return;
      m_mutexArray.push_back(mutex);
      mutex->lock();
    }

  private:
    std::vector<Mutex *> m_mutexArray;
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

  /// Shared mutex for all the MULTI_ONCE macros
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
