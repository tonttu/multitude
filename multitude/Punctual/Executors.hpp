#pragma once

#include "Export.hpp"

#include <folly/executors/ManualExecutor.h>

#include <Radiant/Singleton.hpp>

namespace Punctual
{
  PUNCTUAL_API folly::ManualExecutor * beforeInput();
  PUNCTUAL_API folly::ManualExecutor * afterUpdate();
  PUNCTUAL_API folly::ManualExecutor * beforeUpdate();
}
