#pragma once

#include "Export.hpp"
#include <folly/Executor.h>

#include <memory>

namespace Punctual
{
  /// Manually driven executor that supports task priorities but maintains
  /// relative order of tasks with the same priority. Driving the executor
  /// is done with run() where two different wall-clock time limits can be
  /// given.
  class PUNCTUAL_API LimitedTimeExecutor : public folly::Executor
  {
  public:
    LimitedTimeExecutor();
    virtual ~LimitedTimeExecutor();

    virtual void add(folly::Func func) override;
    virtual void addWithPriority(folly::Func func, int8_t priority) override;

    /// Run graph executor tasks for a limited time in priority order
    /// Task is considered to be a low-priority, if its priority is less than
    /// folly::Executor::MID_PRI
    /// @returns false if some tasks didn't get executed due to time limits
    bool run(double timeBudgetS, double lowPriorityTimeBudgetS);

    void clear();

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
