/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_BGTHREAD_HPP
#define LUMINOUS_BGTHREAD_HPP

#include <Luminous/Export.hpp>
#include <Luminous/Task.hpp>

#include <Radiant/Condition.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/ThreadPool.hpp>

#include <list>
#include <map>
#include <set>

namespace Luminous
{

  /// A class used to execute tasks in a separated threads.

  class LUMINOUS_API BGThread : public Radiant::ThreadPool
  {

  public:
    BGThread();
    virtual ~BGThread();

    /// Add a task to be executed
    /** The task is the property of the BGThread, which will delete the object when its
        operation is finished.

        @param task The task that needs to be added.
    */
    virtual void addTask(Task * task);

    /// Remove the task from the BGThread
    /** If you just want to delete the task, then it is generally better to set the state of
        the task to finished, and schedule it immediately for processing (and thus removal).

        @param task The task to be removed
        @return True if the task was successfully removes, false otherwise.
    */
    virtual bool removeTask(Task * task);

    /// Update the changed task timestamp to queue
    virtual void reschedule(Task * task);

    /// Change the priority of a task
    virtual void setPriority(Task * task, Priority p);

    /** @return Returns the global BGThread instance. If no BGThread has been created
        yet, one will be created now.
        */
    static BGThread * instance();

    /// Container for the tasks
    typedef std::multimap<Priority, Task *, std::greater<Priority> > container;
    /// Objects stored in the task container
    typedef std::pair<Priority, Task * > contained;

    /// Returns the number of tasks in the BGThread.
    unsigned taskCount();
    /// Dump information about the tasks at hand
    void dumpInfo(FILE * f = 0, int indent = 0);
  private:
    virtual void childLoop();

    Task * pickNextTask();

    container::iterator findTask(Task * task);

    void wakeThread();
    void wakeAll();

    // locked with m_mutexWait
    container m_taskQueue;

    // a thread is already waiting for these tasks
    std::set<Task*> m_reserved;

    // number of idle threads, excluding ones that are reserving a task
    int m_idle;
    Radiant::Condition m_idleWait;

    static BGThread * m_instance;
  };

}

#endif

