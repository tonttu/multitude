#include "TaskScheduler.hpp"

namespace Punctual
{

  folly::ManualExecutor* TaskScheduler::afterUpdate()
  {
    return &m_afterUpdate;
  }

  folly::ManualExecutor* TaskScheduler::beforeUpdate()
  {
    return &m_beforeUpdate;
  }

  folly::ManualExecutor* TaskScheduler::beforeInput()
  {
    return &m_beforeInput;
  }

  DEFINE_SINGLETON(TaskScheduler)

}
