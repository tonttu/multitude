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

#include "Mutex.hpp"

#include <QMutex>
#include <cassert>

namespace Radiant {

  static bool mutexDebug = false;

  class Mutex::D : public QMutex 
  {
  public:
	  D(RecursionMode mode) : QMutex(mode) {}
  };

  Mutex::Mutex(bool recursive)
    : m_initialized(false),
      m_recursive(recursive),
      m_d(0)
  {}

  Mutex::~Mutex()
  {
    delete m_d;
  }

  bool Mutex::initialize(bool recursive)
  {
    assert(!m_d);
	  m_d = new D(recursive ? QMutex::Recursive : QMutex::NonRecursive);

    return true;
  }

  bool Mutex::internalLock(bool block)
  {
    assert(m_d);

    if(block) {
      m_d->lock();
      return true;
    } else
      return internalTryLock();
  }

  bool Mutex::internalTryLock()
  { 
	  return m_d->tryLock();
  }

  bool Mutex::unlock()
  {
	  m_d->unlock();

	  return true;
	}

}
