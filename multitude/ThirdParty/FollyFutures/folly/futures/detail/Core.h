/*
 * Copyright 2015 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <cassert>

#include <folly/Optional.h>
#include <folly/MicroSpinLock.h>

#include <folly/futures/Try.h>
#include <folly/futures/Promise.h>
#include <folly/futures/Future.h>
#include <folly/Executor.h>
#include <folly/futures/detail/FSM.h>
#include <folly/ScopeGuard.h>
#include <folly/futures/FutureException.h>

namespace folly { namespace detail {

/*
        OnlyCallback
       /            \
  Start              Armed - Done
       \            /
         OnlyResult

This state machine is fairly self-explanatory. The most important bit is
that the callback is only executed on the transition from Armed to Done,
and that transition can happen immediately after transitioning from Only*
to Armed, if it is active (the usual case).
*/
enum class State : uint8_t {
  Start,
  OnlyResult,
  OnlyCallback,
  Armed,
  Done,
};

class Cancellable {
 public:
  virtual void attachOne() = 0;
  virtual void detachOne() = 0;
  virtual bool cancel() = 0;
  virtual ~Cancellable() { }
};

/// The shared state object for Future and Promise.
/// Some methods must only be called by either the Future thread or the
/// Promise thread. The Future thread is the thread that currently "owns" the
/// Future and its callback-related operations, and the Promise thread is
/// likewise the thread that currently "owns" the Promise and its
/// result-related operations. Also, Futures own interruption, Promises own
/// interrupt handlers. Unfortunately, there are things that users can do to
/// break this, and we can't detect that. However if they follow move
/// semantics religiously wrt threading, they should be ok.
///
/// It's worth pointing out that Futures and/or Promises can and usually will
/// migrate between threads, though this usually happens within the API code.
/// For example, an async operation will probably make a Promise, grab its
/// Future, then move the Promise into another thread that will eventually
/// fulfill it. With executors and via, this gets slightly more complicated at
/// first blush, but it's the same principle. In general, as long as the user
/// doesn't access a Future or Promise object from more than one thread at a
/// time there won't be any problems.
template<typename T>
class Core : public Cancellable {
  static_assert(!std::is_void<T>::value,
                "void futures are not supported. Use Unit instead.");
 public:
  /// This must be heap-constructed. There's probably a way to enforce that in
  /// code but since this is just internal detail code and I don't know how
  /// off-hand, I'm punting.
  Core() : result_(), fsm_(State::Start), attached_(2) {}

  explicit Core(Try<T>&& t)
    : result_(std::move(t)),
      fsm_(State::OnlyResult),
      attached_(1) {}

  ~Core() {
    assert(attached_ == 0);
  }

  // not copyable
  Core(Core const&) = delete;
  Core& operator=(Core const&) = delete;

  // not movable (see comment in the implementation of Future::then)
  Core(Core&&) noexcept = delete;
  Core& operator=(Core&&) = delete;

  /// May call from any thread
  bool hasResult() const {
    switch (fsm_.getState()) {
      case State::OnlyResult:
      case State::Armed:
      case State::Done:
        assert(!!result_);
        return true;

      default:
        return false;
    }
  }

  /// May call from any thread
  bool ready() const {
    return hasResult();
  }

  /// May call from any thread
  Try<T>& getTry() {
    if (ready()) {
      return *result_;
    } else {
      throw FutureNotReady();
    }
  }

  template <typename F>
  class LambdaBufHelper {
   public:
    template <typename FF>
    explicit LambdaBufHelper(FF&& func) : func_(std::forward<FF>(func)) {}
    void operator()(Try<T>&& t) {
      SCOPE_EXIT { this->~LambdaBufHelper(); };
      func_(std::move(t));
    }
   private:
    F func_;
  };

