/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Thread.hpp"
#include "Mutex.hpp"
#include "Sleep.hpp"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <cassert>

#include <QThread>

namespace Radiant {

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

  Thread::Thread(const QString &name)
    : m_d(new D(this))
  {
    setName(name);
  }

  Thread::~Thread()
  {
    assert(isRunning() == false);

	  delete m_d;
  }

  void Thread::setName(const QString & name)
  {
    m_d->setObjectName(name);
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
      return m_d->wait(timeoutms);
    else
      return m_d->wait();
  }

  bool Thread::isRunning() const
  {
    return m_d->isRunning();
  }
}
