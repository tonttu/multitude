/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_THREADPOOLEXECUTOR_HPP
#define RADIANT_THREADPOOLEXECUTOR_HPP

#include <Radiant/Export.hpp>
#include <folly/Executor.h>
#include <QThreadPool>
#include <memory>

namespace Radiant
{
  class RADIANT_API ThreadPoolExecutor : public folly::Executor
  {
  public:
    typedef folly::Func Func;

    /// @param threadPool - uses the given thread pool. If null, uses the global
    /// QThreadPool pool
    ThreadPoolExecutor(const std::shared_ptr<QThreadPool> & threadPool = nullptr);
    ~ThreadPoolExecutor();

    void add(Func func) override;
    void addWithPriority(Func func, int8_t priority) override;

    /// Not sure how this is supposed to be used. This executor accepts any
    /// int8_t priority. This would make the total number of priorities 256
    /// which can't be represented as a uint8_t.
    uint8_t getNumPriorities() const override;

    static const std::shared_ptr<ThreadPoolExecutor> & instance();

    class D;
  private:
    std::unique_ptr<D> m_d;
  };
}

#endif // RADIANT_THREADPOOLEXECUTOR_HPP
