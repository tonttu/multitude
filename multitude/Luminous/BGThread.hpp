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
#include <Radiant/Thread.hpp>

#include <list>
#include <map>

namespace Luminous
{

  /// A class used to execute tasks in a separate thread.

  class LUMINOUS_API BGThread : public Radiant::Thread
  {

  public:
    BGThread();
    virtual ~BGThread();

    /// Add a task to be executed
    virtual void addTask(Task * task);

    /// Remove the task from the BGThread
    virtual bool removeTask(Task * task);

    // Queue a task for deletion. The time of deletion is not guaranteed to be
    // immediate
    //virtual void markForDeletion(Task * task);

    /// Stop the thread and wait for it to terminate
    virtual void stop();

    /// Change the priority of a task
    ///@todo Check that it works in all cases
    virtual void setPriority(Task * task, Priority p);

    /// Returns an instance to a previously created BGThread if one has been
    /// created before. This function will not create a new instance.
    /// @return pointer to an instance or NULL if no BGThread has yet been created
    static BGThread * instance();

    /// Container for the tasks
    typedef std::multimap<Priority, Task *, std::greater<Priority> > container;
    /// Objects stored in the task container
    typedef std::pair<Priority, Task * > contained;

    /// Returns the number of tasks this thread is handling
    unsigned taskCount();

  private:
    virtual void childLoop();

    Task * pickNextTask(Radiant::TimeStamp & wait);

    Radiant::MutexAuto m_mutex;
    Radiant::MutexAuto m_mutexWait;
    Radiant::Condition m_wait;

    container m_taskQueue;

    volatile bool m_continue;
    static BGThread * m_instance;
  };

}

#endif

