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
  }  // unnamed namespace

  class ThreadPoolExecutor::D
  {
  public:
    D(const std::shared_ptr<QThreadPool> & pool)
      : m_threadPool(pool) { }
    void addWithPriority(ThreadPoolExecutor::Func func, int8_t priority);

  private:
    QThreadPool & pool();

    std::shared_ptr<QThreadPool> m_threadPool;
  };

  ThreadPoolExecutor::ThreadPoolExecutor(const std::shared_ptr<QThreadPool> & pool)
    : m_d(new D(pool)) { }

  ThreadPoolExecutor::~ThreadPoolExecutor() { }

  void ThreadPoolExecutor::add(ThreadPoolExecutor::Func func)
  {
    addWithPriority(std::move(func), 0);
  }

  void ThreadPoolExecutor::addWithPriority(ThreadPoolExecutor::Func func, int8_t priority)
  {
    m_d->addWithPriority(std::move(func), priority);
  }

  uint8_t ThreadPoolExecutor::getNumPriorities() const
  {
    return 255;
  }

  void ThreadPoolExecutor::D::addWithPriority(ThreadPoolExecutor::Func func, int8_t priority)
  {
    FuncRunnable *runnable = new FuncRunnable(std::move(func), nullptr);
    pool().start(runnable, priority);
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
