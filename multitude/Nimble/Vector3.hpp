/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef NIMBLE_VECTOR3T_HPP
#define NIMBLE_VECTOR3T_HPP

#include <Nimble/Export.hpp>
#include <Nimble/Vector2.hpp>

#include <iostream>

namespace Nimble {

  /** Three-dimensional vector class for 3D mathematics. */

  template <class T>
  class NIMBLE_API Vector3T
  {
  public:
    typedef T type;

    T		x;										///< x-component of the vector
    T		y;										///< y-component of the vector
    T		z;										///< z-component of the vector
    Vector3T()							   {}
    explicit Vector3T(T xyz)		           { x = y = z = xyz; }
    Vector3T(T cx, T cy, T cz)		           { x = cx;	y = cy;		z = cz; }
    template <class S> Vector3T(const S * v) { x = (T)v[0]; y = (T)v[1]; z = (T)v[2]; }
    template <class S> Vector3T(const Vector3T<S>& v)		   { x = (T)v.x;	y = (T)v.y; z = (T)v.z; }
    template <class S> Vector3T(const Vector2T<S>& v, S az)		   { x = (T)v.x;	y = (T)v.y; z = az; }
    template <class S> Vector3T& operator=(const Vector3T<S>& v)	   { x = (T)v.x; y = (T)v.y; z = (T)v.z; return *this; }
    Vector3T&	clear		(void)				   { x = (T)(0);  y = (T)(0); z = (T)(0); return *this;	}
    Vector3T&	make		(T cx, T cy, T cz)  { x = (cx); y = (cy); z = (cz); return *this; }
    Vector3T&	make		(T xyz)			   { x = (xyz); y = (xyz); z = (xyz); return *this; }
    T * data() { return &x; }
    const T * data() const { return &x; }
    bool		operator==  (const Vector3T& src) const		   { return (x == src.x && y == src.y && z == src.z); }
    bool		operator!=  (const Vector3T& src) const		   { return !(x == src.x && y == src.y && z == src.z); }

    Vector3T      operator+	(const Vector3T& v) const { return Vector3T(x + v.x, y + v.y, z + v.z); }
    Vector3T      operator-	(const Vector3T& v) const { return Vector3T(x - v.x, y - v.y, z - v.z); }
    Vector3T      operator-	()                  const { return Vector3T(-x, -y, -z); }
    Vector3T      operator*	(T s) const               { return Vector3T(x * s, y * s, z * s); }
    Vector3T      operator/	(T s) const               { return Vector3T(x / s, y / s, z / s); }

    Vector3T&	operator+=	(const Vector3T& v)		   { x += v.x; y += v.y; z += v.z;  return *this; }

    Vector3T&	operator-=	(const Vector3T& v)		   { x -= v.x; y -= v.y; z -= v.z;  return *this; }

    Vector3T&	operator*=	(T s)			   { x = (x*s), y = (y*s); z = (z*s); return *this; }
    Vector3T&	operator/=	(T s)			   { s = T(1)/s; x = (x*s), y = (y*s); z = (z*s); return *this; }

    bool		isOne		(void) const			   { return (x == 1.0f && y == 1.0f && z == 1.0f); }
    bool		isZero		(void) const			   { return (x == 0.0f && y == 0.0f && z == 0.0f); }
/// @todo Replace this - finite() is obsolete
//  bool isFinite (void) const { return finite(x) && finite(y) && finite(z); }
    double	length		(void) const			   { return (double)Math::Sqrt(x*x+y*y+z*z); }
    double	lengthSqr	(void) const			   { return x*x+y*y+z*z; }
    Vector3T&	negate		(void)				   { x=-x; y=-y; z=-z; return *this; }
    Vector3T&	normalize	(double len = 1.0)	   { double l = length(); if (l!=0.0) *this *= T(len/l); return *this; }
    Vector3T&	scale		(const Vector3T& v)		   { x *= v.x; y *= v.y; z *= v.z; return *this; }
    Vector3T&	descale		(const Vector3T& v)		   { x /= v.x; y /= v.y; z /= v.z; return *this; }
    Vector3T&	clampUnit	(void)				   { if(x <= (T)0.0) x = (T)0.0; else if(x >= (T)1.0) x = (T)1.0; if(y <= (T)0.0) y = (T)0.0; else if(y >= (T)1.0) y = (T)1.0; if(z <= (T)0.0) z = (T)0.0; else if(z >= (T)1.0) z = (T)1.0; return *this; }

