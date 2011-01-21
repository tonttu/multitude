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
    Mutex();
    virtual ~Mutex();

    /** Initialize the mutex.

        Setting all values to false gives usually the best performance.

        The boolean arguments refer to POSIX-functionality with the same
        name. Not all features work on all platforms how-ever: My Linux
        box does not know anything about inheriting priorities.

        Recursion should work on all platforms. */

    /// @todo useless parameters (except recursive), get rid of them
    bool init(bool shared = false,
              bool prio_inherit = true,
              bool recursive = false);

    /// Close the mutex.
    bool close();

    /** Locks the mutex. Blocks until mutex is available. */
    bool lock();

    /// Lock the mutex, optionally blocking.
    bool lock(bool block);

    /** Tries to lock the mutex. Does not block. */
    bool tryLock();

    /// Unlocks the mutex.
    bool unlock();

  private:
    friend class Condition;

    class D;
    D * m_d;

  protected:

    /// Flag used to initialize static mutexes on non-linux platforms
    /// @todo shouldn't this be moved to MutexStatic?
    bool            m_active;
  };

  /// Mutex that initializes automatically.
  class RADIANT_API MutexAuto : public Mutex
  {
  public:
    /// Calls init.
    MutexAuto(bool shared = false,
              bool prio_inherit = true,
              bool recursive = false)
    { init(shared, prio_inherit, recursive); }
    ~MutexAuto() {}
  };


#ifdef __linux__

  typedef MutexAuto MutexStatic;

#else

  /// Mutex class to be used as static or global variable

  /** Under Linux, this class is simply typedef to MutexAuto. On other
    platforms (OSX, Windows) there is some trouble initializing
    mutexes as static variables as the application/library is
    loaded. For these cases there is an implementation that
    initializes when the mutex is first used.

    This can be problematic, if the mutex is accessed from two
    threads at exactly the same time for the first time. How-over,
    the probability of getting errors in that phase are extremely
    small. */
  class RADIANT_API MutexStatic : public Mutex
  {
  public:
    /// Creates a mutex, without initializing it
    /// @param shared Request a shared mutex
    /// @param prio_inherit Request a mutex with priority inheritance
    /// @param recursive Request a recursive mutex
    MutexStatic(bool shared = false,
                bool prio_inherit = true,
                bool recursive = false)
                  : m_shared(shared), m_prio_inherit(prio_inherit), m_recursive(recursive)
    {}

    bool lock() { if(!m_active) init(m_shared, m_prio_inherit, m_recursive); return Mutex::lock(); }
    bool lock(bool b) { if(!m_active) init(m_shared, m_prio_inherit, m_recursive); return Mutex::lock(b); }
    bool tryLock() { if(!m_active) init(m_shared, m_prio_inherit, m_recursive); return Mutex::tryLock(); }

  private:
    bool m_shared, m_prio_inherit, m_recursive;
  };
#endif

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
    /// Constructs a new guard and locks the mutex
    explicit Guard(Mutex & mutex) : m_mutex(mutex) { m_mutex.lock(); }
    explicit Guard(MutexAuto & mutex) : m_mutex(mutex) { m_mutex.lock(); }

    /// Unlocks the mutex
    ~Guard() { m_mutex.unlock(); }

  private:
    Mutex & m_mutex;
  };

  /** A guard class for static mutexes. */
  class GuardStatic : public Patterns::NotCopyable
  {
  public:
    /// Constructs a new guard and locks the mutex
    explicit GuardStatic(MutexStatic & mutex) : m_mutex(mutex) { m_mutex.lock(); }

    /// Unlocks the mutex
    ~GuardStatic() { m_mutex.unlock(); }

  private:
    MutexStatic & m_mutex;
  };

  /** A guard class that only releases a locked mutex. This class is
    used to automatically unlock a mutex within some function.

    @see Guard
    */
  class ReleaseGuard : public Patterns::NotCopyable
  {
  public:
    /// Constructs a new guard
    explicit ReleaseGuard(Mutex & mutex) : m_mutex( mutex) { }

    /// Unlocks the mutex
    ~ReleaseGuard() { m_mutex.unlock(); }

  private:
    Mutex & m_mutex;
  };

}

#endif
