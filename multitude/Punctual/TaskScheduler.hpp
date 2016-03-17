#ifndef TASKSCHEDULER_HPP
#define TASKSCHEDULER_HPP

#include "Export.hpp"

#include <folly/Executor.h>
#include <folly/futures/ManualExecutor.h>

#include <Radiant/Singleton.hpp>

namespace Punctual
{

  class PUNCTUAL_API TaskScheduler
  {
    DECLARE_SINGLETON(TaskScheduler);
  public:

    /// @todo Should probably have better protection for this.
    folly::ExecutorPtr afterUpdate();

  private:
    TaskScheduler();

    std::shared_ptr<folly::ManualExecutor> m_afterUpdate;
  };

}

#endif // TASKSCHEDULER_HPP
