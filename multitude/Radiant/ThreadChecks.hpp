#ifndef RADIANT_THREADCHECKS_HPP
#define RADIANT_THREADCHECKS_HPP

#include "Thread.hpp"

#ifdef ENABLE_THREAD_CHECKS
#include "Trace.hpp"
#endif

#include <QThread>

namespace Radiant
{
  namespace ThreadInfo
  {
    RADIANT_API extern Thread::id_t mainThreadId;
  }
}

#ifdef ENABLE_THREAD_CHECKS
#define REQUIRE_THREAD_IMPL(file, line, thread, threadName) do { \
    if (Radiant::Thread::myThreadId() != (thread)) { \
      Radiant::fatal("%s:%d # Currently on thread '%s', expected %s", file, line, QThread::currentThread()->objectName().toUtf8().data(), threadName); \
    } \
  } while (false)
#define REQUIRE_THREAD_IMPL2(file, line, thread) do { \
    if (auto t = thread) { \
      if (Radiant::Thread::myThreadId() != t) { \
        if (t == Radiant::ThreadInfo::mainThreadId) { \
          Radiant::fatal("%s:%d # Currently on thread '%s', expected main thread", file, line, QThread::currentThread()->objectName().toUtf8().data()); \
        } else { \
          Radiant::fatal("%s:%d # Currently on thread '%s', expected thread id %p", file, line, QThread::currentThread()->objectName().toUtf8().data(), t); \
        } \
      } \
    } \
  } while (false)
#define REQUIRE_SAME_THREAD_IMPL(file, line, threadVar) do { \
  if (!threadVar) { \
    threadVar = Radiant::Thread::myThreadId(); \
  } else if (threadVar != Radiant::Thread::myThreadId()) { \
    Radiant::fatal("%s:%d # Currently on thread '%s', expected thread id %p", file, line, QThread::currentThread()->objectName().toUtf8().data(), threadVar); \
   } \
 } while (false)
#define REQUIRE_THREAD(thread) REQUIRE_THREAD_IMPL2(__FILE__, __LINE__, thread)
#define REQUIRE_MAIN_THREAD REQUIRE_THREAD_IMPL(__FILE__, __LINE__, Radiant::ThreadInfo::mainThreadId, "main thread")
#define THREAD_CHECK_ID(thread) mutable Radiant::Thread::id_t thread = nullptr;
#define THREAD_CHECK_ID_SELF(thread) mutable Radiant::Thread::id_t thread = Radiant::Thread::myThreadId();
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
