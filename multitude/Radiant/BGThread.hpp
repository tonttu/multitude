/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_BGTHREAD_HPP
#define RADIANT_BGTHREAD_HPP

#include <Radiant/Export.hpp>
#include <Radiant/Task.hpp>
#include <Radiant/Singleton.hpp>
#include <Radiant/ThreadPool.hpp>

#include <memory>
#include <list>
#include <map>
#include <set>
#include <functional>

namespace Radiant
{
  /** A class used to execute tasks in a separated threads.

    BGThread implements a thread-pool of one or more threads that are used to
    execute simple tasks that take too much time to be performed in the
    main-thread. Typical use-cases are generating mip-maps and converting
    images, loading large resources from disk or database, streaming resources
    over network, etc.

    BGThread owns tasks added to it and handles their destruction and memory
    management for you. If you need to keep a pointer to a task in BGThread,
    you should use the shared_ptr returned by Radiant::BGThread::addTask.

    If you decide to hold an external pointer to any Radiant::Task running in a
    BGThread, take special care if you decide to modify the task outside. You
    may not know if the Task is currently being executed in another thread.

    It is possible to change the number of threads executing tasks on the fly
    in BGThread by using the Radiant::ThreadPool::run function.

  **/
  class RADIANT_API BGThread : public Radiant::ThreadPool
  {
    DECLARE_SINGLETON(BGThread);
  public:
    BGThread(const QString & threadNamePrefix);
    virtual ~BGThread();

    /// Add a task to be executed
    /** The task is the property of the BGThread, which will delete the object when its
        operation is finished and the shared pointer's reference count goes to zero.

        @param task The task that needs to be added. Must not be null
    */
    virtual void addTask(TaskPtr task);

    /// Remove the task from the BGThread
    /** Generally you should not use this function. If you want to
        remove/delete a task, you set its state to finished
        (#Radiant::Task::setFinished) and schedule it for immediate processing
        after which BGThread will remove it when it has a chance.

        @param task The task to be removed. Must not be null
        @param cancel Should the task be cancelled if succesfully removed
        @param wait Block until the task execution returns, if the task is currently running
        @return True if the task was successfully removed, false otherwise.
        @sa Radiant::Task::setFinished
        @sa Radiant::Task::schedule
    */
    virtual bool removeTask(TaskPtr task, bool cancel = true, bool wait = false);

    /// Notify the BGThread that a task execution time has been rescheduled.
    /// This function should always be called after modifying the time-stamp of a
    /// task.
    /// @param task task to reschedule. Must not be null
    virtual void reschedule(TaskPtr task);
    /// @copydoc reschedule
    /// @param p new task priority
    void reschedule(TaskPtr task, Priority p);

    /// Change the priority of a task
    /// @param task task to modify. Must not be null
    /// @param p task priority
    virtual void setPriority(TaskPtr task, Priority p);

    /// Container for the tasks
    typedef std::multimap<Priority, TaskPtr, std::greater<Priority> > container;
    /// Objects stored in the task container
    typedef std::pair<Priority, TaskPtr > contained;

    /// Returns the number of tasks in the BGThread.
    /// This includes queued tasks and currently running tasks.
    /// @return Number of tasks.
    unsigned taskCount();

    /// Get the number of tasks right now in doTask().
    /// This function is lock-free and O(1).
    /// @return number of running tasks
    unsigned int runningTasks() const;

    /// Get the number of tasks that should be running right now but are not
    /// yet processed. This function is slow: O(N), needs a mutex lock and
    /// calls TimeStamp::currentTime().
    /// @return number of overdue tasks
    unsigned int overdueTasks() const;

    /// Dump information about the tasks at hand
    /// @param f File handle for printing. If null this will print to stdout
    /// @param indent Default intendation level, used internally in recursive calls
    void dumpInfo(FILE * f = 0, int indent = 0);

    /// Prepare the BGThread for shutdown. This function will cancel all
    /// currently queued tasks and wait for any currently executed tasks to
    /// finish. Any tasks added to the BGThread after calling this function
    /// will be immediately cancelled and never executed.
    void shutdown();

    /// Stops all threads once all tasks have been executed.
    void stopWhenDone();

    /// Restarts BGThread if it shutdown() was called earlier and calls
    /// Radiant::ThreadPool::run().
    virtual void run(int number = 1) override;

  private:
    virtual void childLoop() OVERRIDE;

    TaskPtr pickNextTask();

    container::iterator findTask(TaskPtr task);

    void wakeThread();
    virtual void wakeAll() override;

    /// @todo pimpl

    // locked with m_mutexWait
    container m_taskQueue;

    // a thread is already waiting for these tasks
    std::set<TaskPtr> m_reserved;

    // number of idle threads, excluding ones that are reserving a task
    int m_idle;
    std::condition_variable m_idleWait;

    // protected with m_mutexWait
    std::set<TaskPtr> m_runningTasks;
    int m_runningTasksCount;

    // protected with m_mutexWait, includes all tasks that should be removed and not rescheduled
    std::set<TaskPtr> m_removeQueue;
    // Use with m_mutexWait
    std::condition_variable m_removeCond;

    bool m_isShuttingDown;
    bool m_stopWhenDone = false;
  };

}

#endif

