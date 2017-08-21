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
        setAutoDelete(true);
      }

      ~FuncRunnable()
      {
        suicide();
      }

      void run() override
      {
        if(m_func && tryMarkStarted()) {
          m_func();
        }
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

    struct Funcs
    {
      Mutex mutex;
      // QThreadPool handles the lifetime of FuncRunnable so we can have
      // naked pointers here.
      std::unordered_map<JobId, FuncRunnable*> funcs;
    };
  }  // unnamed namespace

  class ThreadPoolExecutor::D
  {
  public:
    D(const std::shared_ptr<QThreadPool> & pool)
      : m_threadPool(pool), m_funcs(new Funcs()) { }
    JobId addWithPriority(ThreadPoolExecutor::Func func, int8_t priority);
    bool cancel(JobId id);

  private:
    QThreadPool & pool();

    std::shared_ptr<QThreadPool> m_threadPool;
    std::atomic<JobId> m_jobId{0};
    std::shared_ptr<Funcs> m_funcs;
  };

  ThreadPoolExecutor::ThreadPoolExecutor(const std::shared_ptr<QThreadPool> & pool)
    : m_d(new D(pool)) { }

  ThreadPoolExecutor::~ThreadPoolExecutor() { }

  JobId ThreadPoolExecutor::add(ThreadPoolExecutor::Func func)
  {
    return addWithPriority(std::move(func), 0);
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
    std::weak_ptr<Funcs> weakFuncs = m_funcs;
    auto kill = [weakFuncs, key] {
      auto ptr = weakFuncs.lock();
      if(ptr) {
        Guard guard(ptr->mutex);
        ptr->funcs.erase(key);
      }
    };
    FuncRunnable *runnable = new FuncRunnable(std::move(func), std::move(kill));
    {
      Guard guard(m_funcs->mutex);
      assert(m_funcs->funcs.find(key) == m_funcs->funcs.end());
      m_funcs->funcs.emplace(key, runnable);
    }
    pool().start(runnable, priority);
    return key;
  }

  bool ThreadPoolExecutor::D::cancel(JobId id)
  {
    bool stopped = false;
    FuncRunnable * func = nullptr;
    {
      Guard guard(m_funcs->mutex);
      auto it = m_funcs->funcs.find(id);
      if(it == m_funcs->funcs.end()) {
        return false;
      }
      func = it->second;
      stopped = func->tryMarkStarted();
      m_funcs->funcs.erase(it);
    }
    if(stopped && func) {
      // Call pool.cancel() outside the lock, else we can deadlock like this:
      // 1. pool holds an internal lock while executing the runnable
      // 2. the executing runnable waits for m_funcs->mutex to remove itself from the map
      // 3. we are holding m_funcs->mutex because we are cancelling and finally
      // 4. when calling pool().cancel() we are waiting for the internal pool lock
      pool().cancel(func);
      return true;
    }
    return false;
  }

  QThreadPool & ThreadPoolExecutor::D::pool()
  {
    return m_threadPool.get() ? *m_threadPool : *QThreadPool::globalInstance();
  }

  const std::shared_ptr<ThreadPoolExecutor>& ThreadPoolExecutor::instance()
  {
    static auto ptr = std::make_shared<ThreadPoolExecutor>();
    return ptr;
  }
}
