#pragma once

#include <Radiant/Mutex.hpp>

#include <folly/futures/Future.h>

#include <QStringList>

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

  /// Newer folly (which is in use on macOS and Windows) changed how collectAll
  /// works, so our code relying on old behaviour needs to use
  /// folly::collectAllUnsafe.
  /// However, this function is not available on our folly version on Ubuntu,
  /// so use this wrapper instead.
  /// Folly also doesn't seem to have any kind of version macro.
#ifndef RADIANT_LINUX
  template <class T>
  inline auto collectAllUnsafe(T && c) -> decltype(folly::collectAllUnsafe(c))
  {
    return folly::collectAllUnsafe(std::forward<T>(c));
  }
  template <class T>
  inline auto collectUnsafe(T && c) -> decltype(folly::collectUnsafe(c))
  {
    return folly::collectUnsafe(std::forward<T>(c));
  }
#else
  template <class T>
  inline auto collectAllUnsafe(T && c) -> decltype(folly::collectAll(c))
  {
    return folly::collectAll(std::forward<T>(c));
  }
  template <class T>
  inline auto collectUnsafe(T && c) -> decltype(folly::collect(c))
  {
    return folly::collect(std::forward<T>(c));
  }
#endif

  /// Finishes all futures, and then:
  ///  - returns folly::Unit if nothing failed,
  ///  - returns the error if there is just one error, or
  ///  - returns an std::runtime_error with combined unique error messages of all errors
  template <class T>
  inline folly::Future<folly::Unit> collectErrors(T && c)
  {
    return Punctual::collectAllUnsafe(std::forward<T>(c)).thenValue([] (auto v) {
      int errs = 0;
      for (auto & t: v)
        if (t.hasException())
          ++errs;

      if (errs == 0)
        return folly::Unit();

      if (errs == 1)
        for (auto & t: v)
          t.throwIfFailed();

      QStringList messages;
      for (auto & t: v) {
        if (auto ex = t.tryGetExceptionObject()) {
          QString msg = ex->what();
          if (!messages.contains(msg))
            messages << msg;
        }
      }

      if (messages.isEmpty())
        throw std::runtime_error("Operation failed");

      throw std::runtime_error("Operation failed: " + messages.join(", ").toStdString());
    });
  }

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
