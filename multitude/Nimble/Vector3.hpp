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

#include <stdint.h>

namespace Nimble {

  /** Three-dimensional vector class for 3D mathematics. */

  template <class T>
  class NIMBLE_API Vector3T
  {
  public:
    /// Data type of the vector
    typedef T type;

    T		x;										///< x-component of the vector
    T		y;										///< y-component of the vector
    T		z;										///< z-component of the vector
    inline Vector3T()							   {}
    /// Constructs a vector initializing all components to given value
    inline explicit Vector3T(T xyz)		           { x = y = z = xyz; }
    /// Constructs a vector initializing it to given values
    inline Vector3T(T cx, T cy, T cz)		           { x = cx;	y = cy;		z = cz; }
    /// Constructs a vector copying values from memory
    template <class S> Vector3T(const S * v) { x = (T)v[0]; y = (T)v[1]; z = (T)v[2]; }
    /// Constructs a vector copying it from another vector
    template <class S> Vector3T(const Vector3T<S>& v)		   { x = (T)v.x;	y = (T)v.y; z = (T)v.z; }
    /// Constructs a vector using a 2d vector and a scalar component
    template <class S> Vector3T(const Vector2T<S>& v, S az)		   { x = (T)v.x;	y = (T)v.y; z = az; }
    /// Copies a vector
    template <class S> Vector3T& operator=(const Vector3T<S>& v)	   { x = (T)v.x; y = (T)v.y; z = (T)v.z; return *this; }
    /// Fills the vector with zeroes
    inline Vector3T&	clear		(void)				   { x = (T)(0);  y = (T)(0); z = (T)(0); return *this;	}
    /// Sets the vector to given values
    inline Vector3T&	make		(T cx, T cy, T cz)  { x = (cx); y = (cy); z = (cz); return *this; }
    /// Fills the vector with given value
    inline Vector3T&	make		(T xyz)			   { x = (xyz); y = (xyz); z = (xyz); return *this; }
    /// Returns a pointer to the first element
    inline T * data() { return &x; }
    /// Returns a pointer to the first element
    inline const T * data() const { return &x; }
    /// Compares if two vectors are equal
    inline bool		operator==  (const Vector3T& src) const		   { return (x == src.x && y == src.y && z == src.z); }
    /// Compares if two vectors differ
    inline bool		operator!=  (const Vector3T& src) const		   { return !(x == src.x && y == src.y && z == src.z); }

    /// Adds two vectors
    inline Vector3T      operator+	(const Vector3T& v) const { return Vector3T(x + v.x, y + v.y, z + v.z); }
    /// Subtract two vectors
    inline Vector3T      operator-	(const Vector3T& v) const { return Vector3T(x - v.x, y - v.y, z - v.z); }
    /// Returns the negation of the vector
    inline Vector3T      operator-	()                  const { return Vector3T(-x, -y, -z); }
    /// Multiplies a vector with a scalar
    inline Vector3T      operator*	(T s) const               { return Vector3T(x * s, y * s, z * s); }
    /// Divides a vector with a scalar
    inline Vector3T      operator/	(T s) const               { return Vector3T(x / s, y / s, z / s); }

    /// Adds two vectors
    inline Vector3T&	operator+=	(const Vector3T& v)		   { x += v.x; y += v.y; z += v.z;  return *this; }
    /// Subtract two vectors
    inline Vector3T&	operator-=	(const Vector3T& v)		   { x -= v.x; y -= v.y; z -= v.z;  return *this; }

    /// Multiplies a vector with a scalar
    inline Vector3T&	operator*=	(T s)			   { x = (x*s), y = (y*s); z = (z*s); return *this; }
    /// Divides a vector with a scalar
    inline Vector3T&	operator/=	(T s)			   { s = T(1)/s; x = (x*s), y = (y*s); z = (z*s); return *this; }

