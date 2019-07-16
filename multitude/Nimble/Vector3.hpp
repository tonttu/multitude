/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_VECTOR3T_HPP
#define NIMBLE_VECTOR3T_HPP

#include "Export.hpp"
#include "Vector2.hpp"

#include <limits>
#include <iostream>

#include <cstdint>

namespace Nimble {

  /** Three-dimensional vector class for 3D mathematics. */

  template <class T>
  class Vector3T
  {
  public:
    /// Data type of the vector
    typedef T type;

    enum { ELEMENTS = 3 };

    T		x;										///< x-component of the vector
    T		y;										///< y-component of the vector
    T		z;										///< z-component of the vector
    inline Vector3T() = default;
    /// Constructs a vector initializing it to given values
    inline Vector3T(T cx, T cy, T cz) : x(cx), y(cy), z(cz) {}
    /// Constructs a vector using a 2d vector and a scalar component
    inline Vector3T(const Vector2T<T>& v, T az)		   { x = v.x;	y = v.y; z = az; }
    /// Fills the vector with zeroes
    inline void	clear		(void)				   { x = (T)(0);  y = (T)(0); z = (T)(0); }
    /// Sets the vector to given values
    inline Vector3T&	make		(const Vector2T<T>& v, T cz)  { x = v.x; y = v.y; z = (cz); return *this; }
    /// Sets the vector to given values
    inline Vector3T&	make		(T cx, T cy, T cz)  { x = (cx); y = (cy); z = (cz); return *this; }
    /// Fills the vector with given value
    inline Vector3T&	make		(T xyz)			   { x = (xyz); y = (xyz); z = (xyz); return *this; }
    /// Returns a pointer to the first element
    inline T * data() { return &x; }
    /// Returns a pointer to the first element
    inline const T * data() const { return &x; }
    /// Compares if two vectors are equal
	inline bool operator==  (const Vector3T& src) const
	{
      static const T eps = std::numeric_limits<T>::epsilon();
      return
        x >= src.x - eps && x<= src.x + eps &&
        y >= src.y - eps && y<= src.y + eps &&
        z >= src.z - eps && z<= src.z + eps;
    }
    /// Compares if two vectors differ
    inline bool		operator!=  (const Vector3T& src) const { return !operator==(src); }

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
    inline Vector3T&	operator*=	(T s)			   { x *= s; y *= s; z *= s; return *this; }
    /// Divides a vector with a scalar
    inline Vector3T&	operator/=	(T s)			   { x /= s; y /= s; z /= s; return *this; }

    /// Check if all components are one
    inline bool		isOne		(void) const			   { return (x == 1.0f && y == 1.0f && z == 1.0f); }
    /// Check if all components are zero
    inline bool		isZero		(void) const			   { return (x == 0.0f && y == 0.0f && z == 0.0f); }
    /// Returns the length of the vector
    inline double	length		(void) const			   { return (double)std::sqrt(x*x+y*y+z*z); }
    /// Returns the squared length of the vector
    inline double	lengthSqr	(void) const			   { return x*x+y*y+z*z; }
    /// Returns the negation of the vector
    inline Vector3T&	negate		(void)				   { x=-x; y=-y; z=-z; return *this; }
    /// Normalizes the vector to the given length
    /// @param len length to normalize to
    /// @return reference to this
    inline Vector3T&	normalize	(double len = 1.0)	   { double l = length(); if (l!=0.0) *this *= T(len/l); return *this; }
    /// Get a vector normalized to given length
    /// @param len length to normalize to
    /// @return normalized vector
    inline Vector3T	normalized (double len = 1.0)	const { auto v = *this; v.normalize(len); return v; }
    /// Multiply component-wise
    inline Vector3T&	scale		(const Vector3T& v)		   { x *= v.x; y *= v.y; z *= v.z; return *this; }
    /// Divide component-wise
    inline Vector3T&	descale		(const Vector3T& v)		   { x /= v.x; y /= v.y; z /= v.z; return *this; }
    /// Clamps components to range [0,1]
    inline Vector3T&	clampUnit	(void)				   { return clamp(T(0.0), T(1.0)); }
    /// Clamps all components to the range [low, high]
    inline Vector3T&	clamp (T low, T high)       { x = Nimble::Math::Clamp(x, low, high); y = Nimble::Math::Clamp(y, low, high);  z = Nimble::Math::Clamp(z,low, high); return * this; }
    /// Returns a vector with components reordered.
    inline Vector3T    shuffle         (int i1, int i2, int i3) const { return Vector3T(get(i1), get(i2), get(i3)); }

    /// Returns the ith component
    inline const	T&	operator[]	(size_t i) const  { return ((T*)this)[i]; }
    /// Multiply component-wise
    inline T&		operator[]	(size_t i)            { return ((T*)this)[i]; }

    /// Multiply component-wise
    inline T&            get(size_t i)                { return ((T*)this)[i]; }
    /// Multiply component-wise
    inline const T&      get(size_t i) const          { return ((T*)this)[i]; }

    /// Sets the ith component
    inline void		set(size_t i, T v)                { ((T*)this)[i] = v; }

    /// Returns the largest component
    inline T             maximum() const { T t = x > y ? x : y; return t > z ? t : z; }
    /// Returns the smallest component
    inline T             minimum() const { T t = x < y ? x : y; return t < z ? t : z; }
    /// Sum of all components
    inline T             sum() const { return x + y + z; }

