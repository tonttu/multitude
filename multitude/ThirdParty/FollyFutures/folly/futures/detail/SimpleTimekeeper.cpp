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
  if (thread_.joinable()) {
    thread_.join();
  }
}

Future<Unit> SimpleTimekeeper::after(Duration duration) {
  Key key;
  key.time = std::chrono::steady_clock::now() + duration;
  key.counter = counter_++;
  Promise<Unit> promise;
  promise.setInterruptHandler([this, key](const exception_wrapper&) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.erase(key);
  });
  auto result = promise.getFuture();
  std::lock_guard<std::mutex> lock(mutex_);
  assert(pending_.find(key) == pending_.end());
  pending_.emplace(std::make_pair(key, std::move(promise)));
  return result;
}

void SimpleTimekeeper::threadLoop() {
  while (continue_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
  static std::once_flag once;
  static std::shared_ptr<Timekeeper> ptr;
  std::call_once(once, [] {
    ptr = std::make_shared<SimpleTimekeeper>();
  });
  return ptr.get();
}

}  // namespace detail
}  // namespace folly
