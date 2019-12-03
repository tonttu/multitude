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