    /// Check if all components are one
    inline bool		isOne		(void) const			   { return (x == 1.0f && y == 1.0f && z == 1.0f); }
    /// Check if all components are zero
    inline bool		isZero		(void) const			   { return (x == 0.0f && y == 0.0f && z == 0.0f); }
    /// Returns the length of the vector
    inline double	length		(void) const			   { return (double)Math::Sqrt(x*x+y*y+z*z); }
    /// Returns the squared length of the vector
    inline double	lengthSqr	(void) const			   { return x*x+y*y+z*z; }
    /// Returns the negation of the vector
    inline Vector3T&	negate		(void)				   { x=-x; y=-y; z=-z; return *this; }
    /// Normalizes the vector to given length
    inline Vector3T&	normalize	(double len = 1.0)	   { double l = length(); if (l!=0.0) *this *= T(len/l); return *this; }
    /// Multiply component-wise
    inline Vector3T&	scale		(const Vector3T& v)		   { x *= v.x; y *= v.y; z *= v.z; return *this; }
    /// Divide component-wise
    inline Vector3T&	descale		(const Vector3T& v)		   { x /= v.x; y /= v.y; z /= v.z; return *this; }
    /// Clamps components to range [0,1]
    inline Vector3T&	clampUnit	(void)				   { if(x <= (T)0.0) x = (T)0.0; else if(x >= (T)1.0) x = (T)1.0; if(y <= (T)0.0) y = (T)0.0; else if(y >= (T)1.0) y = (T)1.0; if(z <= (T)0.0) z = (T)0.0; else if(z >= (T)1.0) z = (T)1.0; return *this; }

    /// Returns a vector with components reordered.
    inline Vector3T    shuffle         (int i1, int i2, int i3) const { return Vector3T(get(i1), get(i2), get(i3)); }

    /// Returns the ith component
    inline const	T&	operator[]	(int i) const	                   { return ((T*)this)[i]; }
    /// Multiply component-wise
    inline T&		operator[]	(int i)				   { return ((T*)this)[i]; }

    /// Multiply component-wise
    inline T&            get(int i)        { return ((T*)this)[i]; }
    /// Multiply component-wise
    inline const T&      get(int i) const  { return ((T*)this)[i]; }

    /// Sets the ith component
    inline void		set(int i, T v)              			   { ((T*)this)[i] = v; }

    /// Returns the largest component
    inline T             maximum() const { T t = x > y ? x : y; return t > z ? t : z; }
    /// Returns the smallest component
    inline T             minimum() const { T t = x < y ? x : y; return t < z ? t : z; }
    /// Sum of all components
    inline T             sum() const { return x + y + z; }

    //template <class S>
    //void copy(const S * data) { x = data[0]; y = data[1]; z = data[2]; }

    //static Vector3T & cast(T * ptr) { return * (Vector3T *) ptr; }
    //static const Vector3T & cast(const T * ptr) { return * (Vector3T *) ptr; }

    /// @todo duplicates (vector2(), xy())
    /// Returns a vector containing the two first components
    inline Vector2T<T> & vector2() { return * (Vector2T<T> *) this; }
    /// Returns a vector containing the two first components
    inline const Vector2T<T> & vector2() const { return * (Vector2T<T> *) this; }
    /// Returns a vector containing the two first components
    inline Vector2T<T> xy() const { return Vector2T<T>(x, y); }
  };

  /* A bunch of specializations, so that compiler does not warn about
     negating vectors with unsigned components.
  */
#ifdef WIN32
  template <>
      Vector3T<unsigned char>
      Vector3T<unsigned char>::operator-	() const { return * this; }

  template <>
      Vector3T<unsigned>
      Vector3T<unsigned>::operator-	() const { return * this; }

  template <>
      Vector3T<uint64_t>
      Vector3T<uint64_t>::operator-	() const { return * this; }

  template <>
      Vector3T<unsigned char> &
      Vector3T<unsigned char>::negate() { return * this; }

  template <>
      Vector3T<unsigned> &
      Vector3T<unsigned>::negate() { return * this; }

  template <>
      Vector3T<uint64_t> &
      Vector3T<uint64_t>::negate() { return * this; }
#endif
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
//using Nimble::operator *;

// These are needed under Windows
#ifdef WIN32
#   ifdef NIMBLE_EXPORT
        template Nimble::Vector3T<float>;
        template Nimble::Vector3T<unsigned char>;
        template Nimble::Vector3T<int>;
        template Nimble::Vector3T<double>;
        template Nimble::Vector3T<int64_t>;
        template Nimble::Vector3T<uint64_t>;
#   endif
#endif

#endif
