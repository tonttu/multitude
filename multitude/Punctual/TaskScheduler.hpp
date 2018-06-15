#ifndef TASKSCHEDULER_HPP
#define TASKSCHEDULER_HPP

#include "Export.hpp"

#include <folly/executors/ManualExecutor.h>

#include <Radiant/Singleton.hpp>

namespace Punctual
{

  class PUNCTUAL_API TaskScheduler
  {
    DECLARE_SINGLETON(TaskScheduler);
  public:

    /// Do not manually run this (only in Node::processQueue or MultiWidgets::Application)
    /// @todo consider this API, should we protect direct address
    /// to executors some clever way -- the challenge is that in order
    /// to work with folly::Futures there must be way accessable executor)
    std::shared_ptr<folly::ManualExecutor> beforeInput();
    std::shared_ptr<folly::ManualExecutor> afterUpdate();
    std::shared_ptr<folly::ManualExecutor> beforeUpdate();

  private:
    TaskScheduler();

    std::shared_ptr<folly::ManualExecutor> m_afterUpdate;
    std::shared_ptr<folly::ManualExecutor> m_beforeUpdate;
    std::shared_ptr<folly::ManualExecutor> m_beforeInput;
  };

}

#endif // TASKSCHEDULER_HPP
