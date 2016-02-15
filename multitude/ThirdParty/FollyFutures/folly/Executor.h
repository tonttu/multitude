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

#include "Export.h"

#include <atomic>
#include <climits>
#include <functional>
#include <stdexcept>
#include <memory>
#include <cstdint>

namespace folly {

typedef std::function<void()> Func;
typedef uint64_t JobId;

/// An Executor accepts units of work with add(), which should be
/// threadsafe.
class FOLLY_API Executor {
 public:
  virtual ~Executor() = default;

  /// Enqueue a function to executed by this executor. This and all
  /// variants must be threadsafe.
  ///
  /// Returns a job id that must be unique among all running and queued jobs.
  /// After a job is finished the id can be reused.
  virtual JobId add(Func) = 0;

  /// Will remove a function from the queue. Id is the result of an 'add' call.
  /// Job ids can be reused, so be wary of cancelling a finished job. To be safe
  /// you need to keep track of when the job finishes externally.
  ///
  /// Returns true if the job could be cancelled. For example if the job was
  /// in a queue but not started yet.
  virtual bool cancel(JobId) { return false; }

  /// Enqueue a function with a given priority, where 0 is the medium priority
  /// This is up to the implementation to enforce
  virtual JobId addWithPriority(Func, int8_t /*priority*/) {
    throw std::runtime_error(
        "addWithPriority() is not implemented for this Executor");
  }

  virtual uint8_t getNumPriorities() const {
    return 1;
  }

  static const int8_t LO_PRI  = SCHAR_MIN;
  static const int8_t MID_PRI = 0;
  static const int8_t HI_PRI  = SCHAR_MAX;

  /// A convenience function for shared_ptr to legacy functors.
  ///
  /// Sometimes you have a functor that is move-only, and therefore can't be
  /// converted to a std::function (e.g. std::packaged_task). In that case,
  /// wrap it in a shared_ptr (or maybe folly::MoveWrapper) and use this.
  template <class P>
  void addPtr(P fn) {
    this->add([fn]() mutable { (*fn)(); });
  }
};

typedef std::shared_ptr<Executor> ExecutorPtr;

/// Weak pointer that tracks if it was ever set with a non-null value.
/// Normal weak_ptrs don't know if they are uninitialized or pointing
/// to a dead shared_ptr.
template<class T>
class StickyWeakPtr {
 public:
  StickyWeakPtr() noexcept : isSet_(false) { }

  template<class U>
  StickyWeakPtr(const std::weak_ptr<U>& other, bool isSet) noexcept
    : isSet_(isSet), ptr_(other) { }

  template<class U>
  StickyWeakPtr(std::weak_ptr<U>&& other, bool isSet) noexcept
    : isSet_(isSet), ptr_(std::move(other)) { }

  template<class U>
  StickyWeakPtr(const StickyWeakPtr<U>& other) noexcept
    : isSet_(other.isSet_), ptr_(other.ptr_) { }

  template<class U>
  StickyWeakPtr(StickyWeakPtr<U>&& other) noexcept
    : isSet_(other.isSet_), ptr_(std::move(other.ptr_)) { }

  template<class U>
  StickyWeakPtr(const std::shared_ptr<U>& ptr) noexcept
    : isSet_(ptr), ptr_(ptr) { }

  template<class U>
  StickyWeakPtr<T>& operator=(const StickyWeakPtr<U>& other) noexcept {
    isSet_ = other.isSet_;
    ptr_ = other.ptr_;
    return *this;
  }

  template<class U>
  StickyWeakPtr<T>& operator=(StickyWeakPtr<U>&& other) noexcept {
    isSet_ = other.isSet_;
    ptr_ = std::move(other.ptr_);
    return *this;
  }

  template<class U>
  StickyWeakPtr<T>& operator=(const std::shared_ptr<U>& ptr) noexcept {
    isSet_ = static_cast<bool>(ptr);
    ptr_ = ptr;
    return *this;
  }

  void reset() noexcept {
    isSet_ = false;
    ptr_.reset();
  }

  void swap(StickyWeakPtr<T>& other) noexcept {
    using std::swap;
    swap(isSet_, other.isSet_);
    swap(ptr_, other.ptr_);
  }

  long use_count() const noexcept { return ptr_.use_count(); }
  bool expired() const noexcept { return ptr_.expired(); }
  std::shared_ptr<T> lock() const noexcept { return ptr_.lock(); }
  template<class U>
  bool owner_before(const U & other) const noexcept {
    return ptr_.owner_before(other);
  }

  bool wasEverAlive() const noexcept { return isSet_; }

  operator std::weak_ptr<T>() const noexcept { return ptr_; }

 private:
  bool isSet_;
  std::weak_ptr<T> ptr_;
};

typedef StickyWeakPtr<Executor> ExecutorWeakPtr;

} // folly
