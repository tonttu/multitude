/* COPYRIGHT
 */

#include "Mutex.hpp"

#include <Radiant/Condition.hpp>

#include <QMutex>
#include <QWaitCondition>

#include <cassert>


namespace Radiant {

  Mutex s_onceMutex;

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

  int Condition::wait(Mutex &mutex)
  {
    QMutex * qmutex = mutex.m_d;
    return m_d->wait(qmutex);
  }

  int Condition::wait(Mutex &mutex, int millsecs)
  {
    QMutex * qmutex = mutex.m_d;
    return m_d->wait(qmutex, millsecs);
  }

  int Condition::wakeAll()
  {
    m_d->wakeAll();
    return 0;
  }

  int Condition::wakeAll(Mutex &mutex)
  {
    mutex.lock();
    wakeAll();
    mutex.unlock();
    return 0;
  }

  int Condition::wakeOne()
  {
    m_d->wakeOne();
    return 0;
  }

  int Condition::wakeOne(Mutex &mutex)
  {
    mutex.lock();
    m_d->wakeOne();
    mutex.unlock();
    return 0;
  }
}
