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
#include "RefPtr.hpp"

namespace Radiant {
  extern RADIANT_API Radiant::Mutex s_singletonMutex;
}

/** Implements singleton of object type T

    Singleton is used when there is only one object of type T. The one object
    can be accessed with the function instance(). The object is created as you
    for the first time access it. The lazy creation is done because it might
    not be possible to create objects during application startup.

    The singleton instance is destroyed when its reference count reaches zero.
    So if you want to guarantee the life-time of the singleton instance, you
    should store the returned shared_ptr.

    @todo document the usage of these macros
*/

/// This macro implements "double-checked locking pattern" to minimize the
/// mutex usage. In almost all cases the mutex doesn't need to be locked, so
/// using only one static mutex won't slow things down.
#define DEFINE_SINGLETON(T)                                        \
  std::shared_ptr<T> T :: instance() {                             \
    std::shared_ptr<T> p = s_multiSingletonInstance.lock();        \
    if(!p) {                                                       \
      Radiant::Guard g(Radiant::s_singletonMutex);                 \
      p = s_multiSingletonInstance.lock();                         \
      if(p) return p;                                              \
      p.reset(new T());                                            \
      s_multiSingletonInstance = p;                                \
    }                                                              \
    return p;                                                      \
  }                                                                \
  std::weak_ptr<T> T::s_multiSingletonInstance;

#define DECLARE_SINGLETON(T)                                       \
  public: static std::shared_ptr<T> instance();                    \
  private: static std::weak_ptr<T> s_multiSingletonInstance

/*
template<class T>
class Singleton2
{
public:
  static std::shared_ptr<T> instance()
  {
    std::shared_ptr<T> p = s_instance.lock();
    if(!p) {
      p.reset(new T());
      s_instance = p;
    }

    return p;
  }

private:
  static std::weak_ptr<T> s_instance;
};
*/
#endif

