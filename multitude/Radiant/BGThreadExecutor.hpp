#ifndef RADIANT_BGTHREADEXECUTOR_HPP
#define RADIANT_BGTHREADEXECUTOR_HPP

#include <Radiant/Export.hpp>
#include <Radiant/Singleton.hpp>
#include <Radiant/BGThread.hpp>

#include <folly/Executor.h>

#include <memory>

namespace Radiant
{
  /// BGThread adapter that conforms to the folly::Executor interface.
  class RADIANT_API BGThreadExecutor : public folly::Executor
  {
  public:
    typedef folly::Func Func;
    typedef folly::JobId JobId;

    /// Will use the given BGThread instance or the global singleton one if nullptr
    explicit BGThreadExecutor(const std::shared_ptr<BGThread> & bgThread = nullptr);
    ~BGThreadExecutor();

    JobId add(Func) override;
    JobId addWithPriority(Func func, int8_t priority) override;
    bool cancel(JobId id) override;
    uint8_t getNumPriorities() const override;

    static const std::shared_ptr<BGThreadExecutor> & instance();

    class D;
  private:
    std::unique_ptr<D> m_d;
  };
}

#endif // RADIANT_BGTHREADEXECUTOR_HPP