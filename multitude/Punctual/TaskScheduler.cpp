#include "TaskScheduler.hpp"

namespace Punctual
{

  TaskScheduler::TaskScheduler()
    : m_afterUpdate(std::make_shared<folly::ManualExecutor>())
  {
  }

  std::shared_ptr<folly::ManualExecutor> TaskScheduler::afterUpdate()
  {
    return m_afterUpdate;
  }

  DEFINE_SINGLETON(TaskScheduler)

}
