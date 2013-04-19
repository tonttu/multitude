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
    m_priority(p),
//    m_canDelete(false),
      m_scheduled(0),
      m_host(0)
  {}

  void Task::runNow(bool finish)
  {
    if (m_state == DONE || m_state == CANCELLED)
      return;

    auto mutex = sharedMutex(this);
    Radiant::Guard g(*mutex);

    if (m_state == DONE || m_state == CANCELLED)
      return;

    if (m_host)
      m_host->removeTask(shared_from_this(), false, true);

    if (m_state == WAITING) {
      initialize();
      m_state = RUNNING;
    }

    do {
      doTask();
    } while (finish && m_state != DONE);

    if (m_state == DONE)
      finished();
  }

  Task::~Task()
  {}

  void Task::initialize()
  {}

  void Task::finished()
  {
    // Radiant::trace("Task::finished # %s", typeid(*this).name());
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  FunctionTask::FunctionTask(std::function<void ()> func)
    : m_func(func)
  {}

  void FunctionTask::doTask()
  {
    m_state = RUNNING;

    m_func();

    setFinished();
  }
}
