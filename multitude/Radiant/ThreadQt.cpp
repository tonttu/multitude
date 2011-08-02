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

#include <QThread>

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

  enum {
    STOPPED,
    STOPPING,
    RUNNING
  };

  bool Thread::m_threadDebug = false;
  bool Thread::m_threadWarnings = false;

  class Thread::D : public QThread 
  {
  public:
	  D(Thread * t) : m_thread(t) {}

	  virtual void run() {
		m_thread->childLoop();
	  }

	  Thread * m_thread;
  };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

  Thread::Thread()
    : m_d(new D(this)),
    m_state(STOPPED)
  {}

  Thread::~Thread()
  {
    assert(isRunning() == false);

	  delete m_d;
  }

  Thread::id_t Thread::myThreadId()
  {
    return reinterpret_cast<void*> (QThread::currentThread());
  }

  void Thread::run()
  {
	  m_d->start();
  }

  bool Thread::waitEnd(int timeoutms)
  {
    if(timeoutms)
      return m_d->wait(timeoutms * 1000); // Guess that it is microseconds...
    else
      return m_d->wait();
  }

  void Thread::kill()
  {
    // Does nothing
  }

  bool Thread::isRunning() const
  {
    return m_d->isRunning();
  }
}