  /// Call only from Future thread.
  template <typename F>
  void setCallback(F func) {
    bool transitionToArmed = false;
    auto setCallback_ = [&]{
      // Move the lambda into the Core if it fits
      if (sizeof(LambdaBufHelper<F>) <= lambdaBufSize) {
        auto funcLoc = reinterpret_cast<LambdaBufHelper<F>*>(&lambdaBuf_);
        new (funcLoc) LambdaBufHelper<F>(std::forward<F>(func));
        callback_ = std::ref(*funcLoc);
      } else {
        callback_ = std::move(func);
      }
    };

    FSM_START(fsm_)
      case State::Start:
        FSM_UPDATE(fsm_, State::OnlyCallback, setCallback_);
        break;

      case State::OnlyResult:
        FSM_UPDATE(fsm_, State::Armed, setCallback_);
        transitionToArmed = true;
        break;

      case State::OnlyCallback:
      case State::Armed:
      case State::Done:
        throw std::logic_error("setCallback called twice");
    FSM_END

    // we could always call this, it is an optimization to only call it when
    // it might be needed.
    if (transitionToArmed) {
      maybeCallback();
    }
  }

  /// Call only from Promise thread
  void setResult(Try<T>&& t) {
    bool transitionToArmed = false;
    auto setResult_ = [&]{
      // Bypass executor if set with FutureCancellation,
      // even if cancel was not called explicitly.
      auto onCancel = [&](FutureCancellation&) {
        // don't hold the cancelMutex since we're already holding
        // the fsm mutex and we must always lock cancelMutex first.
        // But the only reader of cancelled_ is doCallback which
        // can't execute now.
        //
        // Also, we don't want to cancel prev since it either does
        // not exist or it ran already if we have a value.
        cancelled_ = true;
      };
      t.template withException<FutureCancellation>(onCancel);
      result_ = std::move(t);
    };
    FSM_START(fsm_)
      case State::Start:
        FSM_UPDATE(fsm_, State::OnlyResult, setResult_);
        break;

      case State::OnlyCallback:
        FSM_UPDATE(fsm_, State::Armed, setResult_);
        transitionToArmed = true;
        break;

      case State::OnlyResult:
      case State::Armed:
      case State::Done:
        throw std::logic_error("setResult called twice");
    FSM_END

    if (transitionToArmed) {
      maybeCallback();
    }
  }

  /// Called by a destructing Future (in the Future thread, by definition)
  void detachFuture() {
    activate();
    detachOne();
  }

  /// Called by a destructing Promise (in the Promise thread, by definition)
  void detachPromise() {
    // detachPromise() and setResult() should never be called in parallel
    // so we don't need to protect this.
    if (UNLIKELY(!result_)) {
      setResult(Try<T>(exception_wrapper(BrokenPromise())));
    }
    detachOne();
  }

  /// May call from any thread
  void deactivate() {
    active_.store(false, std::memory_order_release);
  }

  /// May call from any thread
  void activate() {
    active_.store(true, std::memory_order_release);
    maybeCallback();
  }

  /// May call from any thread
  bool isActive() { return active_.load(std::memory_order_acquire); }

  /// Call only from Future thread
  void setExecutor(const ExecutorWeakPtr& x, int8_t priority = Executor::MID_PRI) {
    if (!executorLock_.try_lock()) {
      executorLock_.lock();
    }
    setExecutorNoLock(x, priority);
    executorLock_.unlock();
  }

  void setExecutorNoLock(const ExecutorWeakPtr& x, int8_t priority = Executor::MID_PRI) {
    executor_ = x;
    priority_ = priority;
  }

  ExecutorWeakPtr getExecutor() {
    return executor_;
  }

  /// Call only from Future thread
  void raise(exception_wrapper e) {
    if (!interruptLock_.try_lock()) {
      interruptLock_.lock();
    }
    if (!interrupt_ && !hasResult()) {
      interrupt_ = folly::make_unique<exception_wrapper>(std::move(e));
      if (interruptHandler_) {
        interruptHandler_(*interrupt_);
      }
    }
    interruptLock_.unlock();
  }

  std::function<void(exception_wrapper const&)> getInterruptHandler() {
    if (!interruptHandlerSet_.load(std::memory_order_acquire)) {
      return nullptr;
    }
    if (!interruptLock_.try_lock()) {
      interruptLock_.lock();
    }
    auto handler = interruptHandler_;
    interruptLock_.unlock();
    return handler;
  }

  /// Call only from Promise thread
  void setInterruptHandler(std::function<void(exception_wrapper const&)> fn) {
    if (!interruptLock_.try_lock()) {
      interruptLock_.lock();
    }
    if (!hasResult()) {
      if (interrupt_) {
        fn(*interrupt_);
      } else {
        setInterruptHandlerNoLock(std::move(fn));
      }
    }
    interruptLock_.unlock();
  }

