/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#include "Thread.hpp"
#include "Mutex.hpp"
#include "Sleep.hpp"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <cassert>

namespace Radiant {

  class Thread::D {
    public:
    pthread_t m_pthread;
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
    return size_t(pthread_self());
  }

  bool Thread::run(bool prefer_system)
  {
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

    if(e && (m_threadDebug || m_threadWarnings))
      std::cout << "failed - " << strerror(e) << std::endl;
    else
      m_state = RUNNING;

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

    if(e && (m_threadDebug || m_threadWarnings))
      std::cout << "failed - " << strerror(e) << std::endl;
    else
      m_state = RUNNING;

    return !e;
  }

  bool Thread::waitEnd()
  {
    if(m_threadDebug) {
      std::cout << "Thread::waitEnd " << this << std::endl;
    }

    int e = pthread_join(m_d->m_pthread, 0);

    if(e) {
      if(m_threadDebug || m_threadWarnings)
    std::cerr << "Thread::waitEnd failed - " << strerror(e) << std::endl;
      return false;
    }
    return true;
  }

  void Thread::kill()
  {
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

    childLoop();
  }

  void *Thread::entry(void *t)
  {
    if(m_threadDebug)
      std::cout << "Thread::entry " << t << std::endl;

    ((Thread *) t)->mainLoop();
    ((Thread *) t)->m_state = STOPPING;

    return 0;
  }

  QThread * Thread::qtThread()
  {
    return 0;
  }

}
