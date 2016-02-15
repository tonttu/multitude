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

#include <folly/futures/ManualExecutor.h>

#include <string.h>
#include <string>
#include <tuple>
#include <algorithm>
#include <stdexcept>

namespace folly {

JobId ManualExecutor::add(Func callback) {
  JobId key;
  {
    std::lock_guard<std::mutex> lock(lock_);
    key = jobId_++;
    funcs_.emplace_back(std::move(callback), key);
  }
  cond_.notify_all();
  return key;
}

bool ManualExecutor::cancel(JobId id) {
  std::lock_guard<std::mutex> lock(lock_);

  auto queuedMatch = [id] (const QueuedFunc & func) {
    return func.jobId == id;
  };
  auto newQueueEnd = std::remove_if(funcs_.begin(), funcs_.end(), queuedMatch);
  if (newQueueEnd != funcs_.end()) {
    funcs_.erase(newQueueEnd, funcs_.end());
    return true;
  }
  auto scheduledMatch = [id](const std::pair<const ScheduleKey, Func> & pair) {
    return pair.first.jobId == id;
  };
  for(auto it = scheduledFuncs_.begin(); it != scheduledFuncs_.end();) {
    if(scheduledMatch(*it)) {
      scheduledFuncs_.erase(it);
      return true;
    } else {
      ++it;
    }
  }
  return false;
}

size_t ManualExecutor::run() {
  size_t count;
  size_t n;
  Func func;

  {
    std::lock_guard<std::mutex> lock(lock_);

    while (!scheduledFuncs_.empty()) {
      auto it = scheduledFuncs_.begin();
      if (it->first.time > now_)
        break;
      funcs_.emplace_back(std::move(it->second), it->first.jobId);
      scheduledFuncs_.erase(it);
    }

    n = funcs_.size();
  }

  for (count = 0; count < n; count++) {
    {
      std::lock_guard<std::mutex> lock(lock_);
      if (funcs_.empty()) {
        break;
      }

      func = std::move(funcs_.front().func);
      funcs_.pop_front();
    }
    func();
  }

  return count;
}

void ManualExecutor::wait() {
  while (true) {
    std::unique_lock<std::mutex> lock(lock_);
    if (!funcs_.empty())
      break;
    cond_.wait(lock);
  }
}

void ManualExecutor::advanceTo(TimePoint const& t) {
  if (t > now_) {
    now_ = t;
  }
  run();
}

} // folly
