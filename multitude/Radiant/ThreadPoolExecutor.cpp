#include "ThreadPoolExecutor.hpp"
#include "Mutex.hpp"

#include <QThreadPool>

#include <mutex>
#include <utility>
#include <atomic>
#include <unordered_map>
#include <cassert>

namespace Radiant
{
  namespace
  {
    class FuncRunnable : public QRunnable
    {
    public:
      template<class Arg>
      FuncRunnable(Arg && func, std::function<void()> && killFn)
        : m_func(std::forward<ThreadPoolExecutor::Func>(func))
        , m_suicide(killFn)
        , m_started(false)
      {
        setAutoDelete(false);
      }

      void run() override
      {
        if(m_func && tryMarkStarted()) {
          m_func();
        }
        suicide();
      }

      bool tryMarkStarted()
      {
        bool started = false;
        bool changed = m_started.compare_exchange_strong(started, true);
        return changed;
      }

      void suicide()
      {
        if(m_suicide) {
          m_suicide();
        }
      }

    private:
      ThreadPoolExecutor::Func m_func;
      std::function<void()> m_suicide;
      std::atomic<bool> m_started;
    };
  }  // unnamed namespace

  class ThreadPoolExecutor::D
  {
  public:
    D(const std::shared_ptr<QThreadPool> & pool) : m_threadPool(pool) { }
    JobId addWithPriority(ThreadPoolExecutor::Func func, int8_t priority);
    bool cancel(JobId id);

  private:
    QThreadPool & pool();

    std::shared_ptr<QThreadPool> m_threadPool;
    Mutex m_funcsMutex;
    std::unordered_map<JobId, std::unique_ptr<FuncRunnable>> m_funcs;
    std::atomic<JobId> m_jobId;
  };

  ThreadPoolExecutor::ThreadPoolExecutor(const std::shared_ptr<QThreadPool> & pool)
    : m_d(new D(pool)) { }

  ThreadPoolExecutor::~ThreadPoolExecutor() { }

  JobId ThreadPoolExecutor::add(ThreadPoolExecutor::Func func)
  {
    return addWithPriority(func, 0);
  }

  JobId ThreadPoolExecutor::addWithPriority(ThreadPoolExecutor::Func func, int8_t priority)
  {
    return m_d->addWithPriority(std::move(func), priority);
  }

  bool ThreadPoolExecutor::cancel(JobId id)
  {
    return m_d->cancel(id);
  }

  uint8_t ThreadPoolExecutor::getNumPriorities() const
  {
    return 255;
  }

  JobId ThreadPoolExecutor::D::addWithPriority(ThreadPoolExecutor::Func func, int8_t priority)
  {
    JobId key = m_jobId++;
    auto kill = [this, key] {
      Guard guard(m_funcsMutex);
      m_funcs.erase(key);
    };
    FuncRunnable *runnable = new FuncRunnable(std::move(func), std::move(kill));
    auto runnablePtr = std::unique_ptr<FuncRunnable>(runnable);
    {
      Guard guard(m_funcsMutex);
      assert(m_funcs.find(key) == m_funcs.end());
      m_funcs.emplace(key, std::move(runnablePtr));
    }
    pool().start(runnable, priority);
    return key;
  }

  bool ThreadPoolExecutor::D::cancel(JobId id)
  {
    Guard guard(m_funcsMutex);
    auto it = m_funcs.find(id);
    if(it == m_funcs.end()) {
      return false;
    }
    FuncRunnable& func = *it->second.get();
    bool stopped = func.tryMarkStarted();
    if(stopped) {
      pool().cancel(&func);
      return true;
    }
    return false;
  }

  QThreadPool & ThreadPoolExecutor::D::pool()
  {
    return m_threadPool.get() ? *m_threadPool : *QThreadPool::globalInstance();
  }

  ThreadPoolExecutor & ThreadPoolExecutor::instance()
  {
    static std::once_flag once;
    static std::unique_ptr<ThreadPoolExecutor> ptr;
    std::call_once(once, [] {
      ptr.reset(new ThreadPoolExecutor());
    });
    return *ptr;
  }
}
