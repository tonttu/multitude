/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "ThreadChecks.hpp"

namespace
{
  Radiant::Trace::Severity s_logLevel = Radiant::Trace::FATAL;
}

namespace Radiant::ThreadChecks
{
  Thread::Id mainThreadId = Thread::currentThreadId();

  void handleThreadError(const char * file, int line, Thread::Id expectedThread)
  {
    Radiant::Trace::trace(s_logLevel, "%s:%d # Currently on thread '%s', expected thread '%s'",
                          file, line, Thread::currentThreadName().data(),
                          Thread::threadName(expectedThread).data());
  }

  void setLogLevel(Trace::Severity severity)
  {
    s_logLevel = severity;
  }

  Trace::Severity logLevel()
  {
    return s_logLevel;
  }
}
