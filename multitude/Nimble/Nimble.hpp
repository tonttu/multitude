/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_NIMBLE_HPP
#define NIMBLE_NIMBLE_HPP

#define debugNimble(...) (Radiant::trace("Nimble", Radiant::DEBUG, __VA_ARGS__))

/// Nimble library is a collection of C++ classes for 2D/3D graphics.
/** Nimble is used mostly for simple arithmetic/geometric
    calculations. The code is optimized for performance, thus there
    are a lot of inline functions. Also, the vector and matrix classes
    <b>do not</b> initialize any of their members.

    The matrix and vector classes use right-handed coordinates.
*/
namespace Nimble {
  template <typename T> class Vector2T;
  template <typename T> class Vector3T;
  template <typename T> class Vector4T;
  template <typename T> class Matrix2T;
  template <typename T> class Matrix3T;
  template <typename T> class Matrix4T;

  /// @cond

  // This is used as work-around for gcc/gdb bug 61321
  // Ideally we would like to write this:
  //
  // auto operator* (const Nimble::Vector2T<T> & v, S s) -> Nimble::Vector2T<decltype(T()*S())>;
  // or
  // auto foo() -> decltype(T() * 1.0f);
  //
  // but gcc demangler crashes when trying to parse the type. Work-around is
  // to use this instead:
  //
  // auto operator* (const Nimble::Vector2T<T> & v, S s) -> Nimble::Vector2T<typename Decltype<S, T>::mul>;
  // or
  // Decltype<T, float>::mul foo();
  //
  // see also https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61321
  template <typename A, typename B>
  struct Decltype
  {
    typedef decltype(A()*B()) mul;
    typedef decltype(A()/B()) div;
  };

  /// @endcond
}

#endif
