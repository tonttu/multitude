/* COPYRIGHT
 */

#include "Condition.hpp"
#include "Mutex.hpp"

#include <iostream>

#include <cassert>
#include <errno.h>
#include <string.h>

#include <sys/time.h>


// Trick for Linux
#ifndef WIN32
#ifndef PTHREAD_MUTEX_RECURSIVE
extern "C" {
  extern int pthread_mutexattr_setkind_np __P ((pthread_mutexattr_t *__attr,
                        int __kind));
}
#define PTHREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE_NP
#define pthread_mutexattr_settype(x,y) pthread_mutexattr_setkind_np(x,y)
#endif
#endif

namespace Radiant {

  static bool mutexDebug = false;

  class Mutex::D {
  public:
    D(bool recursive);
    ~D()
    {
      if(m_ptmutex) {
        pthread_mutex_destroy(m_ptmutex);
        delete m_ptmutex;
      }
    }

    bool m_recursive;
    pthread_mutex_t* m_ptmutex;
  };


  Mutex::D::D(bool recursive)
    : m_recursive(recursive), m_ptmutex(new pthread_mutex_t)
  {
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);

    int err;

    if(m_recursive) {
      err = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);

      if(err) {
        if(mutexDebug) std::cerr << "Could not create recursive mutex "
            << strerror(err) << std::endl;

        return;
      }
    }

    err = pthread_mutex_init( m_ptmutex, &mutex_attr);

    if(err && mutexDebug) {
      std::cerr << "Could not create mutex " << strerror(err) << std::endl;
    }
  }

  Mutex::Mutex(bool recursive)
    : m_d(new D(recursive))
  {}

  Mutex::~Mutex()
  {
    delete m_d;
  }

  bool Mutex::lock(bool block)
  {
    if(block) {
      int e = pthread_mutex_lock(m_d->m_ptmutex);
      if(e)
        std::cerr << "Mutex::lock # FAILED " << strerror(e) << std::endl;
      return e == 0;
    }

    int e = pthread_mutex_trylock(m_d->m_ptmutex);
    return e == 0;
  }

  bool Mutex::unlock()
  {
    const char *fname = "Mutex::unlock ";

    int e = pthread_mutex_unlock(m_d->m_ptmutex);

    if(e)
      std::cerr << fname << strerror(e) << " " << this << std::endl;

    return !e;
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

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
    pthread_mutex_t * ptmutex = mutex.m_d->m_ptmutex;

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

    pthread_mutex_t * ptmutex = mutex.m_d->m_ptmutex;

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

}
