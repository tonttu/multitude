/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_BGTHREADEXECUTOR_HPP
#define RADIANT_BGTHREADEXECUTOR_HPP

#include <Radiant/Export.hpp>
#include <Radiant/Singleton.hpp>
#include <Radiant/BGThread.hpp>

#include <folly/Executor.h>

#include <memory>

namespace Radiant
{
  /// BGThread adapter that conforms to the folly::Executor interface.
  class RADIANT_API BGThreadExecutor : public folly::Executor
  {
  public:
    typedef folly::Func Func;

    /// Will use the given BGThread instance or the global singleton one if nullptr
    /// BGThreadExecutor won't start BGThread automatically
    explicit BGThreadExecutor(const std::shared_ptr<BGThread> & bgThread = nullptr);
    ~BGThreadExecutor();

    void add(Func) override;
    void addWithPriority(Func func, int8_t priority) override;
    uint8_t getNumPriorities() const override;

    static BGThreadExecutor * instance();

    /// BGThreadExecutor instance that is driven by BGThread::ioThreadPool().
    static BGThreadExecutor * instanceIo();

    class D;
  private:
    std::unique_ptr<D> m_d;
  };
}

#endif // RADIANT_BGTHREADEXECUTOR_HPP
