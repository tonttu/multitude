#pragma once

#include "Export.hpp"
#include "LimitedTimeExecutor.hpp"

#include <folly/executors/ManualExecutor.h>

#include <Radiant/Singleton.hpp>

namespace Punctual
{
  PUNCTUAL_API folly::ManualExecutor * beforeInput();
  PUNCTUAL_API folly::ManualExecutor * afterUpdate();
  PUNCTUAL_API folly::ManualExecutor * beforeUpdate();
  PUNCTUAL_API folly::ManualExecutor * beforeRender();
  PUNCTUAL_API folly::ManualExecutor * afterRender();

  /// This executor is driven by MultiWidgets::Application with a limited time
  /// budget per frame. Unless you need to get a task executed immediately, use
  /// this executor instead of afterUpdate/beforeUpdate executors. On slower
  /// frames there will be less or none time available. This is driven after
  /// afterUpdate() but before render collect starts.
  PUNCTUAL_API LimitedTimeExecutor * mainThread();

  /// @todo this assumes T is some kind of smart pointer
  template <typename T>
  void deleteLaterInMainThread(T && t)
  {
    mainThread()->addWithPriority([v = std::move(t)] () mutable {
      v.reset();
    }, folly::Executor::LO_PRI);
  }
}
