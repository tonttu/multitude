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

#ifndef RADIANT_REF_PTR_HPP
#define RADIANT_REF_PTR_HPP

/// @cond

#include "Radiant/Platform.hpp"

#include <cstddef>
#include <cassert>

// try to detect C++11
#if defined(RADIANT_CXX11)
  #include <memory>
#elif defined(__GCCXML__)
  #include <generator/gccxml_tr1.hpp>
#else
  #if defined(__GNUC__) || defined(RADIANT_LINUX) || defined(RADIANT_OSX)
    #include <tr1/memory>
    #define NEED_MAKE_SHARED
  #elif defined(RADIANT_WINDOWS) && defined(_HAS_TR1)
    #include <memory>
  #else
    #include <boost/tr1/memory.hpp>
  #endif
  namespace std
  {
    using tr1::shared_ptr;
    using tr1::weak_ptr;
    using tr1::swap;
    using tr1::get_deleter;
    using tr1::static_pointer_cast;
    using tr1::dynamic_pointer_cast;
    using tr1::const_pointer_cast;
    using tr1::enable_shared_from_this;
  }
#endif

#ifdef NEED_MAKE_SHARED
namespace std
{
  // std::make_shared
  template <typename T> std::shared_ptr<T> make_shared() { return std::shared_ptr<T>(new T()); }
  template <typename T, typename A1> std::shared_ptr<T> make_shared(const A1 & a1) { return std::shared_ptr<T>(new T(a1)); }
  template <typename T, typename A1, typename A2> std::shared_ptr<T> make_shared(const A1 & a1, const A2 & a2) { return std::shared_ptr<T>(new T(a1,a2)); }
  template <typename T, typename A1, typename A2, typename A3> std::shared_ptr<T> make_shared(const A1 & a1, const A2 & a2, const A3 & a3) { return std::shared_ptr<T>(new T(a1,a2,a3)); }
  template <typename T, typename A1, typename A2, typename A3, typename A4> std::shared_ptr<T> make_shared(const A1 & a1, const A2 & a2, const A3 & a3, const A4 & a4) { return std::shared_ptr<T>(new T(a1,a2,a3,a4)); }
  template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5> std::shared_ptr<T> make_shared(const A1 & a1, const A2 & a2, const A3 & a3, const A4 & a4, const A5 & a5) { return std::shared_ptr<T>(new T(a1,a2,a3,a4,a5)); }
  template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6> std::shared_ptr<T> make_shared(const A1 & a1, const A2 & a2, const A3 & a3, const A4 & a4, const A5 & a5, const A6 & a6) { return std::shared_ptr<T>(new T(a1,a2,a3,a4,a5,a6)); }
  template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7> std::shared_ptr<T> make_shared(const A1 & a1, const A2 & a2, const A3 & a3, const A4 & a4, const A5 & a5, const A6 & a6, const A7 & a7) { return std::shared_ptr<T>(new T(a1,a2,a3,a4,a5,a6,a7)); }
}
#undef NEED_MAKE_SHARED
#endif

#ifdef NEED_MAKE_UNIQUE
namespace std
{
  // std::make_unique
  template <typename T> std::unique_ptr<T> make_unique() { return std::unique_ptr<T>(new T()); }
  template <typename T, typename A1> std::unique_ptr<T> make_unique(const A1 & a1) { return std::unique_ptr<T>(new T(a1)); }
  template <typename T, typename A1, typename A2> std::unique_ptr<T> make_unique(const A1 & a1, const A2 & a2) { return std::unique_ptr<T>(new T(a1,a2)); }
  template <typename T, typename A1, typename A2, typename A3> std::unique_ptr<T> make_unique(const A1 & a1, const A2 & a2, const A3 & a3) { return std::unique_ptr<T>(new T(a1,a2,a3)); }
  template <typename T, typename A1, typename A2, typename A3, typename A4> std::unique_ptr<T> make_unique(const A1 & a1, const A2 & a2, const A3 & a3, const A4 & a4) { return std::unique_ptr<T>(new T(a1,a2,a3,a4)); }
  template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5> std::unique_ptr<T> make_unique(const A1 & a1, const A2 & a2, const A3 & a3, const A4 & a4, const A5 & a5) { return std::unique_ptr<T>(new T(a1,a2,a3,a4,a5)); }
  template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6> std::unique_ptr<T> make_unique(const A1 & a1, const A2 & a2, const A3 & a3, const A4 & a4, const A5 & a5, const A6 & a6) { return std::unique_ptr<T>(new T(a1,a2,a3,a4,a5,a6)); }
  template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7> std::unique_ptr<T> make_unique(const A1 & a1, const A2 & a2, const A3 & a3, const A4 & a4, const A5 & a5, const A6 & a6, const A7 & a7) { return std::unique_ptr<T>(new T(a1,a2,a3,a4,a5,a6,a7)); }
}
#undef NEED_MAKE_UNIQUE
#endif

/// @endcond

#endif
