#include "Export.hpp"

#include <folly/executors/SequencedExecutor.h>

#include <Radiant/Mutex.hpp>

#include <queue>
#include <thread>

namespace Punctual
{
  /// Executor that spawns a thread when needed, and joins the thread when
  /// there is nothing to do. Meant for executors that rarely have anything
  /// to do but could have long-lasting operations that need to run in
  /// a sequence.
  class PUNCTUAL_API OnDemandExecutor : public folly::SequencedExecutor
  {
  public:
    virtual void add(folly::Func func) override;
    virtual ~OnDemandExecutor();

  private:
    void work();

  private:
    Radiant::Mutex m_workMutex;
    std::queue<folly::Func> m_queue;
    std::thread m_worker;
    bool m_running = true;
  };
}
