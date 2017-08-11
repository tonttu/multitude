/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_MUTEX_HPP
#define RADIANT_MUTEX_HPP

#include "Export.hpp"

#include <Patterns/NotCopyable.hpp>

#include <vector>
#include <cstdlib>
#include <atomic>

namespace Radiant {

  /// Mutual exclusion (or mutex for short) is used to avoid simultaneous use
  /// of a shared resource.
  /// A mutex can be recursive. This means the same calling thread can lock the
  /// mutex more than once and won't deadlock.
  class RADIANT_API Mutex : public Patterns::NotCopyable
  {
  public:
    /// Construct a mutex
    /// @param recursive if true, create a recursive mutex
    Mutex(bool recursive = false);
    /// Destructor
    ~Mutex();

    /// Lock the mutex    
    /// If another thread has already locked the mutex the calling thread will
    /// block until the other thread unlocks the mutex.
    void lock();

    /// Try to lock the mutex
    /// Tries to lock the mutex but does not block if the mutex has already been locked.
    /// @return true if the lock was attained, false otherwise
    bool tryLock();

    /// Unlock the mutex
    /// Must be called from the same thread as the mutex was locked from.
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
  class RADIANT_API Guard
  {
  public:
    /// Construct guard.
    /// Locks the given mutex.
    /// @param mutex mutex to guard
    Guard(Mutex & mutex) : m_mutex(&mutex) { m_mutex->lock(); }
    /// Moves the guard and lock state
    Guard(Guard && guard) : m_mutex(guard.m_mutex) { guard.m_mutex = nullptr; }
    /// Moves the guard and lock state
    Guard & operator=(Guard && guard)
    {
      if (m_mutex) m_mutex->unlock();
      m_mutex = guard.m_mutex;
      guard.m_mutex = nullptr;
      return *this;
    }

    /// Destructor, unlocks the associated mutex.
    ~Guard() { if (m_mutex) m_mutex->unlock(); }

    Guard(const Guard &) = delete;
    Guard & operator=(const Guard &) = delete;

  private:
    Mutex * m_mutex;
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
    /// Constructs a new guard. Does not lock the given mutex.
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
 *     MULTI_ONCE { initializeStuff(); }
 *     // or MULTI_ONCE initializeStuff();
 *     useStuff();
 *   }
 * @endcode
 *
 * Another example:
 * @code
 *   void doStuff() {
 *     MULTI_ONCE {
 *       initializeStuff();
 *       initializeSomeMoreStuff();
 *       sentSend("initialized");
 *     }
 *     useStuff();
 *   }
 * @endcode
 */

#define MULTI_ONCE                                                \
  static std::atomic<bool> s_multi_once{false};                   \
  if (!s_multi_once)                                              \
    for (Radiant::Guard multi_once_guard_(Radiant::s_onceMutex);  \
         !s_multi_once; s_multi_once = true)


#endif
