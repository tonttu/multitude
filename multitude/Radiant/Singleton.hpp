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

#ifndef RADIANT_SINGLETON_HPP
#define RADIANT_SINGLETON_HPP

#include "Export.hpp"
#include "Mutex.hpp"

namespace Radiant {
  extern RADIANT_API Radiant::Mutex s_singletonMutex;
}

/// Implements singleton of object type T
/** Singleton is used when there is only one object of type T. The
    one object can be accessed with the function instance(). The
    object is created as you for the first time access it. The lazy
    creation is done because it might not be possible to create
    objects during application startup.

    Once created, there is no way to delete the single object.

    @todo document the usage of these macros
*/

/// This macro implements "double-checked locking" to minimize the
/// mutex usage. In almost all cases the mutex doesn't need to be locked,
/// so using only one static mutex won't slow things down.
#define DEFINE_SINGLETON(T)                                        \
  T & T :: instance() {                                            \
    if(s_multiSingletonInstance) return *s_multiSingletonInstance; \
    Radiant::Guard g(Radiant::s_singletonMutex);                  \
    if(s_multiSingletonInstance) return *s_multiSingletonInstance; \
    s_multiSingletonInstance = new T;                              \
    return *s_multiSingletonInstance;                              \
  }                                                                \
  T * volatile T::s_multiSingletonInstance = 0

#define DECLARE_SINGLETON(T)                                       \
  public: static T & instance();                                   \
  private: static T * volatile s_multiSingletonInstance


#endif

