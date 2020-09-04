/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include <Radiant/BGThread.hpp>

#include <Nimble/Math.hpp>

#include <Radiant/FileUtils.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/StringUtils.hpp>

#include <typeinfo>
#include <cassert>
#include <limits>

namespace Radiant
{
  static void logSlowThreshold(double time, Radiant::Task & task, CallStack * callStack)
  {
    if (callStack) {
      Radiant::warning("Slow task: %s [%.3f s]:\n%s", StringUtils::type(task).data(),
                       time, callStack->toStringList().join("\n").toUtf8().data());
    } else {
      Radiant::warning("Slow task: %s [%.3f s] - no callstack available",
                       StringUtils::type(task).data(), time);
    }
  }

  BGThread::BGThread(const QString & threadNamePrefix)
    : ThreadPool(threadNamePrefix)
    , m_idle(0)
    , m_runningTasksCount(0)
    , m_isShuttingDown(false)
  {
  }

  BGThread::~BGThread()
  {
    shutdown();
  }

  void BGThread::addTask(std::shared_ptr<Task> task)
  {
    assert(task);
    if(!task)
      return;

    if(m_isShuttingDown) {
      task->setCanceled();
      return;
    }

    if(task->m_host == this) return;
    task->m_host = this;

    std::lock_guard<std::mutex> g(m_mutexWait);
    m_taskQueue.insert(contained(task->priority(), std::move(task)));
    wakeThread();
  }

  bool BGThread::removeTask(std::shared_ptr<Task> task, bool cancel, bool wait)
  {
    assert(task);
    if(!task)
      return false;
    if(task->m_host != this)
      return false;

    std::unique_lock<std::mutex> lock(m_mutexWait);

    if(m_reserved.find(task) != m_reserved.end())
      m_wait.notify_all();

    container::iterator it = findTask(task);
    if(it != m_taskQueue.end()) {
      task->m_host = nullptr;
      m_taskQueue.erase(it);
      if (cancel)
        task->setCanceled();
      return true;
    }

    // The task wasn't in the queue, maybe it's been executed currently, we don't care
    if (!wait)
      return false;

    if (m_runningTasks.count(task) > 0) {
      m_removeQueue.insert(task);
      while (!m_isShuttingDown && m_removeQueue.count(task) > 0) {
        m_removeCond.wait(lock);
      }
      if (m_isShuttingDown) {
        m_removeQueue.erase(task);
        return false;
      }
      if (cancel)
        task->setCanceled();
      return true;
    }

    return false;
  }

  void BGThread::reschedule(std::shared_ptr<Task> task)
  {
    assert(task);
    if(!task)
      return;
    std::lock_guard<std::mutex> g(m_mutexWait);
    if(m_reserved.find(task) != m_reserved.end()) {
      m_wait.notify_all();
    } else {
      wakeThread();
    }
  }

  void BGThread::reschedule(std::shared_ptr<Task> task, Priority p)
  {
    assert(task);
    if(!task)
      return;
    std::lock_guard<std::mutex> g(m_mutexWait);
    if(m_reserved.find(task) != m_reserved.end()) {
      task->m_priority = p;
      m_wait.notify_all();
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
    assert(task);
    if(!task)
      return;

    std::lock_guard<std::mutex> g(m_mutexWait);

    container::iterator it = findTask(task);
    task->setPriority(p);

    if(it != m_taskQueue.end()) {
      // Move the task in the queue and update its priority
      m_taskQueue.erase(it);
      m_taskQueue.insert(contained(p, task));
      if(m_reserved.find(task) != m_reserved.end()) {
        m_wait.notify_all();
      } else wakeThread();
    }
  }

  unsigned BGThread::taskCount()
  {
    std::lock_guard<std::mutex> g(m_mutexWait);
    return (unsigned) m_taskQueue.size() + m_runningTasksCount;
  }

  unsigned int BGThread::runningTasks() const
  {
    return m_runningTasksCount;
  }

  unsigned int BGThread::overdueTasks() const
  {
    std::lock_guard<std::mutex> g(m_mutexWait);

    unsigned int counter = 0;

    for(container::const_iterator it = m_taskQueue.begin(), end = m_taskQueue.end();
        it != end; ++it) {
      if(it->second->secondsUntilScheduled() <= 0.0)
        ++counter;
    }

    return counter;
  }

  void BGThread::dumpInfo(FILE * f, int indent)
  {
    std::lock_guard<std::mutex> g(m_mutexWait);

    if(!f)
      f = stdout;

    for(container::iterator it = m_taskQueue.begin(); it != m_taskQueue.end(); ++it) {
      Radiant::FileUtils::indent(f, indent);
      std::shared_ptr<Task> t = it->second;
      fprintf(f, "TASK %s %p\n", Radiant::StringUtils::type(*t).data(), t.get());
      Radiant::FileUtils::indent(f, indent + 1);
      fprintf(f, "PRIORITY = %d UNTIL = %.3f\n", (int) t->priority(),
              (float) t->secondsUntilScheduled());
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
        task->setState(Task::RUNNING);
      }

      if(task->state() == Task::RUNNING && !task->isCanceled()) {
        float slowThreshold = Task::slowTaskDebuggingThreshold();
        if (slowThreshold > 0.f) {
          Radiant::Timer timer;
          task->doTask();
          double time = timer.time();
          if (time >= slowThreshold) {
            logSlowThreshold(time, *task, task->m_createStack.get());
          }
        } else {
          task->doTask();
        }
      }

      bool done = (task->state() == Task::DONE || task->isCanceled());
      if (done) {
        /// @todo there is no thread-safe way to add the task back to BGThread
        /// if doTask() was just finished with DONE state. However, by clearing
        /// m_host before calling calling canceled/finished allows those
        /// functions to add the task back to the queue.
        ///
        /// That is still not thread-safe, since access to m_runningTasks might
        /// be interleaved wrong, but sometimes it might be acceptable, since
        /// it will only potentially break removeTask.
        task->m_host = nullptr;
        if (task->isCanceled()) {
          // Task is canceled: notify the task
          task->canceled();
        }
        else if(task->state() == Task::DONE) {
          // Task is completed: notify the task
          task->finished();
        }
      }

      std::lock_guard<std::mutex> g(m_mutexWait);
      m_runningTasks.erase(task);
      --m_runningTasksCount;
      if (m_isShuttingDown)
        task->m_host = nullptr;

      auto it = m_removeQueue.find(task);
      if (it != m_removeQueue.end()) {
        m_removeQueue.erase(it);
        m_removeCond.notify_all();
      } else if (!done) {
        // If we are still running, push the task to the back of the given
        // priority range so that other tasks with the same priority will be
        // executed in round-robin
        m_taskQueue.insert(contained(task->priority(), task));
      }
    }
  }

