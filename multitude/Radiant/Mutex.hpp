/* COPYRIGHT
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
