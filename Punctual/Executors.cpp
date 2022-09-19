/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "Executors.hpp"

namespace Punctual
{
  namespace
  {
    folly::ManualExecutor s_beforeProcessInput;
    folly::ManualExecutor s_beforeInput;
    folly::ManualExecutor s_afterUpdate;
    folly::ManualExecutor s_beforeUpdate;
    folly::ManualExecutor s_beforeRender;
    folly::ManualExecutor s_afterRender;
    LimitedTimeExecutor s_mainThread;
  }

  folly::ManualExecutor * beforeProcessInput()
  {
    return &s_beforeProcessInput;
  }

  folly::ManualExecutor * beforeInput()
  {
    return &s_beforeInput;
  }

  folly::ManualExecutor * afterUpdate()
  {
    return &s_afterUpdate;
  }

  folly::ManualExecutor * beforeUpdate()
  {
    return &s_beforeUpdate;
  }

  folly::ManualExecutor * beforeRender()
  {
    return &s_beforeRender;
  }

  folly::ManualExecutor * afterRender()
  {
    return &s_afterRender;
  }

  LimitedTimeExecutor * mainThread()
  {
    return &s_mainThread;
  }
}
