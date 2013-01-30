#ifndef RADIANT_FUTUREBOOL_HPP
#define RADIANT_FUTUREBOOL_HPP

#include "Task.hpp"

#include <future>

namespace Radiant {

  /// This class wraps a std::future boolean and provides implicit conversion
  /// to boolean type. The class is used to provide asynchronous return values
  /// from functions.
  class FutureBool
  {
  public:
    /// Construct a new FutureBool by taking ownership of the given std::future
    /// @param future boolean future to wrap
    explicit FutureBool(std::future<bool> && future)
      : m_future(std::move(future))
    {}

    /// Implicit conversion to boolean. This function will block until the
    /// value of the future has been set.
    /// @return boolean value
    operator bool()
    {
      try {
        return m_future.get();
      } catch(std::future_error &) {
        return false;
      }
    }

  private:
    std::future<bool> m_future;
  };

  class FutureBoolI
  {
  public:
    virtual bool isReady() const = 0;
    virtual bool validate() = 0;
    virtual Radiant::TaskPtr task() const = 0;
  };
  typedef std::unique_ptr<FutureBoolI> FutureBoolIPtr;

  class FutureBool2
  {
  public:
    FutureBool2(bool value)
      : m_cached(value)
      , m_cacheSet(true)
    {
    }

    FutureBool2(FutureBoolIPtr future)
      : m_future(std::move(future))
      , m_cached(false)
      , m_cacheSet(false)
    {}

    FutureBool2(FutureBool2 && f)
      : m_future(std::move(f.m_future))
      , m_cached(f.m_cached)
      , m_cacheSet(f.m_cacheSet)
    {}

    FutureBool2 & operator=(FutureBool2 && f)
    {
      std::swap(m_future, f.m_future);
      m_cached = f.m_cached;
      m_cacheSet = f.m_cacheSet;
      return *this;
    }

    inline operator bool()
    {
      if (m_cacheSet)
        return m_cached;

      m_cached = run();
      m_cacheSet = true;
      return m_cached;
    }

  private:
    bool run() const
    {
      while (!m_future->isReady()) {
        auto task = m_future->task();
        if (!task)
          break;
        task->runNow(true);
      }

      return m_future->validate();
    }

  private:
    FutureBoolIPtr m_future;
    bool m_cached;
    bool m_cacheSet;
  };

} // namespace Radiant

#endif // RADIANT_FUTUREBOOL_HPP
