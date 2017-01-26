#include "TaskScheduler.hpp"

namespace Punctual
{

  TaskScheduler::TaskScheduler()
    : m_afterUpdate(std::make_shared<folly::ManualExecutor>()),
      m_beforeUpdate(std::make_shared<folly::ManualExecutor>()),
      m_beforeInput(std::make_shared<folly::ManualExecutor>())
  {
  }

  std::shared_ptr<folly::ManualExecutor> TaskScheduler::afterUpdate()
  {
    return m_afterUpdate;
  }

  std::shared_ptr<folly::ManualExecutor> TaskScheduler::beforeUpdate()
  {
    return m_beforeUpdate;
  }

  std::shared_ptr<folly::ManualExecutor> TaskScheduler::beforeInput()
  {
    return m_beforeInput;
  }


  DEFINE_SINGLETON(TaskScheduler)

}
