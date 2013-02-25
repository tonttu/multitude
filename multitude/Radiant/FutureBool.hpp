#ifndef RADIANT_FUTUREBOOL_HPP
#define RADIANT_FUTUREBOOL_HPP

#include "Task.hpp"

namespace Radiant
{

  /// Interface class for FutureBool backends.
  class FutureBoolI
  {
  public:
    virtual ~FutureBoolI() {}
    /// Are all Radiant::Tasks finished that are associated with this object
    /// If the job we are waiting for doesn't use tasks, return true
    virtual bool isReady() const = 0;
    /// Return next task that need to be executed to get the job done
    virtual Radiant::TaskPtr task() const = 0;
    /// Once all tasks are ready, this returns the actual boolean value we want.
    /// This is only called once and might block.
    virtual bool validate() = 0;
  };
  typedef std::unique_ptr<FutureBoolI> FutureBoolIPtr;

  /// This class provides implicit conversion to boolean type. The class is
  /// used to provide asynchronous return values from functions.
  class FutureBool
  {
  public:
    FutureBool(bool value)
      : m_cached(value)
      , m_cacheSet(true)
    {
    }

    FutureBool(FutureBoolIPtr future)
      : m_future(std::move(future))
      , m_cached(false)
      , m_cacheSet(false)
    {}

    FutureBool(FutureBool && f)
      : m_future(std::move(f.m_future))
      , m_cached(f.m_cached)
      , m_cacheSet(f.m_cacheSet)
    {}

    FutureBool & operator=(FutureBool && f)
    {
      std::swap(m_future, f.m_future);
      m_cached = f.m_cached;
      m_cacheSet = f.m_cacheSet;
      return *this;
    }

    /// Implicit conversion to boolean. This function will block until the
    /// value of the future has been set.
    /// @return boolean value
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
