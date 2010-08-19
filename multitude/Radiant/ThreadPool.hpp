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

#ifndef RADIANT_THREADPOOL_HPP
#define RADIANT_THREADPOOL_HPP

#include "Export.hpp"
#include "Condition.hpp"

#include <Patterns/NotCopyable.hpp>

namespace Radiant {

  /// Thread pool class that is similar to Thread class, but the childLoop is
  /// executed concurrently with many threads.
  /// @see Thread
  class RADIANT_API ThreadPool : public Patterns::NotCopyable
  {
  public:
    /// Construct a thread pool class.
    /// The threads are NOT activated by this method.
    ThreadPool();

    /// @copydoc Thread::~Thread
    virtual ~ThreadPool();

    /// Sets the number of threads.
    /// If number is bigger than threads(), new threads are started immediately
    /// If number is smaller, then we politely give a hint to randomly chosen
    /// "extra" threads to shut down (@see stop())
    /// This can be freely called many times.
    /// This function is thread-safe.
    /// @param number the target number of threads
    bool run(int number = 1);

    /// Asks threads to stop. Doesn't work as expected if the childLoop()
    /// implementation doesn't use and obey running().
    bool stop();

    /// Waits until all threads are finished.
    /// @see Thread::waitEnd()
    bool waitEnd();

    /// Returns true if any of the threads are running.
    /// Also counts threads that have been asked to quit.
    /// Not to be confused with running()
    /// This function is thread-safe.
    bool isRunning() const;

    /// Returns the number of running or starting threads.
    /// Doesn't count threads that have been asked to quit.
    /// This function is thread-safe.
    int threads() const;

  protected:
    /// The actual contents of the threads. You need to override this to add
    /// functionality to your software. This will be called once per thread
    /// at the same time.
    virtual void childLoop() = 0;

    /// This should only be called from childLoop(),
    /// This function is thread-safe.
    bool running() const;

    /// Every time when we want to delete a thread, this condition variable
    /// will signaled.
    Radiant::Condition m_wait;
    /// Mutex to be used with m_wait.
    Radiant::MutexAuto m_mutexWait;

  private:
    class Private;
    Private * m_p;
  };
}

#endif
