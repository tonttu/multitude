#include "BGThreadExecutor.hpp"
#include <Radiant/Mutex.hpp>
#include <QThread>
#include <unordered_map>
#include <mutex>

namespace Radiant
{
  namespace
  {
    typedef BGThreadExecutor::Func Func;
    typedef BGThreadExecutor::JobId JobId;

    struct FuncTask : public Task
    {
      FuncTask(const Func & func, int8_t priority, Func && kill)
        : m_func(func), m_kill(std::move(kill))
      {
        // map priority from [-128, 127] to [0, 1000]
        setPriority(priority * 500.0f / 128 + 500);
      }

      void doTask() override
      {
        if(m_func)
          m_func();
        if(m_kill)
          m_kill();
        setFinished();
      }

      std::function<void()> m_func;
      std::function<void()> m_kill;
    };
  }  // unnamed namespace

  class BGThreadExecutor::D
  {
  public:
    D(const std::shared_ptr<BGThread> & bgThread)
      : m_bgThread(bgThread), m_jobId(0) { }

    JobId addWithPriority(const Func& func, int8_t priority)
    {
      JobId key = m_jobId++;
      auto kill = [this, key] {
        Guard guard(m_tasksMutex);
        m_tasks.erase(key);
      };
      auto taskPtr = std::make_shared<FuncTask>(func, priority, std::move(kill));
      {
        Guard guard(m_tasksMutex);
        m_tasks.emplace(key, taskPtr);
      }
      if(!m_bgThread->isRunning()) {
        m_bgThread->run(QThread::idealThreadCount());
      }
      m_bgThread->addTask(taskPtr);
      return key;
    }

    bool cancel(JobId id)
    {
      std::shared_ptr<FuncTask> ptr;
      {
        Guard guard(m_tasksMutex);
        auto it = m_tasks.find(id);
        if(it == m_tasks.end()) {
          return false;
        }
        ptr = it->second;
        m_tasks.erase(it);
      }
      return m_bgThread->removeTask(ptr, true, false);
    }

  private:
    std::shared_ptr<BGThread> m_bgThread;
    Radiant::Mutex m_tasksMutex;
    std::unordered_map<JobId, std::shared_ptr<FuncTask>> m_tasks;
    std::atomic<JobId> m_jobId;
  };

  BGThreadExecutor::BGThreadExecutor(const std::shared_ptr<BGThread> & bgThread)
    : m_d(new D(bgThread ? bgThread : BGThread::instance())) { }

  BGThreadExecutor::~BGThreadExecutor() { }

  JobId BGThreadExecutor::add(BGThreadExecutor::Func func)
  {
    return m_d->addWithPriority(func, folly::Executor::MID_PRI);
  }

  JobId BGThreadExecutor::addWithPriority(BGThreadExecutor::Func func, int8_t priority)
  {
    return m_d->addWithPriority(func, priority);
  }

  bool BGThreadExecutor::cancel(JobId id)
  {
    return m_d->cancel(id);
  }

  uint8_t BGThreadExecutor::getNumPriorities() const { return 255; }

  const std::shared_ptr<BGThreadExecutor>& BGThreadExecutor::instance()
  {
    static std::once_flag once;
    static std::shared_ptr<BGThreadExecutor> ptr;
    std::call_once(once, [] {
      ptr.reset(new BGThreadExecutor());
    });
    return ptr;
  }
}
