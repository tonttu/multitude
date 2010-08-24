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
    virtual void addTask(Task * task);

    /// Remove the task from the BGThread
    virtual bool removeTask(Task * task);

    /// Update the changed task timestamp to queue
    virtual void reschedule(Task * task);

    /// Change the priority of a task
    virtual void setPriority(Task * task, Priority p);

    /// Returns the global BGThread instance. If no BGThread has been created
    /// yet, one will be created now.
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

    // locked with m_mutexWait
    container m_taskQueue;

    // a thread is already waiting for these tasks
    std::set<Task*> m_reserved;

    static BGThread * m_instance;
  };

}

#endif

