#pragma once

#include <Radiant/Mutex.hpp>

#include <folly/futures/Future.h>

namespace Punctual
{
  /// Calls callback with values from all futures in order. The returned
  /// future is fulfilled once all futures are completed.
  /// @param executor must not be null
  template <typename T, typename Collection>
  folly::Future<folly::Unit> forEach(
      Collection && futures,
      std::function<void(T)> callback,
      folly::Executor * executor,
      bool stopOnError);

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename T, typename Collection>
  folly::Future<folly::Unit> forEach(
      Collection && futures,
      std::function<void(T)> callback,
      folly::Executor * executor,
      bool stopOnError)
  {
    if (futures.empty())
      return folly::makeFuture();

    struct Context
    {
      Context(size_t size, std::function<void(T)> && cb)
        : values(size)
        , callback(std::move(cb))
      {}

      Radiant::Mutex mutex;
      std::vector<folly::Try<T>> values;
      size_t next = 0;
      std::function<void(T)> callback;
      folly::Promise<folly::Unit> promise;
      bool failed = false;
    };
    auto ctx = std::make_shared<Context>(futures.size(), std::move(callback));
    auto future = ctx->promise.getFuture().via(executor);

    size_t idx = 0;
    for (auto & f: futures) {
      std::move(f).thenTry([ctx, idx, executor, stopOnError] (folly::Try<T> res) {
        Radiant::Guard g(ctx->mutex);
        if (ctx->failed)
          return;
        ctx->values[idx] = std::move(res);

        while (ctx->next < ctx->values.size()) {
          auto & t = ctx->values[ctx->next];
          if (t.hasValue()) {
            executor->add([ctx, v = std::move(t.value())] {
              ctx->callback(std::move(v));
            });
            ++ctx->next;
          } else if (t.hasException()) {
            if (stopOnError) {
              ctx->failed = true;
              ctx->promise.setException(t.exception());
              break;
            } else {
              ++ctx->next;
            }
          } else {
            break;
          }
        }

        if (ctx->next == ctx->values.size())
          ctx->promise.setValue();
      });
      ++idx;
    }
    return future;
  }
}
