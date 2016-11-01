#include <folly/futures/detail/SimpleTimekeeper.h>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <cassert>

namespace folly { namespace detail {

SimpleTimekeeper::SimpleTimekeeper() 
  : counter_(0)
  , continue_(true)
  , thread_([this] { threadLoop(); }) { }

SimpleTimekeeper::~SimpleTimekeeper() {
  continue_ = false;
  cond_.notify_all();
  if (thread_.joinable()) {
    thread_.join();
  }
}

Future<Unit> SimpleTimekeeper::after(Duration duration) {
  Key key;
  key.time = std::chrono::steady_clock::now() + duration;
  key.counter = counter_++;
  Promise<Unit> promise;
  promise.setInterruptHandler([this, key](const exception_wrapper& e) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pending_.find(key);
    if (it != pending_.end()) {
      it->second.setException(e);
      pending_.erase(key);
    }
  });
  auto result = promise.getFuture();
  {
    std::lock_guard<std::mutex> lock(mutex_);
    assert(pending_.find(key) == pending_.end());
    pending_.emplace(std::make_pair(key, std::move(promise)));
  }
  cond_.notify_all();
  return result;
}

void SimpleTimekeeper::threadLoop() {
  while (continue_) {
    {
      // sleep until the first pending promise, or until a new promise
      // is added.
      std::unique_lock<std::mutex> guard(mutex_);
      if (pending_.empty()) {
        cond_.wait(guard);
      } else {
        auto until = pending_.begin()->first.time;
        cond_.wait_until(guard, until);
      }
    }
    auto now = std::chrono::steady_clock::now();
    setValues(now);
  }
}

void SimpleTimekeeper::setValues(std::chrono::time_point<std::chrono::steady_clock> now) {
  while (continue_) {
    Promise<Unit> promise;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (pending_.empty() || pending_.begin()->first.time > now) {
        break;
      } else {
        auto it = pending_.begin();
        promise = std::move(it->second);
        pending_.erase(it);
      }
    }
    promise.setValue();
  }
}

Timekeeper* getTimekeeperSingleton() {
  static auto ptr = std::make_shared<SimpleTimekeeper>();
  return ptr.get();
}

}  // namespace detail
}  // namespace folly
