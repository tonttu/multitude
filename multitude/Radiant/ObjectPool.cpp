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
