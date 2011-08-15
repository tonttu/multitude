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
#include <Radiant/StringUtils.hpp>

#include <typeinfo>
#include <cassert>
#include <limits>

namespace Luminous
{

  // Helper to get around the fact that Task destructor is protected.
  class TaskDeleter
  {
  public:
    void operator()(Task* p)
    {
      delete p;
    }
  };

  std::weak_ptr<BGThread> BGThread::m_instance;

  BGThread::BGThread()
    : m_idle(0)
  {
  }

  BGThread::~BGThread()
  {
    stop();
  }

  /// @todo should this be deprecated?
  void BGThread::addTask(Task * task)
  {
    // Have to use custom deleter because Task destructor is protected
    addTask(std::shared_ptr<Task>(task, TaskDeleter()));
  }

  void BGThread::addTask(std::shared_ptr<Task> task)
  {
    assert(task);
    if(task->m_host == this) return;
    task->m_host = this;

    Radiant::Guard g(m_mutexWait);
    m_taskQueue.insert(contained(task->priority(), task));
    wakeThread();
  }

  bool BGThread::removeTask(std::shared_ptr<Task> task)
  {
    if(task->m_host != this)
      return false;

    Radiant::Guard g(m_mutexWait);

    if(m_reserved.find(task) != m_reserved.end())
      m_wait.wakeAll();

    container::iterator it = findTask(task);
    if(it != m_taskQueue.end()) {
      task->m_host = 0;
      m_taskQueue.erase(it);
      return true;
    }

    // The task isn't in the queue, maybe it's been executed currently
    return false;
  }

  void BGThread::reschedule(std::shared_ptr<Task> task)
  {
    Radiant::Guard g(m_mutexWait);
    if(m_reserved.find(task) != m_reserved.end()) {
      m_wait.wakeAll();
    } else {
      wakeThread();
    }
  }

  void BGThread::reschedule(std::shared_ptr<Task> task, Priority p)
  {
    Radiant::Guard g(m_mutexWait);
    if(m_reserved.find(task) != m_reserved.end()) {
      task->m_priority = p;
      m_wait.wakeAll();
    } else {
      if (task->m_priority != p) {
        container::iterator it = findTask(task);
        task->m_priority = p;
        if(it != m_taskQueue.end()) {
          m_taskQueue.erase(it);
         m_taskQueue.insert(contained(task->priority(), task));
        }
      }
      wakeThread();
    }
  }

  void BGThread::setPriority(std::shared_ptr<Task> task, Priority p)
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

   std::shared_ptr<BGThread> BGThread::instance()
   {
     std::shared_ptr<BGThread> p = m_instance.lock();
     if(!p) {
       p.reset(new BGThread());
       p->run();

       m_instance = p;
     }

     return p;
   }

  unsigned BGThread::taskCount()
  {
    Radiant::Guard guard(m_mutexWait);
    return (unsigned) m_taskQueue.size();
  }

  unsigned int BGThread::runningTasks() const
  {
    return m_runningTasks;
  }

  unsigned int BGThread::overdueTasks() const
  {
    Radiant::Guard guard(m_mutexWait);

    const Radiant::TimeStamp now = Radiant::TimeStamp::getTime();
    unsigned int counter = 0;

    for(container::const_iterator it = m_taskQueue.begin(), end = m_taskQueue.end();
        it != end; ++it) {
      Radiant::TimeStamp tmp = it->second->scheduled() - now;
      if(tmp <= 0) ++counter;
    }

    return counter;
  }

  void BGThread::dumpInfo(FILE * f, int indent)
  {
    Radiant::Guard guard(m_mutexWait);

    if(!f)
      f = stdout;

    for(container::iterator it = m_taskQueue.begin(); it != m_taskQueue.end(); it++) {
      Radiant::FileUtils::indent(f, indent);
      std::shared_ptr<Task> t = it->second;
      fprintf(f, "TASK %s %p\n", Radiant::StringUtils::demangle(typeid(*t).name()).toUtf8().data(), t.get());
      Radiant::FileUtils::indent(f, indent + 1);
      fprintf(f, "PRIORITY = %d UNTIL = %.3f\n", (int) t->priority(),
              (float) -t->scheduled().sinceSecondsD());
    }
  }

  void BGThread::childLoop()
  {
    while(running()) {
      // Pick a task to run
      std::shared_ptr<Task> task = pickNextTask();

      if(!task)
        break;

      // Run the task
      bool first = (task->state() == Task::WAITING);

      if(first) {
        task->initialize();
        task->m_state = Task::RUNNING;
      }

      if(task->state() != Task::DONE) {
        m_runningTasks.ref();
        task->doTask();
        m_runningTasks.deref();
      }

      // Did the task complete?
      if(task->state() == Task::DONE) {
        task->finished();
        task->m_host = 0;
      } else {
        // If we are still running, push the task to the back of the given
        // priority range so that other tasks with the same priority will be
        // executed in round-robin
        Radiant::Guard guard(m_mutexWait);
        m_taskQueue.insert(contained(task->priority(), task));
      }
    }
  }

  std::shared_ptr<Task> BGThread::pickNextTask()
  {
    while(running()) {
      Radiant::TimeStamp wait = std::numeric_limits<Radiant::TimeStamp::type>::max();

      Radiant::Guard guard(m_mutexWait);

      container::iterator nextTask = m_taskQueue.end();
      const Radiant::TimeStamp now = Radiant::TimeStamp::getTime();

      for(container::iterator it = m_taskQueue.begin(); it != m_taskQueue.end(); it++) {
        std::shared_ptr<Task> task = it->second;
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
        std::shared_ptr<Task> task = nextTask->second;
        m_reserved.insert(task);
        m_wait.wait(m_mutexWait, int(wait.secondsD() * 1000.0));
        m_reserved.erase(task);
      }
    }
    return std::shared_ptr<Task>();
  }

  BGThread::container::iterator BGThread::findTask(std::shared_ptr<Task> task)
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
