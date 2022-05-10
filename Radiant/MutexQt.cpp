/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Mutex.hpp"
#include "Timer.hpp"

#include <Radiant/Condition.hpp>

#include <QMutex>
#include <QWaitCondition>

#include <cassert>


namespace Radiant {

  Mutex s_onceMutex(true);

  // static bool mutexDebug = false;

  class Mutex::D : public QMutex
  {
  public:
    D(bool recursive)
      : QMutex(recursive ? QMutex::Recursive : QMutex::NonRecursive) {}
  };

  Mutex::Mutex(bool recursive)
    : m_d(new D(recursive))
  {}

  Mutex::~Mutex()
  {
    delete m_d;
  }

  void Mutex::lock()
  {
    m_d->lock();
  }

  bool Mutex::tryLock()
  {
    return m_d->tryLock();
  }

  void Mutex::unlock()
  {
    m_d->unlock();
  }

  class Condition::D : public QWaitCondition {};

  Condition::Condition()
    : m_d(new D())
  {
  }

  Condition::~Condition()
  {
    delete m_d;
  }

  bool Condition::wait(Mutex &mutex, unsigned long millsecs)
  {
    QMutex * qmutex = mutex.m_d;
    return m_d->wait(qmutex, millsecs);
  }

  bool Condition::wait2(Mutex & mutex, unsigned int & millsecs)
  {
    Timer timer;
    QMutex * qmutex = mutex.m_d;
    bool ret = m_d->wait(qmutex, millsecs);
    if(!ret) {
      millsecs = 0;
    } else {
      unsigned int diff = static_cast<unsigned int>(timer.time()*1000.0);
      if(diff > millsecs) millsecs = 0;
      else millsecs -= diff;
    }
    return ret;
  }

  void Condition::wakeAll()
  {
    m_d->wakeAll();
  }

  void Condition::wakeAll(Mutex & mutex)
  {
    Guard g(mutex);
    wakeAll();
  }

  void Condition::wakeOne()
  {
    m_d->wakeOne();
  }

  void Condition::wakeOne(Mutex & mutex)
  {
    Guard g(mutex);
    m_d->wakeOne();
  }
}
