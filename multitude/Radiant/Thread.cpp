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

#ifdef WIN32
#include <process.h>
#include <WinPort.h>
#pragma warning(disable : 4996)
#endif

namespace Radiant {

  enum {
    STOPPED,
    STOPPING,
    RUNNING
  };

  bool Thread::m_threadDebug = false;
  bool Thread::m_threadWarnings = false;

  Thread::Thread()
    : m_state(STOPPED)
  {}

  Thread::~Thread()
  {
    assert(isRunning() == false);
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

    e = pthread_create(&m_thread, &thread_attr, entry, this);

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
  
    e = pthread_create(&m_thread, &thread_attr, entry, this);

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

    int e = pthread_join(m_thread, 0);
  
    if(e) {
      if(m_threadDebug || m_threadWarnings)
	std::cerr << "Thread::waitEnd failed - " << strerror(e) << std::endl;
      return false;
    }
    return true;
  }

  void Thread::kill()
  {
    pthread_kill(m_thread, SIGKILL);
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

  void Thread::test()
  {
    bool debug = m_threadDebug;
    m_threadDebug = true;  // Debugging texts on.

    const char name[] = "Thread::test ";

    Sleep sleep;

    std::cout << name << " pid: " << getpid() << " test 1" << std::endl;
    {
      std::cout << name << "creating thread that runs for 4 seconds" << std::endl;
      ThreadTest t;
      std::cout << name << "starting thread..." << std::endl;
      t.run();
      sleep.sleepUs(100000);
      std::cout << name << "...thread running, sleeping 4 seconds" << std::endl;
      Sleep::sleepS(4);
      std::cout << name << "ending thread" << std::endl;
      t.requestEnd();
      bool ok = t.waitEnd();
      std::cout << name << "success: " << ok << std::endl << std::endl;
    }

    std::cout << name << "test 2" << std::endl;
    {
      std::cout << name << "creating thread that runs for 2 seconds" << std::endl;
      ThreadTest t;
      std::cout << name << "starting thread..." << std::endl;
      t.run();
      std::cout << name << "...thread running, sleeping 2 seconds" << std::endl;
      Sleep::sleepS(2);
      std::cout << name << "deliberate deadlock now, USE CTRL+C to exit " << std::endl;
      /* "requestEnd" is not called. Therefore we deadlock. */ 
      bool ok = t.waitEnd();
      std::cout << name << "Failed to deadlock " << ok << std::endl;
    }
    m_threadDebug = debug; // Return the original value
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

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  bool ThreadAvailable::m_systemLevel = false;
  bool ThreadAvailable::m_processLevel = false;
  bool ThreadAvailable::m_defined = false;

  bool ThreadAvailable::systemLevelWorks()
  {
    if(!m_defined) check();
    return m_systemLevel;
  }

  bool ThreadAvailable::processLevelWorks()
  {
    if(!m_defined) check();
    return m_processLevel;
  }

  void ThreadAvailable::check()
  {
    {
      ThreadTest t;
      t.requestEnd();
      m_systemLevel = t.runSystem();
      if(m_systemLevel)
	t.waitEnd();
    }
    {
      ThreadTest t;
      t.requestEnd();
      m_processLevel = t.runProcess();
      if(m_processLevel)
	t.waitEnd();
    }
    m_defined = true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ThreadTest::ThreadTest()
    : m_mutex(new MutexAuto),
      m_continue(true),
      m_counter(0)
  {}

  int ThreadTest::count()
  {
    m_mutex->lock();
    int i = m_counter;
    m_mutex->unlock();
    return i;
  }

  void ThreadTest::requestEnd()
  {
    m_mutex->lock();
    m_continue = false;
    m_mutex->unlock();
  }

  void ThreadTest::childLoop()
  {
    m_mutex->lock();
    Sleep s;

    while(m_continue) {

      m_mutex->unlock();
      s.sleepUs(500000); // Half second sleep.
      m_mutex->lock();

      std::cout << "ThreadTest::childLoop " << this 
		<< "  " << m_counter++ << std::endl;
    }
    m_mutex->unlock();
  }

}
