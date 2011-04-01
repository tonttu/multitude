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
#include <cassert>

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

  Mutex::Mutex(bool recursive)
    : m_initialized(false),
      m_recursive(recursive),
      m_d(new D)
  {}

  Mutex::~Mutex()
  {
    if(m_initialized) {
      int err = pthread_mutex_destroy(&m_d->m_ptmutex);
      if(err && mutexDebug)
        std::cerr << "Error:Mutex::~Mutex " << strerror(err) << std::endl;
    }

    delete m_d;
  }

  bool Mutex::initialize(bool recursive)
  {
    assert(!m_initialized);

    int err = 0;
    const char ename[] = "Error:Mutex::init"; // Error and name.

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);

    if(recursive) {
      err = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);

      if(err) {
        if(mutexDebug) std::cerr << ename << strerror(err) << std::endl;

        return false;
      }
    }

    err = pthread_mutex_init(&m_d->m_ptmutex, &mutex_attr);
    if(err && mutexDebug) {
      std::cerr << ename << strerror(err) << std::endl;
    }

    return ! err;
  }

  bool Mutex::internalLock(bool block)
  {
    const char *fname = "Mutex::lock ";

    if(!block)
      return internalTryLock();
    else {
      int e = pthread_mutex_lock(&m_d->m_ptmutex);

      if(e)
        std::cerr << fname << strerror(e) << " " << this << std::endl;

      return !e;
    }
  }

  bool Mutex::internalTryLock()
  {
    const char *fname = "Mutex::trylock ";

    int e = pthread_mutex_trylock(&m_d->m_ptmutex);
    if(e && e != EBUSY)
      std::cerr << fname << strerror(e) << " " << this << std::endl;

    return !e;
  }

  bool Mutex::unlock()
  {
    const char *fname = "Mutex::unlock ";

    int e = pthread_mutex_unlock(&m_d->m_ptmutex);

    if(e)
      std::cerr << fname << strerror(e) << " " << this << std::endl;

    return !e;
  }

}
