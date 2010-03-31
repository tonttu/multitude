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
    : m_state(STOPPED),
	m_d(new D(this))
  {}

  Thread::~Thread()
  {
    assert(isRunning() == false);

	delete m_d;
  }

  Thread::id_t Thread::myThreadId()
  {
	  return reinterpret_cast<size_t> (QThread::currentThreadId());
  }

  bool Thread::run(bool /*prefer_system*/)
  {
	  m_d->start();
	  return true;
  }

  bool Thread::waitEnd()
  {
	  return m_d->wait();
  }

  void Thread::kill()
  {
    // Does nothing
  }

  bool Thread::isRunning()
  {
    return m_d->isRunning();
  }

  bool Thread::setThreadRealTimePriority(int )
  {
	// Does nothing
	  return false;
  }

  QThread * Thread::qtThread()
  {
      return m_d;
  }

}