  void setInterruptHandlerNoLock(
      std::function<void(exception_wrapper const&)> fn) {
    interruptHandlerSet_.store(true, std::memory_order_relaxed);
    interruptHandler_ = std::move(fn);
  }

  template<class U>
  void chainFrom(Core<U> & prev) {
    prev_ = &prev;
    setInterruptHandlerNoLock(prev.getInterruptHandler());
    setExecutorNoLock(prev.getExecutor());
  }

  bool isCancelled() const {
    return cancelled_;
  }

  bool cancelPrev() {
    bool result = false;

    auto keepPrevAlive = [this] {
      // While holding the lock, inc prev's reference count. This ensures prev is alive
      // while calling cancel, since it can't die while the callback is executing and the
      // callback can't exit because we're holding the FSM lock.
      prev_->attachOne();
    };

    auto doCancelPrev = [this, &result]{
      SCOPE_EXIT { prev_->detachOne(); };
      result = prev_->cancel();
    };

    // Only attempt to cancel prev if our result is not set. This means that prev must
    // be alive.
    FSM_START(fsm_)
      case State::Start:
        FSM_RUN(fsm_, keepPrevAlive, doCancelPrev);
        break;

      case State::OnlyCallback:
        FSM_RUN(fsm_, keepPrevAlive, doCancelPrev);
        break;

      default:
        FSM_BREAK
    FSM_END

    return result;
  }

  bool cancel() override {
    if (cancelled_) {
      // If we are calling cancel a second time, we might want to cancel
      // something that is happening further up the chain. Maybe more futures
      // were chained since the last call. We can only return true if we
      // are certain that we can insert a future cancellation. This can only
      // ever happen if our callback was not yet started.
      //
      // If our callback did start already it means either the old cancellation
      // was before or after the callback. If the old cancellation was before,
      // then we're fine. If it was after, it will have tried to cancel the
      // callback. Whether it failed or not it does not matter, we can do
      // nothing more.
      //
      // So we can only return true if callback has not started and we are
      // certain nothing but FutureCancellation can leave this promise.
      std::unique_lock<std::mutex> guard(cancelMutex_);
      return !callbackStarted_;
    }

    if (prev_) {
      bool done = cancelPrev();
      if (done) {
        // Only need to cancel in one place. If we inserted a
        // FutureCancellation somewhere we can stop since it will propagate.
        return true;
      }
    }

    cancelled_ = true;
    // Tell the source promise that we got cancelled. Don't hold the
    // cancelMutex while doing so, else through setResult or by somehow
    // calling cancel recursively we might deadlock.
    raise(FutureCancellation());

    std::unique_lock<std::mutex> guard(cancelMutex_);
    if(!callbackStarted_) {
      // we set cancelled_ and the doCallback code will test that and emit
      // a FutureCancellation
      return true;
    }

    // Callback has started, let's see if we can stop it
    auto executor = executorRunningCallback_.lock();
    if (executor) {
      bool ok = executor->cancel(executorJobId_);
      if (ok) {
        executorRunningCallback_.reset();
        // don't hold the lock while executing the callback
        guard.unlock();
        // Executor will not run callback anymore.
        // Resolve manually with FutureCancellation.
        SCOPE_EXIT { callback_ = nullptr; };
        callback_(Try<T>(exception_wrapper(FutureCancellation())));
      }
      // If we did not manage to stop the executor, we can't guarantee the
      // cancellation will occur.
      return ok;
    } else {
      // If there is no executor it means either the promise thread will run
      // the callback and we can't stop it, or the executor has finished and
      // we're too late. Either way, we can't guarantee that the cancellation
      // has been taken into account.
      return false;
    }
  }

 protected:
  void maybeCallback() {
    FSM_START(fsm_)
      case State::Armed:
        if (active_.load(std::memory_order_acquire)) {
          FSM_UPDATE2(fsm_, State::Done, []{}, [this]{ this->doCallback(); });
        }
        FSM_BREAK

      default:
        FSM_BREAK
    FSM_END
  }

