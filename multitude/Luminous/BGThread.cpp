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

#include <Luminous/BGThread.hpp>

#include <Nimble/Math.hpp>

#include <Radiant/FileUtils.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>

#include <typeinfo>
#include <cassert>
#include <limits>

namespace Luminous
{

  BGThread * BGThread::m_instance = 0;

  BGThread::BGThread() : m_idle(0)
  {
    if(m_instance == 0)
      m_instance = this;
  }

  BGThread::~BGThread()
  {
    if(m_instance == this)
      m_instance = 0;
    stop();
  }

  void BGThread::addTask(Task * task)
  {
    assert(task);
    task->m_host = this;

    Radiant::Guard g(m_mutexWait);
    m_taskQueue.insert(contained(task->priority(), task));
    wakeThread();
  }

  bool BGThread::removeTask(Task * task)
  {
    if(task->m_host != this)
      return false;

    Radiant::Guard g(m_mutexWait);

    if(m_reserved.find(task) != m_reserved.end())
      m_wait.wakeAll();

    container::iterator it = findTask(task);
    if(it != m_taskQueue.end()) {
      m_taskQueue.erase(it);
      return true;
    }

    // The task isn't in the queue, maybe it's been executed currently
    return false;
  }

  void BGThread::reschedule(Task * task)
  {
    Radiant::Guard g(m_mutexWait);
    if(m_reserved.find(task) != m_reserved.end()) {
      m_wait.wakeAll();
    } else wakeThread();
  }

  void BGThread::setPriority(Task * task, Priority p)
  {
    Radiant::Guard g(m_mutexWait);

    container::iterator it = findTask(task);
    task->m_priority = p;

    if(it != m_taskQueue.end()) {
      // Move the task in the queue and update its priority
      m_taskQueue.erase(it);
      m_taskQueue.insert(contained(p, task));
      if(m_reserved.find(task) != m_reserved.end()) {
        m_wait.wakeAll();
      } else wakeThread();
    }
  }

   BGThread * BGThread::instance()
   {
     if(!m_instance) {
       m_instance = new BGThread();
       m_instance->run();
     }
     else if(!m_instance->isRunning())
       m_instance->run();

     return m_instance;
   }

  unsigned BGThread::taskCount()
  {
    Radiant::Guard guard(m_mutexWait);
    return (unsigned) m_taskQueue.size();
  }

  void BGThread::dumpInfo(FILE * f, int indent)
  {
    Radiant::Guard guard(m_mutexWait);

    if(!f)
      f = stdout;

    for(container::iterator it = m_taskQueue.begin(); it != m_taskQueue.end(); it++) {
      Radiant::FileUtils::indent(f, indent);
      Task * t = it->second;
      fprintf(f, "TASK %s %p\n", typeid(*t).name(), t);
      Radiant::FileUtils::indent(f, indent + 1);
      fprintf(f, "PRIORITY = %d UNTIL = %.3f\n", (int) t->priority(),
              (float) -t->scheduled().sinceSecondsD());

    }
  }

  void BGThread::childLoop()
  {
    while(running()) {
      // Pick a task to run
      Task * task = pickNextTask();

      if(!task)
        break;

      // Run the task
      bool first = (task->state() == Task::WAITING);

      if(first) {
        task->initialize();
        task->m_state = Task::RUNNING;
      }

      task->doTask();

      // Did the task complete?
      if(task->state() == Task::DONE) {
        task->finished();
        delete task;
      } else {
        // If we are still running, push the task to the back of the given
        // priority range so that other tasks with the same priority will be
        // executed in round-robin
        Radiant::Guard guard(m_mutexWait);
        m_taskQueue.insert(contained(task->priority(), task));
      }
    }
  }

  Task * BGThread::pickNextTask()
  {
    while(running()) {
      Radiant::TimeStamp wait = std::numeric_limits<Radiant::TimeStamp::type>::max();

      Radiant::Guard guard(m_mutexWait);

      container::iterator nextTask = m_taskQueue.end();
      const Radiant::TimeStamp now = Radiant::TimeStamp::getTime();

      for(container::iterator it = m_taskQueue.begin(); it != m_taskQueue.end(); it++) {
        Task * task = it->second;
        Radiant::TimeStamp next = task->scheduled() - now;

        // Should the task be run now?
        if(next <= 0) {
          m_taskQueue.erase(it);
          return task;
        } else if(next < wait && m_reserved.find(task) == m_reserved.end()) {
          wait = next;
          nextTask = it;
        }
      }

      if(nextTask == m_taskQueue.end()) {
        ++m_idle;
        m_idleWait.wait(m_mutexWait);
        --m_idle;
      } else {
        Task * task = nextTask->second;
        m_reserved.insert(task);
        m_wait.wait(m_mutexWait, int(wait.secondsD() * 1000.0));
        m_reserved.erase(task);
      }
    }
    return 0;
  }

  BGThread::container::iterator BGThread::findTask(Task * task)
  {
    // Try optimized search first, assume that the priority hasn't changed
    std::pair<container::iterator, container::iterator> range = m_taskQueue.equal_range(task->priority());
    container::iterator it;

    for(it = range.first; it != range.second; ++it)
      if(it->second == task) return it;

    // Maybe the priority has changed, so iterate all tasks
    for(it = m_taskQueue.begin(); it != range.first; ++it)
      if(it->second == task) return it;

    for(it = range.second; it != m_taskQueue.end(); ++it)
      if(it->second == task) return it;

    return it;
  }

  void BGThread::wakeThread()
  {
    // assert(locked)
    // if there is at least one idle thread, we can just wake any of those threads randomly
    if(m_idle > 0)
      m_idleWait.wakeOne();
    else if(!m_reserved.empty())
      // wake all threads that are reserving any tasks,
      // since those could all be waiting for wrong tasks
      m_wait.wakeAll();
    // if there are no idle/reserving threads, then there is no point waking up anybody
  }

  void BGThread::wakeAll()
  {
    ThreadPool::wakeAll();
    m_idleWait.wakeAll();
  }
}