  std::shared_ptr<Task> BGThread::pickNextTask()
  {
    while(running()) {
      double wait = std::numeric_limits<double>::max();

      // We need to delete this task after m_mutexWait has been released,
      // otherwise there is a risk of getting a deadlock.
      std::shared_ptr<Task> reservedTask;

      std::unique_lock<std::mutex> lock(m_mutexWait);

      if (!running()) {
        return nullptr;
      }

      container::iterator nextTask = m_taskQueue.end();

      for(container::iterator it = m_taskQueue.begin(); it != m_taskQueue.end(); ++it) {
        std::shared_ptr<Task> task = it->second;

        double next = task->secondsUntilScheduled();

        // Should the task be run now?
        if(next <= 0) {
          m_taskQueue.erase(it);
          m_runningTasks.insert(task);
          m_runningTasksCount = static_cast<int>(m_runningTasks.size());
          return task;
        } else if(next < wait && m_reserved.find(task) == m_reserved.end()) {
          wait = next;
          nextTask = it;
        }
      }

      if(nextTask == m_taskQueue.end()) {
        ++m_idle;
        m_idleWait.wait(lock);
        --m_idle;
      } else {
        reservedTask = nextTask->second;
        m_reserved.insert(reservedTask);
        double waitTimeMs = wait * 1000.0;
        unsigned int waitTimei =
            waitTimeMs > std::numeric_limits<unsigned int>::max()
            ? std::numeric_limits<unsigned int>::max() - 1 : std::ceil(waitTimeMs);

        m_wait.wait_for(lock, std::chrono::milliseconds(waitTimei));
        m_reserved.erase(reservedTask);
      }
    }
    return std::shared_ptr<Task>();
  }

  BGThread::container::iterator BGThread::findTask(std::shared_ptr<Task> task)
  {
    assert(task);
    if(!task)
      return m_taskQueue.end();

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
      m_idleWait.notify_one();
    else if(!m_reserved.empty())
      // wake all threads that are reserving any tasks,
      // since those could all be waiting for wrong tasks
      m_wait.notify_all();
    // if there are no idle/reserving threads, then there is no point waking up anybody
  }

  void BGThread::wakeAll()
  {
    ThreadPool::wakeAll();
    std::unique_lock<std::mutex> lock(m_mutexWait);
    m_idleWait.notify_all();
  }

  void BGThread::shutdown()
  {
    m_isShuttingDown = true;

    container taskQueue;
    std::set<TaskPtr> reserved;

    {
      std::lock_guard<std::mutex> g(m_mutexWait);

      // Cancel all tasks
      for(auto & task : m_taskQueue) {
        task.second->setCanceled();
        task.second->m_host = nullptr;
      }

      for(auto & task : m_reserved) {
        task->setCanceled();
        task->m_host = nullptr;
      }

      for (auto & task: m_runningTasks)
        task->setCanceled();

      std::swap(taskQueue, m_taskQueue);
      std::swap(reserved, m_reserved);
    }

    /// Do not lock m_mutexWait while clearing these, since ~Task() might
    /// trigger something that calls BGThread::removeTask.
    taskQueue.clear();
    reserved.clear();

    stop();
  }

  void BGThread::run(int number)
  {
    m_isShuttingDown = false;
    ThreadPool::run(number);
  }
}

DEFINE_SINGLETON2(Radiant::BGThread,,, "BGThread")
