#include "Executors.hpp"

namespace Punctual
{
  namespace
  {
    folly::ManualExecutor s_beforeInput;
    folly::ManualExecutor s_afterUpdate;
    folly::ManualExecutor s_beforeUpdate;
  }

  folly::ManualExecutor * beforeInput()
  {
    return &s_beforeInput;
  }

  folly::ManualExecutor * afterUpdate()
  {
    return &s_afterUpdate;
  }

  folly::ManualExecutor * beforeUpdate()
  {
    return &s_beforeUpdate;
  }
}