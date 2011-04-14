/* COPYRIGHT
 */

#include "Mutex.hpp"

#include <QMutex>
#include <cassert>

namespace Radiant {

  static bool mutexDebug = false;

  class Mutex::D : public QMutex
  {
  public:
    D(bool recursive)
      : QMutex(recursive ? QMutex::Recursive : QMutex::NonRecursive) {}
  };

  Mutex::Mutex(bool recursive)
    : m_initialized(false),
    m_d(new QMutex(recursive))
  {}

  Mutex::~Mutex()
  {
    delete m_d;
  }

  bool Mutex::lock(bool block)
  {
    if(block) {
      m_d->lock();
      return true;
    } else
      return m_d->tryLock();
  }

  bool Mutex::unlock()
  {
    m_d->unlock();

    return true;
  }

}
