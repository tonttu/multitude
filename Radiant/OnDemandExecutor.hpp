/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "Export.hpp"

#include <folly/executors/SequencedExecutor.h>

#include <Radiant/Mutex.hpp>

#include <queue>
#include <thread>

namespace Radiant
{
  /// Executor that spawns a thread when needed, and joins the thread when
  /// there is nothing to do. Meant for executors that rarely have anything
  /// to do but could have long-lasting operations that need to run in
  /// a sequence.
  class RADIANT_API OnDemandExecutor : public folly::SequencedExecutor
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