    /// Returns a copy of the first two components as a Vector2
    /// @return New vector2
    inline Vector2T<T> vector2() const { return Vector2T<T>(x, y); }
    /// Makes a new Nimble::Vector2f of two freely selected components of vector3
    /// @param i0 Index of the first component,  vec2.x = vec3[i0], 0..2
    /// @param i1 Index of the second component, vec2.y = vec3[i1], 0..2
    /// @return New vector2
    inline Vector2T<T> vector2(int i0, int i1) const
    {
      return Vector2T<T>(get(i0), get(i1));
    }

    /// Zero vector
    /// @return a zero vector
    static inline Vector3T<T> null() { return Vector3T<T>(0, 0, 0); }

    /// Cast the vector to another type
    template<typename S>
    Nimble::Vector3T<S> cast() const
    {
      return Nimble::Vector3T<S>(S(x), S(y), S(z));
    }

    /// Cast the vector to another type and round the values with std::round
    template<typename S>
    Nimble::Vector3T<S> round() const
    {
      return Nimble::Vector3T<S>(S(Nimble::Math::Roundf(x)), S(Nimble::Math::Roundf(y)), S(Nimble::Math::Roundf(z)));
    }
  };

  /// Vector of three floats
  typedef Vector3T<float> Vector3;
  /// Vector of three floats
  typedef Vector3T<float> Vector3f;
  /// Vector of three unsigned chars
  typedef Vector3T<unsigned char> Vector3ub;
  /// Vector of three ints
  typedef Vector3T<int> Vector3i;
  /// Vector of three unsigned ints
  typedef Vector3T<unsigned int> Vector3u;
  /// Vector of three doubles
  typedef Vector3T<double> Vector3d;

  /// Multiply vector with a scalar (v * s)
  /// @param v vector to multiply
  /// @param s scalar to multiply with
  /// @return scaled vector
  template<class T, class S, typename = typename std::enable_if<std::is_arithmetic<S>::value>::type>
  Vector3T<typename Decltype<T, S>::mul> operator* (const Nimble::Vector3T<T> & v, S s)
  {
    return Nimble::Vector3T<decltype(T()*S())>(v.x * s, v.y * s, v.z * s);
  }

  /// Multiply vector with a scalar (s * v)
  /// @param v vector to multiply
  /// @param s scalar to multiply with
  /// @return scaled vector
  template<class T, class S, typename = typename std::enable_if<std::is_arithmetic<S>::value>::type>
  Vector3T<typename Decltype<S, T>::mul> operator* (S s, const Nimble::Vector3T<T> & v)
  {
    return v * s;
  }

  /// Return the length of the vector
  /// @param t vector whose length to get
  /// @return length of the vector
  template <class T>
  inline T abs(const Nimble::Vector3T<T> & t)
  {
    return t.length();
  }
  /// Compute the dot product of two vectors
  /// @param a first vector
  /// @param b second vector
  /// @return the dot product of the given vectors
  template <class K, class T>
  inline T dot(const Nimble::Vector3T<K>& a, const Nimble::Vector3T<T>& b)
  {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
  }

  /// Return the cross-product of two vectors
  /// @param a first vector
  /// @param b second vector
  /// @return cross-product of the two vectors
  template <class T>
  inline Nimble::Vector3T<T> cross(const Nimble::Vector3T<T>& a, const Nimble::Vector3T<T>& b)
  {
    Nimble::Vector3T<T> v((a[1]*b[2])-(a[2]*b[1]),
        -(a[0]*b[2])+(a[2]*b[0]),
        (a[0]*b[1])-(a[1]*b[0]));
    return v;
  }

  /// Output the given vector to a stream
  /// @param os stream to output to
  /// @param t vector to output
  /// @return reference to the stream
  template <class T>
  inline std::ostream &operator<<(std::ostream &os, const Nimble::Vector3T<T> &t)
  {
    os << t.x << ' ' << t.y << ' ' << t.z;
    return os;
  }

  /// Read the given vector from a stream
  /// @param is stream to read from
  /// @param t vector to read
  /// @return reference to the stream
  template <class T>
  inline std::istream &operator>>(std::istream &is, Nimble::Vector3T<T> &t)
  {
    is >> t.x;
    is >> t.y;
    is >> t.z;
    return is;
  }

  /// Output the given vector to a stream
  /// @param os stream to output to
  /// @param t vector to output
  /// @return reference to the stream
  template <>
  inline std::ostream &operator<<(std::ostream &os, const Nimble::Vector3T<uint8_t> &t)
  {
    os << int(t.x) << ' ' << int(t.y) << ' ' << int(t.z);
    return os;
  }

  /// Read the given vector from a stream
  /// @param is stream to read from
  /// @param t vector to read
  /// @return reference to the stream
  template <>
  inline std::istream &operator>>(std::istream &is, Nimble::Vector3T<uint8_t> &t)
  {
    int x, y, z;
    is >> x >> y >> z;
    t.x = x;
    t.y = y;
    t.z = z;
    return is;
  }

} // namespace

namespace std
{
  // Define std::abs for Vector3T
  template <typename T>
  inline T abs(const Nimble::Vector3T<T> & v)
  {
    return Nimble::abs(v);
  }
} 

#endif
