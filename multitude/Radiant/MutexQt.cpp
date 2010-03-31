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

namespace Radiant {

  static bool mutexDebug = false;

  class Mutex::D : public QMutex 
  {
  public:
	  D(RecursionMode mode) : QMutex(mode) {}
  };

  Mutex::Mutex()
    : m_d(0),
	m_active(false)
  {}

  Mutex::~Mutex()
  {
    if(!m_active) return;
	delete m_d;
  }

  bool Mutex::init(bool /*shared*/, bool /*prio_inherit*/, bool recursive)
  {
	  m_d = new D(recursive ? QMutex::Recursive : QMutex::NonRecursive);

	  m_active = true;

	  return true;
  }

  bool Mutex::close()
  {
    if(m_active) {
      m_active = false;
      delete m_d;
	  m_d = 0;

	  return true;
    }

    return false;
  }

  bool Mutex::lock()
  {
	m_d->lock();

	return true;
  }

  bool Mutex::lock(bool block)
  {
    if(!block) return tryLock();
    else return lock();
  }

  bool Mutex::tryLock() 
  { 
	  return m_d->tryLock();
  }

  bool Mutex::unlock()
  {
	  m_d->unlock();

	  return true;
	}

}
