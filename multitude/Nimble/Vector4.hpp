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

#ifndef NIMBLE_VECTOR4T_HPP
#define NIMBLE_VECTOR4T_HPP

#include <Nimble/Export.hpp>
#include <Nimble/Vector3.hpp>

#include <iostream>

namespace Nimble {

  /// A four-dimensional homogenic vector class for 3D graphics
  /** This class is used to represent homogenic coordinates for 3D
      calculations.

      Vector4T is also widely used to carry RGBA color values.*/
  template <class T>
  class NIMBLE_API Vector4T
  {
  public:
    typedef T type;
    /// The x-component
    T		x;
    /// The y-component
    T		y;
    /// The z-component
    T		z;
    /// The w-component
    T   w;
    Vector4T	()					       {}
    Vector4T	(T cx, T cy, T cz, T cw)                       { x = cx;       y = cy;	     z = cz;      w =  cw; }
    template <class S> Vector4T(const Vector4T<S>& v)	       { x = (T)v.x;   y = (T)v.y;  z = (T)v.z;  w = (T) v.w; }
    /// @todo remove the conversion (make static functions)
    template <class S> Vector4T(const S * v)	               { x = (T)v[0];  y = (T)v[1]; z = (T)v[2]; w = (T) v[3]; }
    template <class S> Vector4T& operator=(const Vector4T<S>& v) { x = (T)v.x; y = (T)v.y; z = (T)v.z; w = (T) v.w; return *this; }
    Vector4T&	clear(void)                                    { x = (T)(0);  y = (T)(0); z = (T)(0); w = (T)(0); return *this;	}
    bool		operator==  (const Vector4T& src) const        { return (x == src.x && y == src.y && z == src.z && w == src.w);	}
    bool		operator!=  (const Vector4T& src) const        { return !(x == src.x && y == src.y && z == src.z && w == src.w); }
    Vector4T&	operator+=  (const Vector4T& v)	               { x += v.x; y += v.y; z += v.z;  w += v.w; return *this; }
    Vector4T&	operator-=  (const Vector4T& v)                { x -= v.x; y -= v.y; z -= v.z;  w -= v.w; return *this; }
    Vector4T&	operator*=  (T s)		               { x = (x*s), y = (y*s); z = (z*s); w = (w*s); return *this; }
    Vector4T&	operator/=  (T s)		               { s = T(1)/s; x = (x*s), y = (y*s); z = (z*s); w = (w*s); return *this; }
    bool		isOne	    (void) const		       { return (x == 1.0f && y == 1.0f && z == 1.0f && w == 1.0f); }
    bool		isZero	    (void) const		       { return (x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f); }
    double	length	    (void) const		       { return sqrt(double(x*x+y*y+z*z+w*w)); }
    double	lengthSqr   (void) const		       { return x*x+y*y+z*z+w*w; }
    Vector4T&	normalize   (double len = 1.0)		       { double l = length(); if (l!=0.0) *this *= T(len/l); return *this; }
    Vector4T&	normalize3   (double len = 1.0)		       { vector3().normalize(len); return *this; }
    Vector4T&	scale		(const Vector4T& v)	       { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    Vector4T&	descale		(const Vector4T& v)	       { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }

    /// Returns a vector with components reordered.
    Vector4T    shuffle         (int i1, int i2, int i3, int i4) const { return Vector4T(get(i1), get(i2), get(i3), get(i4)); }

    const	T&	operator[]	(int i) const		       { return ((T*)this)[i]; }
    T&		operator[]	(int i)			       { return ((T*)this)[i]; }

    void 	        make(T cx, T cy, T cz, T cw)                   { x = cx; y = cy; z = cz; w = cw; }

    T *           data() { return &x; }
    const T *     data() const { return &x; }

    T&            get(int i)        { return ((T*)this)[i]; }
    const T&      get(int i) const  { return ((T*)this)[i]; }

    T             maximum() const { T q = x>y?x:y; T a = z>w?z:w; return q>a?q:a; }

    /// Cast to Vector2T
    const Vector2T<T> & xy() const { return * (Vector2T<T> *) this; }
    /// Cast to Vector2T
    const Vector2T<T> & vector2() const { return * (Vector2T<T> *) this; }
    /// Cast to Vector3T
    Vector3T<T> & vector3() { return * (Vector3T<T> *) this; }
    const Vector3T<T> & vector3() const { return * (Vector3T<T> *) this; }

    /// Copy some data
    template <class S>
    void copy(const S * data) { x = data[0]; y = data[1]; z = data[2]; w = data[3]; }
    /** Copies three elements without affecting the fourth element. */
    template <class S>
    void copy3(const S * data) { x = data[0]; y = data[1]; z = data[2]; }
  };

