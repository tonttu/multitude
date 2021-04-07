#include "LimitedTimeExecutor.hpp"

#include <Radiant/Timer.hpp>

#include <QMutex>

#include <map>

namespace Punctual
{
  namespace
  {
    inline int8_t extractPriority(uint32_t idx)
    {
      uint32_t normalizedPriority = idx >> 24;
      return normalizedPriority + std::numeric_limits<int8_t>::min();
    }
  }

  class LimitedTimeExecutor::D
  {
  public:
    inline uint32_t makeIdx(int8_t priority)
    {
      // Scale the priority from -128..127 to 0..255
      uint32_t normalizedPriority = priority - std::numeric_limits<int8_t>::min();
      return (normalizedPriority << 24) | m_idx--;
    }

  public:
    QMutex m_tasksMutex;
    // Tasks in priority and insertion order.
    // High 8 bits of the key is the priority, rest 24 bits is m_idx. m_idx is
    // required to maintain the insertion order of tasks with equivalent priority.
    std::multimap<uint32_t, folly::Func, std::greater<uint32_t>> m_tasks;
    uint32_t m_idx = 0xFFFFFF;
    bool m_closing = false;
  };

  LimitedTimeExecutor::LimitedTimeExecutor()
    : m_d(new D())
  {}

  LimitedTimeExecutor::~LimitedTimeExecutor()
  {
    // When deleting m_tasks, it's possible that some folly promises will
    // in their destructors set some values to future executors, which might
    // include this executor. Avoid modifying m_tasks while it's being deleted.
    m_d->m_closing = true;
  }

  void LimitedTimeExecutor::add(folly::Func func)
  {
    if (m_d->m_closing)
      return;
    QMutexLocker g(&m_d->m_tasksMutex);
    m_d->m_tasks.emplace(m_d->makeIdx(folly::Executor::MID_PRI), std::move(func));
  }

  void LimitedTimeExecutor::addWithPriority(folly::Func func, int8_t priority)
  {
    if (m_d->m_closing)
      return;
    QMutexLocker g(&m_d->m_tasksMutex);
    m_d->m_tasks.emplace(m_d->makeIdx(priority), std::move(func));
  }

  bool LimitedTimeExecutor::run(double timeBudgetS, double lowPriorityTimeBudgetS)
  {
    Radiant::Timer timer;
    for (int i = 0;; ++i) {
      folly::Func func;
      bool lowPriority = false;
      {
        QMutexLocker g(&m_d->m_tasksMutex);
        if (m_d->m_tasks.empty()) {
          m_d->m_idx = 0xFFFFFF;
          return true;
        }

        auto it = m_d->m_tasks.begin();
        // We need to check timer here because this might be a low
        // priority task while the previous task was a high priority task,
        // and in this case we might already be over the low priority time
        // budget.
        lowPriority = extractPriority(it->first) < folly::Executor::MID_PRI;
        if (lowPriority && i > 0 && timer.time() >= lowPriorityTimeBudgetS)
          return false;

        func = std::move(it->second);
        m_d->m_tasks.erase(it);
      }

      func();

      double effectiveTimeBudget = lowPriority ? lowPriorityTimeBudgetS : timeBudgetS;
      if (timer.time() >= effectiveTimeBudget)
        return false;
    }
  }

  void LimitedTimeExecutor::clear()
  {
    bool closing = m_d->m_closing;
    m_d->m_closing = true;

    decltype(m_d->m_tasks) tasks;
    {
      QMutexLocker g(&m_d->m_tasksMutex);
      std::swap(tasks, m_d->m_tasks);
    }
    tasks.clear();

    m_d->m_closing = closing;
  }

  void LimitedTimeExecutor::shutdown()
  {
    m_d->m_closing = true;
    clear();
  }
}
