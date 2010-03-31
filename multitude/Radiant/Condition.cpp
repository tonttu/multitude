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

#ifndef WIN32

#define timeeval timeval

#endif

namespace Radiant
{

  Condition::Condition() 
  {
    pthread_cond_init(&m_cond, 0);
  }
    
  Condition::~Condition()
  {
    pthread_cond_destroy(&m_cond);
  }
    
  int Condition::wait(Mutex &mutex)
  {
    return pthread_cond_wait(& m_cond, & mutex.pthreadMutex());
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
    return pthread_cond_timedwait(& m_cond, & mutex.pthreadMutex(), & abstime);
  }
    
  int Condition::wakeAll() 
  {
    return pthread_cond_broadcast(& m_cond);
  }
    
  int Condition::wakeAll(Mutex &mutex) 
  { 
    mutex.lock();
    int r = pthread_cond_broadcast(& m_cond); 
    mutex.unlock();
    return r;
  }
    
  int Condition::wakeOne() 
  {
    return pthread_cond_signal(& m_cond);
  }
    
  int Condition::wakeOne(Mutex &mutex) 
  {
    mutex.lock();
    int r = wakeOne();
    mutex.unlock();
    return r;
  }

} // namespace
