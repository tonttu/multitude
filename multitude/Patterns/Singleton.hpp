/* COPYRIGHT
 *
 * This file is part of Patterns.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Patterns.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef PATTERNS_SINGLETON_HPP
#define PATTERNS_SINGLETON_HPP

namespace {
  static Radiant::Mutex s_singletonMutex;
}

namespace Patterns {

  /// Implements singleton of object type T
  /** Singleton is used when there is only one object of type T. The
      one object can be accessed with the function instance(). The
      object is created as you for the first time access it. The lazy
      creation is done because it might not be possible to create
      objects during application startup. 

      Once created, there is no way to delete the single object.
  */
  template<class T>
  class Singleton
  {
 public:
    static T & instance() {
      /// This function implements "double-checked locking" to minimize the
      /// mutex usage. In almost all cases the mutex doesn't need to be locked,
      /// so using only one static mutex won't slow things down.
      static volatile T * obj = 0;
      if(obj) return *obj;

      Radiant::Guard g(s_singletonMutex);

      if(obj) return *obj;
      obj = new T();

      return *obj;
    }

  protected:
    Singleton() {}
    virtual ~Singleton() {}
  };

}

#endif

