/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Task.hpp"

#include "BGThread.hpp"

#include <typeinfo>

#include <Radiant/Trace.hpp>

namespace
{
  static float s_slowTaskDebuggingThresholdS = 0;
  static Radiant::Mutex s_sharedMutexMutex;

  std::map<void *, std::weak_ptr<Radiant::Mutex> > s_sharedMutexStore;
  std::shared_ptr<Radiant::Mutex> sharedMutex(void * ptr)
  {
    Radiant::Guard g(s_sharedMutexMutex);

    auto & weak = s_sharedMutexStore[ptr];
    std::shared_ptr<Radiant::Mutex> mutex = weak.lock();
    if (!mutex) {
      mutex = std::make_shared<Radiant::Mutex>();
      weak = mutex;
    }
    // do some cleanup
    if (s_sharedMutexStore.size() > 20) {
      auto it = s_sharedMutexStore.begin();
      for (int i = 0; i < 10; ++i) {
        if (it->second.lock()) {
          ++it;
        } else {
          it = s_sharedMutexStore.erase(it);
        }
      }
    }
    return mutex;
  }
}


namespace Radiant
{

  Task::Task(Priority p)
    : m_state(WAITING),
      m_canceled(false),
      m_priority(p),
      m_host(0),
      m_createStack(s_slowTaskDebuggingThresholdS > 0 ? new CallStack() : nullptr)
  {}

  void Task::runNow(bool finish)
  {
    if (m_state == DONE || isCanceled())
      return;

    auto mutex = sharedMutex(this);
    Radiant::Guard g(*mutex);

    if (m_state == DONE || isCanceled())
      return;

    // Must make a copy, since m_host might get cleared if the task completes
    auto host = m_host;
    if (host)
      host->removeTask(shared_from_this(), false, true);

    if (m_state == WAITING) {
      initialize();
      m_state = RUNNING;
    }

    while (m_state != DONE && !isCanceled()) {
      doTask();

      if (isCanceled())
        canceled();
      else if (m_state == DONE)
        finished();

      if (!finish)
        break;
    }
  }

  void Task::setSlowTaskDebuggingThreshold(float timeS)
  {
    if (timeS > 0) {
      Radiant::info("Enabling slow task debugging (threshold %.3f seconds)", timeS);
    } else {
      Radiant::info("Disabling slow task debugging");
    }
    s_slowTaskDebuggingThresholdS = timeS;
  }

  float Task::slowTaskDebuggingThreshold()
  {
    return s_slowTaskDebuggingThresholdS;
  }

  Task::~Task()
  {}

  double Task::secondsUntilScheduled() const { return -m_scheduled.time(); }

  void Task::scheduleFromNowSecs(double seconds)
  { m_scheduled.start(seconds); }

  void Task::initialize()
  {}

  void Task::canceled()
  {
  }

  void Task::finished()
  {
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  FunctionTask::FunctionTask(std::function<void (Task &)> func)
    : m_func(func)
  {}

  void FunctionTask::doTask()
  {
    m_func(*this);
  }

  void FunctionTask::executeInBGThread(std::function<void (Task &)> func)
  {
    BGThread::instance()->addTask(std::make_shared<FunctionTask>(func));
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  SingleShotTask::SingleShotTask(std::function<void ()> func)
    : m_func(func)
  {}

  void SingleShotTask::doTask()
  {
    m_func();
    setFinished();
  }

  void SingleShotTask::run(double delay, std::function<void ()> func)
  {
    auto task = std::make_shared<SingleShotTask>(func);
    if (delay != 0.0) {
      task->scheduleFromNowSecs(delay);
    }
    BGThread::instance()->addTask(std::move(task));
  }
}
