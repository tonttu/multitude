/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_SINGLETON_HPP
#define RADIANT_SINGLETON_HPP

#include "Export.hpp"
#include "Mutex.hpp"
#include <memory>


/// @file Singleton.hpp

/** @ingroup macros
    @def DEFINE_SINGLETON(T)
    Implements singleton of object type T.

    Singleton is used when there is only one object of type T. The one object
    can be accessed with the function instance(). The object is created the
    first time you access it. The lazy creation is done because it might
    not be possible to create objects during application startup statically.

    The singleton instance is destroyed when its reference count reaches zero.
    So if you want to guarantee the life-time of the singleton instance, you
    should store the returned shared_ptr.

    To declare a class as singleton, you use the DECLARE_SINGLETON macro in your
    class declaration:

    @code
    // Declare a MyClass and make it a singleton
    class MyClass {
      DECLARE_SINGLETON(MyClass);
      ...
    };
    @endcode

    You must also define the singleton using DEFINE_SINGLETON macro. Usually
    you put this in the .cpp file with the rest of the class definition:

    @code
    #include "MyClass.hpp"

    // Define the MyClass singleton
    DEFINE_SINGLETON(MyClass);
    @endcode

    The DEFINE_SINGLETON macro implements "double-checked locking pattern" to
    minimize the mutex usage. In almost all cases the mutex doesn't need to be
    locked, so using only one shared static mutex for all singletons won't slow
    things down.

    Then to actually use the singleton, you just call the instance() method
    which returns a shared pointer.

    @code
    // Do something with the singleton
    std::shared_ptr<MyClass> ptr = MyClass::instance();

    ptr->doSomething();
    @endcode

    This class is thread-safe. */
#define DEFINE_SINGLETON(T)                                        \
  DEFINE_SINGLETON2(T,,,)

#define DEFINE_SINGLETON2(T, INIT, INIT2, ARGS)                    \
  std::shared_ptr<T> T :: instance() {                             \
    std::shared_ptr<T> p = s_multiSingletonInstance.lock();        \
    if(!p) {                                                       \
      Radiant::Guard g(Radiant::s_singletonMutex);                 \
      p = s_multiSingletonInstance.lock();                         \
      if(p) return p;                                              \
      p.reset(new T(ARGS));                                        \
      INIT                                                         \
      s_multiSingletonInstance = p;                                \
      INIT2                                                        \
    }                                                              \
    return p;                                                      \
  }                                                                \
  std::weak_ptr<T> T::s_multiSingletonInstance;

/// @todo this can not be used to properly export the instance() function from
/// libraries. The macro needs one more parameter.
#define DECLARE_SINGLETON(T)                                       \
  public: static std::shared_ptr<T> instance();                    \
  private: static std::weak_ptr<T> s_multiSingletonInstance

namespace Radiant {

  /// Shared mutex for all the singleton macros
  extern RADIANT_API Radiant::Mutex s_singletonMutex;
}

#endif
