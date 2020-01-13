#ifndef RADIANT_THREADCHECKS_HPP
#define RADIANT_THREADCHECKS_HPP

#include "Thread.hpp"
#include "Trace.hpp"

#include <QThread>

namespace Radiant
{
  namespace ThreadChecks
  {
    RADIANT_API extern Thread::Id mainThreadId;

    RADIANT_API void handleThreadError(const char * file, int line, Thread::Id expectedThread);

    RADIANT_API Trace::Severity logLevel();
    RADIANT_API void setLogLevel(Trace::Severity severity);
  }
}

#ifdef ENABLE_THREAD_CHECKS
#define REQUIRE_THREAD_IMPL(file, line, thread) do {                       \
    if (auto t = (thread); t && Radiant::Thread::currentThreadId() != t)   \
      Radiant::ThreadChecks::handleThreadError(file, line, t);             \
  } while (false)
#define REQUIRE_SAME_THREAD_IMPL(file, line, threadVar) do {               \
  if (!threadVar)                                                          \
    threadVar = Radiant::Thread::currentThreadId();                        \
  else if (threadVar != Radiant::Thread::currentThreadId())                \
    Radiant::ThreadChecks::handleThreadError(file, line, threadVar);       \
 } while (false)
#define REQUIRE_THREAD(thread) REQUIRE_THREAD_IMPL(__FILE__, __LINE__, thread)
#define REQUIRE_MAIN_THREAD REQUIRE_THREAD_IMPL(__FILE__, __LINE__, Radiant::ThreadChecks::mainThreadId)
#define THREAD_CHECK_ID(thread) mutable Radiant::Thread::Id thread = nullptr;
#define THREAD_CHECK_ID_SELF(thread) mutable Radiant::Thread::Id thread = Radiant::Thread::currentThreadId();
#define REQUIRE_SAME_THREAD(threadVar) REQUIRE_SAME_THREAD_IMPL(__FILE__, __LINE__, threadVar)
#define THREAD_CHECK_CLEAR(threadVar) threadVar = nullptr;
#else
#define REQUIRE_THREAD(thread) do {} while (false)
#define REQUIRE_MAIN_THREAD do {} while (false)
#define REQUIRE_SAME_THREAD(thread) do {} while (false)
#define THREAD_CHECK_ID(thread)
#define THREAD_CHECK_ID_SELF(thread)
#define THREAD_CHECK_CLEAR(threadVar)
#endif

#endif // RADIANT_THREADCHECKS_HPP
