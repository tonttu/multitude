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

    class FuncTask : public Task
    {
    public:
      template<class Arg>
      FuncTask(Arg && func, int8_t priority)
        : m_func(std::forward<Arg>(func))
      {
        float lowPriority = Task::PRIORITY_LOW;
        float normalPriority = Task::PRIORITY_NORMAL;
        float highPriority = Task::PRIORITY_URGENT;
        float intervalWidth = std::max(highPriority - normalPriority,
                                       normalPriority - lowPriority);
        float overshoot = 1.1f;
        // Map priority from [-128, 127] to
        // [NORMAL - width * overshoot, NORMAL + width * overshoot].
        // 0 priority must map to PRIORITY_NORMAL, and we need to be able
        // to go a bit over PRIORITY_URGENT and under PRIORITY_LOW.
        setPriority(priority / 128.0f * intervalWidth * overshoot + normalPriority);
      }

      void doTask() override
      {
        if(m_func)
          m_func();
        setFinished();
      }

    private:
      Func m_func;
    };
  }  // unnamed namespace

  class BGThreadExecutor::D
  {
  public:
    D(const std::shared_ptr<BGThread> & bgThread)
      : m_bgThread(bgThread) { }

    void addWithPriority(Func func, int8_t priority)
    {
      auto taskPtr = std::make_shared<FuncTask>(std::move(func), priority);
      m_bgThread->addTask(taskPtr);
    }

  private:
    std::shared_ptr<BGThread> m_bgThread;
  };

  BGThreadExecutor::BGThreadExecutor(const std::shared_ptr<BGThread> & bgThread)
    : m_d(new D(bgThread ? bgThread : BGThread::instance())) { }

  BGThreadExecutor::~BGThreadExecutor() { }

  void BGThreadExecutor::add(BGThreadExecutor::Func func)
  {
    m_d->addWithPriority(std::move(func), folly::Executor::MID_PRI);
  }

  void BGThreadExecutor::addWithPriority(BGThreadExecutor::Func func, int8_t priority)
  {
    m_d->addWithPriority(std::move(func), priority);
  }

  uint8_t BGThreadExecutor::getNumPriorities() const { return 255; }

  BGThreadExecutor * BGThreadExecutor::instance()
  {
    static BGThreadExecutor s_instance;
    return &s_instance;
  }

  BGThreadExecutor * BGThreadExecutor::instanceIo()
  {
    static BGThreadExecutor s_instance{BGThread::ioThreadPool()};
    return &s_instance;
  }
}
