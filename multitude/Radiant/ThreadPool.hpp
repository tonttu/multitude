/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_THREADPOOL_HPP
#define RADIANT_THREADPOOL_HPP

#include "Export.hpp"

#include <Patterns/NotCopyable.hpp>

#include <mutex>
#include <condition_variable>

#include <QThread>

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
    /// "extra" threads to shut down (ThreadPool::stop).
    ///
    /// This can be freely called many times.
    ///
    /// This function is thread-safe.

    /// @param number the target number of threads
    void run(int number = 1);

    /// Asks threads to stop. Doesn't work as expected if the childLoop()
    /// implementation doesn't use and obey running().
    /// @return true if the stop was successful
    bool stop();

    /// Waits until all threads are finished.
    /// @see Thread::waitEnd()
    /// @returns true if all threads have successfully finished
    bool waitEnd();

    /// Returns true if any of the threads are running.
    /// Not to be confused with running()
    /// This function is thread-safe.
    /// @returns true if any of the threads are running
    bool isRunning() const;

    /// Returns the number of running or starting threads.
    /// Doesn't count threads that have been asked to quit.
    /// This function is thread-safe.
    /// @returns the number of running or starting threads
    int threads() const;

    /// Check if the given thread belongs to the thread pool.
    /// @param thread thread to check
    /// @returns true if the thread belongs to the thread pool; otherwise false
    bool contains(const QThread* thread) const;

  protected:
    /// The actual contents of the threads. You need to override this to add
    /// functionality to your software. This will be called once per thread
    /// at the same time.
    virtual void childLoop() = 0;

    /// This should only be called from childLoop(),
    /// This function is thread-safe.
    /// @returns true if we should continue running this thread
    bool running() const;

    /// Wakes all threads to perform their duties
    virtual void wakeAll();

    /// Every time when we want to delete a thread, this condition variable
    /// will signaled.
    std::condition_variable m_wait;
    /// Mutex to be used with m_wait.
    mutable std::mutex m_mutexWait;

  private:
    class Private;
    Private * m_p;
  };
}

#endif
