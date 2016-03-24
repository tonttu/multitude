#ifndef TASKSCHEDULER_HPP
#define TASKSCHEDULER_HPP

#include "Export.hpp"

#include <folly/futures/ManualExecutor.h>

#include <Radiant/Singleton.hpp>

namespace Punctual
{

  class PUNCTUAL_API TaskScheduler
  {
    DECLARE_SINGLETON(TaskScheduler);
  public:

    /// Do not manually run this (only in Node::processQueue)
    /// @todo consider this API, should we protect direct address
    /// to executors some clever way -- the challenge is that in order
    /// to work with folly::Futures there must be way accessable executor)
    std::shared_ptr<folly::ManualExecutor> afterUpdate();

  private:
    TaskScheduler();

    std::shared_ptr<folly::ManualExecutor> m_afterUpdate;
  };

}

#endif // TASKSCHEDULER_HPP