    /// Returns a vector with components reordered.
    Vector3T    shuffle         (int i1, int i2, int i3) const { return Vector3T(get(i1), get(i2), get(i3)); }

    const	T&	operator[]	(int i) const	                   { return ((T*)this)[i]; }
    T&		operator[]	(int i)				   { return ((T*)this)[i]; }

    T&            get(int i)        { return ((T*)this)[i]; }
    const T&      get(int i) const  { return ((T*)this)[i]; }

    void		set(int i, T v)              			   { ((T*)this)[i] = v; }

    T             maximum() const { T t = x > y ? x : y; return t > z ? t : z; }
    T             minimum() const { T t = x < y ? x : y; return t < z ? t : z; }
    /// Sum of all components
    T             sum() const { return x + y + z; }

    template <class S>
    void copy(const S * data) { x = data[0]; y = data[1]; z = data[2]; }

    static Vector3T & cast(T * ptr) { return * (Vector3T *) ptr; }
    static const Vector3T & cast(const T * ptr) { return * (Vector3T *) ptr; }

    Vector2T<T> & vector2() { return * (Vector2T<T> *) this; }
    const Vector2T<T> & vector2() const { return * (Vector2T<T> *) this; }

    Vector2T<T> xy() const { return Vector2T<T>(x, y); }
  };

  /// Vector of three floats
  typedef Vector3T<float> Vector3;
  /// Vector of three floats
  typedef Vector3T<float> Vector3f;
  /// Vector of three unsigned chars
  typedef Vector3T<unsigned char> Vector3ub;
  /// Vector of three ints
  typedef Vector3T<int> Vector3i;
  /// Vector of three doubles
  typedef Vector3T<double> Vector3d;

  /// Multiply a vector with scalar
  template <class T>
  Nimble::Vector3T<T> operator* (T s, const Nimble::Vector3T<T>& v)
  { return v * s; }

} // namespace

template <class T>
inline T abs(Nimble::Vector3T<T> t)
{
  return t.length();
}

template <class S, class T>
inline T dot(const Nimble::Vector3T<S>& a, const Nimble::Vector3T<T>& b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

template <class S, class T>
inline T dot2(const Nimble::Vector3T<S>& a, const Nimble::Vector2T<T>& b)
{
  return a[0]*b[0] + a[1]*b[1];
}

template <class S, class T>
inline T dot3(const Nimble::Vector3T<S>& a, const Nimble::Vector2T<T>& b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2];
}

template <class T>
inline Nimble::Vector3T<T> cross(const Nimble::Vector3T<T>& a, const Nimble::Vector3T<T>& b)
{
  Nimble::Vector3T<T> v((a[1]*b[2])-(a[2]*b[1]),
               -(a[0]*b[2])+(a[2]*b[0]),
               (a[0]*b[1])-(a[1]*b[0]));
  return v;
}

template <class T>
inline std::ostream &operator<<(std::ostream &os, const Nimble::Vector3T<T> &t)
{
  os << t.x << ' ' << t.y << ' ' << t.z;
  return os;
}

template <class T>
inline std::istream &operator>>(std::istream &is, Nimble::Vector3T<T> &t)
{
  is >> t.x;
  is >> t.y;
  is >> t.z;
  return is;
}

/// @todo never use 'using' in a header file!
using Nimble::operator *;

#endif
