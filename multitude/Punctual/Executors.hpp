#pragma once

#include "Export.hpp"
#include "LimitedTimeExecutor.hpp"

#include <folly/executors/ManualExecutor.h>

#include <Radiant/Singleton.hpp>

namespace Punctual
{
  /// Executed just before input() is called for the application root widget.
  /// Typically called roughly once per frame, but that can vary if there
  /// are no input samples to process, or if there are multiple ones.
  PUNCTUAL_API folly::ManualExecutor * beforeProcessInput();
  /// Executed once per frame before any input processing. Also called when no
  /// input processing is done. Called once per frame, even if input processes
  /// multiple samples.
  PUNCTUAL_API folly::ManualExecutor * beforeInput();
  PUNCTUAL_API folly::ManualExecutor * afterUpdate();
  PUNCTUAL_API folly::ManualExecutor * beforeUpdate();
  /// Executed once per frame in the main thread before starting render collect
  /// using the gfx driver.
  PUNCTUAL_API folly::ManualExecutor * beforeRender();
  /// Executed once per frame in the main thread after render collect has been
  /// finished.
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