  void doCallback() {
    ExecutorWeakPtr weak;
    int8_t priority;
    if (!executorLock_.try_lock()) {
      executorLock_.lock();
    }
    weak = executor_;
    priority = priority_;
    executorLock_.unlock();

    std::unique_lock<std::mutex> guard(cancelMutex_);
    callbackStarted_ = true;

    auto runCallbackNow = [&](Try<T>&& t) {
      // don't hold the lock while executing the callback
      guard.unlock();
      // clear callback so any captured stuff is released
      SCOPE_EXIT { callback_ = nullptr; };
      callback_(std::move(t));
    };

    if (cancelled_) {
      // bypass the executor if cancelled
      runCallbackNow(Try<T>(exception_wrapper(FutureCancellation())));
      return;
    }

    if (weak.wasEverAlive()) {
      ExecutorPtr x = weak.lock();
      if (!x) {
        // executor_ was alive at some point but it eventually died.
        runCallbackNow(Try<T>(exception_wrapper(DeadExecutor())));
        return;
      }

      // keep Core alive until executor did its thing
      ++attached_;
      try {
        executorRunningCallback_ = weak;

        auto runCallbackOnExecutor = [this]() {
          SCOPE_EXIT { detachOne(); };
          // After the callback finishes, job id can be reused. Make sure
          // we don't try to cancel a reused job id.
          SCOPE_EXIT { executorRunningCallback_.reset(); };
          // clear callback so any captured stuff is released
          SCOPE_EXIT { callback_ = nullptr; };
          callback_(std::move(*result_));
        };

        if (LIKELY(x->getNumPriorities() == 1)) {
          executorJobId_ = x->add(runCallbackOnExecutor);
        } else {
          executorJobId_ = x->addWithPriority(runCallbackOnExecutor, priority);
        }
      } catch (...) {
        executorRunningCallback_.reset();
        --attached_; // Account for extra ++attached_ before try
        result_ = Try<T>(exception_wrapper(std::current_exception()));
        runCallbackNow(std::move(*result_));
      }
    } else {
      runCallbackNow(std::move(*result_));
    }
  }

  void detachOne() override {
    auto a = --attached_;
    assert(a >= 0);
    if (a == 0) {
      delete this;
    }
  }

  void attachOne() override {
    ++attached_;
  }

  // lambdaBuf occupies exactly one cache line
  static constexpr size_t lambdaBufSize = 8 * sizeof(void*);
  typename std::aligned_storage<lambdaBufSize>::type lambdaBuf_;
  // place result_ next to increase the likelihood that the value will be
  // contained entirely in one cache line
  folly::Optional<Try<T>> result_;
  std::function<void(Try<T>&&)> callback_ {nullptr};
  FSM<State> fsm_;
  std::atomic<unsigned char> attached_;
  std::atomic<bool> active_ {true};
  std::atomic<bool> interruptHandlerSet_ {false};
  folly::MicroSpinLock interruptLock_ {0};
  folly::MicroSpinLock executorLock_ {0};
  int8_t priority_ {-1};
  // You might expect this to be a shared_ptr not a weak_ptr. But that
  // would make it possible for Core to be the only thing keeping the
  // executor alive. So when the executor executes the callback it will
  // in effect kill itself, which will lead to all sorts of nice crashes
  // and deadlocks.
  ExecutorWeakPtr executor_;
  std::unique_ptr<exception_wrapper> interrupt_ {};
  std::function<void(exception_wrapper const&)> interruptHandler_ {nullptr};

  // Keep track of previous promise that chains its result into this one.
  // This is to enable cancellation.
  Cancellable * prev_ { nullptr };
  // Protects executorJobId_, executorHandlingCallback_, cancelled_.
  // Always lock this first, then fsm lock or executor lock.
  //
  // The mutex is held in cancel() and doCallback(). This ensures that as
  // the cancellation signal moves from the end of the chain to the beginning,
  // and the value moves from beginning to end, they can't bypass each other
  // due to race conditions.
  std::mutex cancelMutex_;
  // Need to copy executor ptr when we start the callback since it can be
  // changed afterwards. This is only set while the callback is on the
  // executor - either queued or running.
  ExecutorWeakPtr executorRunningCallback_;
  JobId executorJobId_ {0};
  // Atomic not naked bool so we can set it during setResult without holding
  // the cancelMutex (since we're holding the fsm lock there already and we
  // don't want to deadlock by locking in wrong order).
  //
  // Need a boolean to keep track of cancellation state since we might not
  // be able to cancel the prev callback, if the prev executor has already
  // started it. In that case, prev might setResult in the future, after
  // the call to cancel returns.
  std::atomic<bool> cancelled_ {false};
  // Need to know in which order the critical sections in cancel and doCallback
  // are executed. doCallback is only ever executed once and it sets this bool.
  bool callbackStarted_ {false};
};

