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

#include <sys/time.h>
#include <pthread.h>

#ifndef WIN32

#define timeeval timeval

#endif

namespace Radiant
{
  class Condition::D
  {
    public:
    pthread_cond_t m_ptcond;
  };

  Condition::Condition() 
  : m_d(new D())
  {
    pthread_cond_init(&m_d->m_ptcond, 0);
  }
    
  Condition::~Condition()
  {
    pthread_cond_destroy(&m_d->m_ptcond);
    delete m_d;
  }
    
  int Condition::wait(Mutex &mutex)
  {
    pthread_mutex_t * ptmutex = (pthread_mutex_t *)mutex.m_d;

    return pthread_cond_wait(& m_d->m_ptcond, ptmutex);
  }
  
  int Condition::wait(Mutex &mutex, int millsecs)
  { 
    struct timespec abstime;
    struct timeval tv;

    gettimeofday(&tv, 0);
    tv.tv_sec += millsecs / 1000;
    tv.tv_usec += 1000 * (millsecs % 1000);
    if(tv.tv_usec >= 1000000)
    {
	    tv.tv_sec++;
	    tv.tv_usec -= 1000000;
    }
    abstime.tv_sec = tv.tv_sec;
    abstime.tv_nsec = 1000 * tv.tv_usec;

    pthread_mutex_t * ptmutex = (pthread_mutex_t *)mutex.m_d;

    return pthread_cond_timedwait(& m_d->m_ptcond, ptmutex, & abstime);
  }
    
  int Condition::wakeAll() 
  {
    return pthread_cond_broadcast(& m_d->m_ptcond);
  }
    
  int Condition::wakeAll(Mutex &mutex) 
  { 
    mutex.lock();
    int r = pthread_cond_broadcast(& m_d->m_ptcond);
    mutex.unlock();
    return r;
  }
    
  int Condition::wakeOne() 
  {
    return pthread_cond_signal(& m_d->m_ptcond);
  }
    
  int Condition::wakeOne(Mutex &mutex) 
  {
    mutex.lock();
    int r = wakeOne();
    mutex.unlock();
    return r;
  }

} // namespace