  /// Add two vectors
  template <class T> inline	Vector4T<T>	operator+	(const Vector4T<T>& v1, const Vector4T<T>& v2)	{ Vector4T<T> t(v1); t+=v2; return t; }
  /// Subtract two vectors
  template <class T> inline	Vector4T<T>	operator-	(const Vector4T<T>& v1, const Vector4T<T>& v2)	{ Vector4T<T> t(v1); t-=v2; return t; }
  /// Multiply a vector by scalar
  template <class T> inline	Vector4T<T>	operator*	(const Vector4T<T>& v, const double s)		{ Vector4T<T> t(v); t*=s; return t; }
  /// Multiply a vector by scalar
  template <class T> inline	Vector4T<T>	operator*	(const double s, const Vector4T<T>& v)		{ return v*s; }
  /// Divide a vector by scalar
  template <class T> inline	Vector4T<T>	operator/	(const Vector4T<T>& v, const double s)		{ double r = 1.0/s; return v*r; }
  /// Returns the negation of a vector
  template <class T> inline	Vector4T<T>	operator-	(const Vector4T<T>& v)						{ return Vector4T<T>(-v.x, -v.y, -v.z, -v.w); }

  /// Vector of four floats
  typedef Vector4T<float> Vector4;
  /// Vector of four floats
  typedef Vector4T<float> Vector4f;
  /// Vector of four unsigned chars
  typedef Vector4T<unsigned char> Vector4ub;
  /// Vector of four ints
  typedef Vector4T<int> Vector4i;
  /// Vector of four doubles
  typedef Vector4T<double> Vector4d;

} // namespace

template <class T>
inline T abs(Nimble::Vector4T<T> t)
{
  return t.length();
}

/// Returns 4D dot product of the two Vector4T objects.
template <class T>
inline T dot(const Nimble::Vector4T<T>& a, const Nimble::Vector4T<T>& b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
}

/// Returns 3D dot product of Vector4T and Vector3T objects.
template <class T>
inline T dot3(const Nimble::Vector4T<T>& a, const Nimble::Vector3T<T>& b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

/// Returns 3D dot product of Vector3T and Vector4T objects.
template <class T>
inline T dot3(const Nimble::Vector3T<T>& a, const Nimble::Vector4T<T>& b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

template <class T>
inline T dot4(const Nimble::Vector3T<T>& a, const Nimble::Vector4T<T>& b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + b[3];
}

template <class T>
inline T dot4(const Nimble::Vector4T<T>& a, const Nimble::Vector3T<T>& b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3];
}

/// Serialize a 4D vector into a stream
template <class T>
inline std::ostream &operator<<(std::ostream &os, const Nimble::Vector4T<T> &t)
{
  os << t.x << ' ' << t.y << ' ' << t.z << ' ' << t.w;
  return os;
}

/// De-serialize a 4D vector from a stream
template <class T>
inline std::istream &operator>>(std::istream &is, Nimble::Vector4T<T> &t)
{
  is >> t.x;
  is >> t.y;
  is >> t.z;
  is >> t.w;
  return is;
}

#endif
