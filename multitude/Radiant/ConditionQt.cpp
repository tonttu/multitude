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

#include <Radiant/Condition.hpp>

#include <QWaitCondition>
#include <QMutex>

namespace Radiant
{
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
	  QMutex * qmutex = (QMutex *)(mutex.m_d);
	  return m_d->wait(qmutex);
  }
  
  int Condition::wait(Mutex &mutex, int millsecs)
  { 
	  QMutex * qmutex = (QMutex *)(mutex.m_d);
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

} // namespace
