/* COPYRIGHT
 */

#include "Thread.hpp"
#include "Mutex.hpp"
#include "Platform.hpp"
#include "Sleep.hpp"
#include "Trace.hpp"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <cassert>

#ifdef RADIANT_LINUX
#include <sys/syscall.h>
namespace Radiant {
  int gettid() { return syscall(SYS_gettid); }
}
#elif defined(WIN32)
#include <Windows.h>
namespace Radiant {
  int gettid() { return GetCurrentThreadId(); }
}
#else
namespace Radiant {
  int gettid() { return getpid(); }
}
#endif

namespace Radiant {

  class Thread::D {
  public:
    D() : m_valid(false) {}
    pthread_t m_pthread;
    bool m_valid;
  };

  enum {
    STOPPED,
    STOPPING,
    RUNNING
  };

  bool Thread::m_threadDebug = false;
  bool Thread::m_threadWarnings = false;

  Thread::Thread()
    : m_d(new D()),
    m_state(STOPPED)
  {}

  Thread::~Thread()
  {
    assert(isRunning() == false);
    delete m_d;
  }

  Thread::id_t Thread::myThreadId()
  {
    // Not sure how safe this is, but we just cast the pointer to size_t
    return reinterpret_cast<void*> (pthread_self());
  }

  bool Thread::run(bool prefer_system)
  {
    if(m_state == RUNNING)
      return true;

    if(m_threadDebug)
      std::cout << "Thread::run " << prefer_system << " " << this << std::endl;

    if(prefer_system) {
      if(!runSystem()) {
    return runProcess();
      }
    }
    else if(!runProcess())
      return runSystem();

    return true;
  }

  bool Thread::runSystem()
  {
    if(m_threadDebug)
      std::cout << "Thread::runSystem " << this << std::endl;

    pthread_attr_t thread_attr;

    pthread_attr_init(&thread_attr);
    int e = pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);

    if(e) {
      if(m_threadDebug || m_threadWarnings)
    std::cout << "failed - " << strerror(e) << std::endl;
      return false;
    }

    e = pthread_create(&m_d->m_pthread, &thread_attr, entry, this);
    m_d->m_valid = true;

    if(e && (m_threadDebug || m_threadWarnings))
      std::cout << "failed - " << strerror(e) << std::endl;

    return !e;
  }

  bool Thread::runProcess()
  {
    if(m_threadDebug)
      std::cout << "Thread::runProcess " << this << std::endl;

    pthread_attr_t thread_attr;

    pthread_attr_init(&thread_attr);

    int e = pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_PROCESS);

    if(e) {
      if(m_threadDebug || m_threadWarnings)
    std::cout << "failed - " << strerror(e) << std::endl;
      return false;
    }

    e = pthread_create(&m_d->m_pthread, &thread_attr, entry, this);
    m_d->m_valid = true;

    if(e && (m_threadDebug || m_threadWarnings))
      std::cout << "failed - " << strerror(e) << std::endl;

    return !e;
  }

  bool Thread::waitEnd(int timeoutms)
  {
    if(m_threadDebug) {
      std::cout << "Thread::waitEnd " << this << std::endl;
    }

    if(!m_d->m_valid)
      return true;

    int e;

#ifdef RADIANT_OSX
    if(timeoutms) {
      error("Thread::waitEnd # Timeout unimplemented on OS X");
    }
    e = pthread_join(m_d->m_pthread, 0);
#else

    if(timeoutms) {
      Radiant::TimeStamp now(Radiant::TimeStamp::getTime());
      now += Radiant::TimeStamp::createSecondsD(timeoutms * 0.001);
      timespec ts;
      ts.tv_sec = now.seconds();
      ts.tv_nsec = now.subSecondsD() * 1000.0f;
      e = pthread_timedjoin_np(m_d->m_pthread, 0, &ts);
    }
    else {
      e = pthread_join(m_d->m_pthread, 0);
    }

#endif

    if(e) {
      if(m_threadDebug || m_threadWarnings)
        std::cerr << "Thread::waitEnd failed - " << strerror(e) << std::endl;
      return false;
    }
    return true;
  }

  void Thread::kill()
  {
    if(m_d->m_valid)
      pthread_kill(m_d->m_pthread, SIGKILL);
  }

  bool Thread::isRunning()
  {
    return m_state == RUNNING;
  }

  bool Thread::setThreadRealTimePriority(int priority)
  {
    sched_param sp;
    bzero(&sp, sizeof(sp));

    sp.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), SCHED_RR, &sp)  == -1)
      return false;

    return true;
  }

  void Thread::threadExit()
  {
    if(m_threadDebug) {
      std::cout << "Thread::threadExit " << this << std::endl;
    }

    pthread_exit(0);
  }

  void Thread::mainLoop()
  {
    if(m_threadDebug) {
      std::cout << "Thread::mainLoop " << this <<  " pid: " << getpid() << std::endl;
    }

    m_state = RUNNING;
    childLoop();
    m_state = STOPPING;
  }

  void *Thread::entry(void *t)
  {
    if(m_threadDebug)
      std::cout << "Thread::entry " << t << std::endl;

    ((Thread *) t)->mainLoop();

    return 0;
  }

  QThread * Thread::qtThread()
  {
    return 0;
  }

}