class CancelManyContext {
 public:
  template<class T>
  CancelManyContext(Promise<T>& p, size_t n = 0)
  {
    setInterruptHandler(p);
    if (n)
      cores_.reserve(n);
  }

  void addCore(Cancellable* ptr) {
    cores_.push_back(ptr);
  }

  template<class Iterator>
  void addCores(Iterator begin, Iterator end) {
    for(auto it = begin; it != end; ++it) {
      cores_.push_back(it->cancellable());
    }
  }

  void done(size_t i) {
    folly::MSLGuard guard(cancelLock_);
    cores_[i] = nullptr;
  }

  void cancel() {
    for (auto core : cores_) {
      // Don't hold the lock while calling cancel, else
      // might deadlock through the callback recursing into done()
      Cancellable* ptr = nullptr;
      {
        folly::MSLGuard guard(cancelLock_);
        if (core) {
          core->attachOne();
          ptr = core;
        }
      }
      if (ptr) {
        SCOPE_EXIT { core->detachOne(); };
        core->cancel();
      }
    }
  }

 private:
  template<class T>
  void setInterruptHandler(Promise<T>& p) {
    p.setInterruptHandler([this](const exception_wrapper& e) {
      e.with_exception<FutureCancellation>([&](const FutureCancellation&) {
        cancel();
      });
    });
  }

  folly::MicroSpinLock cancelLock_ {0};
  std::vector<Cancellable*> cores_;
};

template <typename... Ts>
struct CollectAllVariadicContext {
  CollectAllVariadicContext() : cancelMany(p) {}
  template <typename T, size_t I>
  inline void setPartialResult(Try<T>& t) {
    cancelMany.done(I);
    std::get<I>(results) = std::move(t);
  }
  ~CollectAllVariadicContext() {
    p.setValue(std::move(results));
  }
  Promise<std::tuple<Try<Ts>...>> p;
  std::tuple<Try<Ts>...> results;
  typedef Future<std::tuple<Try<Ts>...>> type;
  CancelManyContext cancelMany;
};

template <typename... Ts>
struct CollectVariadicContext {
  CollectVariadicContext() : cancelMany(p) { }
  template <typename T, size_t I>
  inline void setPartialResult(Try<T>& t) {
    cancelMany.done(I);
    if (t.hasException()) {
      if (!threw.exchange(true)) {
        p.setException(std::move(t.exception()));
      }
    } else if (!threw) {
      std::get<I>(results) = std::move(t.value());
    }
  }
  ~CollectVariadicContext() {
    if (!threw.exchange(true)) {
      p.setValue(std::move(results));
    }
  }
  Promise<std::tuple<Ts...>> p;
  std::tuple<Ts...> results;
  std::atomic<bool> threw {false};
  CancelManyContext cancelMany;
  typedef Future<std::tuple<Ts...>> type;
};

template <template <typename ...> class T, typename... Ts>
void collectVariadicHelper(const std::shared_ptr<T<Ts...>>&) {
  // base case
}

template <template <typename ...> class T, typename... Ts,
          typename THead, typename... TTail>
void collectVariadicHelper(const std::shared_ptr<T<Ts...>>& ctx,
                           THead&& head, TTail&&... tail) {
  ctx->cancelMany.addCore(head.cancellable());
  head.setCallback_([ctx](Try<typename THead::value_type>&& t) {
    ctx->template setPartialResult<typename THead::value_type,
                                   sizeof...(Ts) - sizeof...(TTail) - 1>(t);
  });
  // template tail-recursion
  collectVariadicHelper(ctx, std::forward<TTail>(tail)...);
}

}} // folly::detail
