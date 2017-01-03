#ifndef RADIANT_THREADCHECKS_HPP
#define RADIANT_THREADCHECKS_HPP

#include "Thread.hpp"

#include <QThread>
#include <cassert>

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
#define REQUIRE_THREAD(thread) REQUIRE_THREAD_IMPL2(__FILE__, __LINE__, thread)
#define REQUIRE_MAIN_THREAD REQUIRE_THREAD_IMPL(__FILE__, __LINE__, Radiant::ThreadInfo::mainThreadId, "main thread")
#else
#define REQUIRE_THREAD(thread) do {} while (false)
#define REQUIRE_MAIN_THREAD do {} while (false)
#endif

#endif // RADIANT_THREADCHECKS_HPP
