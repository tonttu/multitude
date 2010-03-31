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

#include <errno.h>
#include <string.h>

#include <iostream>

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
    pthread_mutex_t m_ptmutex;
  };

  Mutex::Mutex()
    : m_d(new D),
    m_active(false)
  {}

  Mutex::~Mutex()
  {
    if(!m_active) return;

    int err = pthread_mutex_destroy(&m_d->m_ptmutex);
    if(err && mutexDebug)
      std::cerr << "Error:Mutex::~Mutex " << strerror(err) << std::endl;

    delete m_d;
    /* A small service to the people who may need to debug deleted mutexes: */
    m_active = false;
  }

  bool Mutex::init(bool shared, bool prio_inherit, bool recursive)
  {
    int ishared, iprio_inherit, err;
    const char ename[] = "Error:Mutex::init"; // Error and name.

    if(shared && prio_inherit) {
      if(mutexDebug) std::cerr << ename << " - bad argument" << std::endl;
      return false;
    }

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);

    if(recursive) {
      err = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
      if(err) {
    if(mutexDebug) std::cerr << ename << strerror(err) << std::endl;
    return false;
      }
    }

#if defined(_SGI_MP_SOURCE) || _POSIX_THREAD_PROCESS_SHARED
    if(shared) ishared = PTHREAD_PROCESS_SHARED;
    else ishared = PTHREAD_PROCESS_PRIVATE;

    err = pthread_mutexattr_setpshared(&mutex_attr, ishared);
    if(err) {
      if(mutexDebug) std::cerr << ename << strerror(err) << std::endl;
      return false;
    }
#else
    // Not supported (Linux).
    (void) ishared;
#endif

#if defined(PTHREAD_PRIO_INHERIT) && defined(PTHREAD_PRIO_NONE)
    if(prio_inherit) iprio_inherit = PTHREAD_PRIO_INHERIT;
    else iprio_inherit = PTHREAD_PRIO_NONE;
    err = pthread_mutexattr_setprotocol(&mutex_attr, iprio_inherit);
    if(err) {
      std::cerr << ename << strerror(err) << std::endl;
      return false;
    }
#else
    (void) iprio_inherit;
#endif

    err = pthread_mutex_init(&m_d->m_ptmutex, &mutex_attr);
    if(err && mutexDebug) {
      std::cerr << ename << strerror(err) << std::endl;
    }
    else
      m_active = true;

    return ! err;
  }

  bool Mutex::close()
  {
    if(m_active) {
      m_active = false;
      return ! pthread_mutex_destroy(&m_d->m_ptmutex);
    }
    return false; // Nothing to close
  }

  bool Mutex::lock()
  {
    const char *fname = "Mutex::lock ";

    int e = pthread_mutex_lock(&m_d->m_ptmutex);
    if(e) std::cerr << fname << strerror(e) << " " << this << std::endl;
    return !e;
  }

  bool Mutex::lock(bool block)
  {
    if(!block) return tryLock();
    else return lock();
  }

  bool Mutex::tryLock()
  {
    const char *fname = "Mutex::trylock ";

    int e = pthread_mutex_trylock(&m_d->m_ptmutex);
    if(e) std::cerr << fname << strerror(e) << " " << this << std::endl;
    return !e;
  }

  bool Mutex::unlock()
  {
    const char *fname = "Mutex::unlock ";

    int e = pthread_mutex_unlock(&m_d->m_ptmutex);
    if(e) std::cerr << fname << strerror(e) << " " << this << std::endl;
    return !e;
  }

}
