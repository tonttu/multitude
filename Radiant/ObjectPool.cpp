/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "ObjectPool.hpp"

static std::vector<Radiant::ObjectPool*> s_pools;

namespace Radiant
{
  ObjectPool::ObjectPool(std::size_t poolSize)
    : m_poolSize(poolSize)
  {
    s_pools.push_back(this);
  }

  ObjectPool::~ObjectPool()
  {
    auto it = std::find(s_pools.begin(), s_pools.end(), this);
    if (it != s_pools.end()) {
      s_pools.erase(it);
    }
  }

  std::pair<std::size_t, std::size_t> ObjectPool::fillAll()
  {
    std::size_t created = 0, total = 0;
    for (auto p: s_pools) {
      total += p->poolSize();
      created += p->fill();
    }
    return {created, total};
  }

  TaskPtr ObjectPool::createFillTask()
  {
    return std::make_shared<Radiant::FunctionTask>([] (Task & t) {
      auto p = ObjectPool::fillAll();
      t.scheduleFromNowSecs(p.first == 0 ? 2.0 : 0.1);
    });
  }

  void ObjectPool::setAllPoolSizes(std::size_t size)
  {
    for (auto p: s_pools) {
      p->setPoolSize(size);
    }
  }

  void ObjectPool::clearAll()
  {
    for (auto p: s_pools)
      p->clear();
  }

} // namespace Radiant
