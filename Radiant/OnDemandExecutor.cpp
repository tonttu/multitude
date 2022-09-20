/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "OnDemandExecutor.hpp"

namespace Radiant
{
  void OnDemandExecutor::add(folly::Func func)
  {
    Radiant::Guard g(m_workMutex);
    if (!m_running)
      return;

    m_queue.emplace(std::move(func));
    if (!m_worker.joinable())
      m_worker = std::thread([this] { work(); });
  }

  OnDemandExecutor::~OnDemandExecutor()
  {
    decltype(m_queue) queue;
    std::thread worker;
    {
      Radiant::Guard g(m_workMutex);
      m_running = false;
      m_queue.swap(queue);
      worker = std::move(m_worker);
    }
    if (worker.joinable())
      worker.join();
  }

  void OnDemandExecutor::work()
  {
    while (true) {
      m_workMutex.lock();
      if (!m_running)
        return;

      if (m_queue.empty()) {
        std::thread me = std::move(m_worker);
        m_workMutex.unlock();
        if (me.joinable())
          me.detach();
        return;
      }

      folly::Func func = std::move(m_queue.front());
      m_queue.pop();
      m_workMutex.unlock();

      func();
    }
  }
}
