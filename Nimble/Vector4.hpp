/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_VECTOR4T_HPP
#define NIMBLE_VECTOR4T_HPP

#include "Vector3.hpp"

#include <limits>
#include <iostream>

namespace Nimble {

  /// A four-dimensional homogenic vector class for 3D graphics
  /** This class is used to represent homogenic coordinates for 3D
      calculations.

      Vector4T is also widely used to carry RGBA color values.*/
  template <class T>
  class Vector4T
  {
  public:
    /// Data type of the vector
    typedef T type;

    enum { ELEMENTS = 4 };

    /// The x-component
    T		x;
    /// The y-component
    T		y;
    /// The z-component
    T		z;
    /// The w-component
    T   w;

    inline Vector4T	() = default;
    /// Constructs a vector and initializes it with the given values
    inline Vector4T (const Vector2T<T> & v, T cz, T cw) : x(v.x), y(v.y), z(cz), w(cw) {}
    /// Constructs a vector and initializes it with the given values
    inline Vector4T (const Vector3T<T> & v, T cw) : x(v.x), y(v.y), z(v.z), w(cw) {}
    /// Constructs a vector and initializes it with the given values
    inline Vector4T (T cx, T cy, T cz, T cw) : x(cx), y(cy), z(cz), w(cw) {}
    /// Fills the vector with zeroes
    inline void	clear(void)                                    { x = (T)(0);  y = (T)(0); z = (T)(0); w = (T)(0);	}

    /// Compares if two vectors are equal
    inline bool operator==  (const Vector4T& src) const
    {
      static const T eps = std::numeric_limits<T>::epsilon();
      return
        x >= src.x - eps && x<= src.x + eps && y >= src.y - eps && y<= src.y + eps &&
        z >= src.z - eps && z<= src.z + eps && w >= src.w - eps && w<= src.w + eps;
    }

    /// Compares if two vectors differ
    inline bool         operator!=  (const Vector4T& src) const        { return !operator==(src); }

    /// Adds two vectors
    inline Vector4T      operator+	(const Vector4T& v) const { return Vector4T(x + v.x, y + v.y, z + v.z, w + v.w); }
    /// Subtract two vectors
    inline Vector4T      operator-	(const Vector4T& v) const { return Vector4T(x - v.x, y - v.y, z - v.z, w - v.w); }
    /// Returns the negation of the vector
    inline Vector4T      operator-	()                  const { return Vector4T(-x, -y, -z, -w); }
    /// Multiplies a vector with a scalar
    inline Vector4T      operator*	(T s) const               { return Vector4T(x * s, y * s, z * s, w * s); }
    /// Divides a vector with a scalar
    inline Vector4T      operator/	(T s) const               { return Vector4T(x / s, y / s, z / s, w / s); }

    /// Adds two vectors
    inline Vector4T&	operator+=  (const Vector4T& v)	               { x += v.x; y += v.y; z += v.z;  w += v.w; return *this; }
    /// Subtracts two vectors
    inline Vector4T&	operator-=  (const Vector4T& v)                { x -= v.x; y -= v.y; z -= v.z;  w -= v.w; return *this; }
    /// Multiplies a vector by scalar
    inline Vector4T&	operator*=  (T s)		               { x *= s; y *= s; z *= s; w *= s; return *this; }
    /// Divides a vector by scalar
    inline Vector4T&	operator/=  (T s)		               { x /= s; y /= s; z /= s; w /= s; return *this; }

    /// Checks if all components are one
    inline bool		isOne	    (void) const		       { return (x == 1.0f && y == 1.0f && z == 1.0f && w == 1.0f); }
    /// Checks if all components are zero
    inline bool		isZero	    (void) const		       { return (x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f); }
    /// Returns the length of the vector
    inline typename Decltype<T, float>::mul length() const { return std::sqrt(lengthSqr()); }
   /// Returns the squared length of the vector
    inline typename Decltype<T, float>::mul lengthSqr() const { return x*x+y*y+z*z+w*w; }
    /// Normalizes the vector to the given length
    /// @param len length to normalize to
    /// @return reference to this
    inline Vector4T&	normalize   (double len = 1.0)		       { double l = length(); if (l!=0.0) *this *= T(len/l); return *this; }
    /// Get a vector normalized to given length
    /// @param len length to normalize to
    /// @return normalized vector
    inline Vector4T	normalized  (double len = 1.0)	const	       { auto v = *this; v.normalize(len); return v; }
    /// Multiplies the vector component-wise
    inline Vector4T&	scale		(const Vector4T& v)	       { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    /// Divides the vector component-wise
    inline Vector4T&	descale		(const Vector4T& v)	       { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }

    /// Clamps both components to the range [0,1]
    inline Vector4T&	clampUnit	(void)						{ return clamp(T(0.0), T(1.0)); }
    /// Clamps both components to the range [low, high]
    inline Vector4T&	clamp (T low, T high)       { x = Nimble::Math::Clamp(x, low, high); y = Nimble::Math::Clamp(y, low, high); z = Nimble::Math::Clamp(z, low, high); w = Nimble::Math::Clamp(w, low, high); return * this; }

    /// Returns a vector with components reordered.
    inline Vector4T    shuffle         (int i1, int i2, int i3, int i4) const { return Vector4T(get(i1), get(i2), get(i3), get(i4)); }

    /// Returns the ith component
    inline const	T&	operator[]	(size_t i) const { return ((T*)this)[i]; }
    /// Returns the ith component
    inline T&		operator[]	(size_t i)           { return ((T*)this)[i]; }

    /// Sets the vector to given values
    inline Nimble::Vector4T<T> & make(const Vector2T<T> & v, T cz, T cw)                   { x = v.x; y = v.y; z = cz; w = cw; return *this; }

    /// Sets the vector to given values
    inline Nimble::Vector4T<T> & make(const Vector3T<T> & v, T cw)                   { x = v.x; y = v.y; z = v.z; w = cw; return *this; }

    /// Sets the vector to given values
    inline Nimble::Vector4T<T> & make(T cx, T cy, T cz, T cw)                   { x = cx; y = cy; z = cz; w = cw; return *this; }

    /// Returns the largest component
    inline T             maximum() const { return std::max(std::max(x,y),std::max(y,w)); }
    /// Returns the smallest component
    inline T             minimum() const { return std::min(std::min(x,y),std::min(y,w)); }
    /// Sum of all components
    inline T             sum() const { return x + y + z + w; }

    /// Returns a pointer to the first component
    inline T *           data() { return &x; }
    /// Returns a pointer to the first component
    inline const T *     data() const { return &x; }

    /// Returns the ith component
    inline T&            get(size_t i)        { return ((T*)this)[i]; }
    /// Returns the ith component
    inline const T&      get(size_t i) const  { return ((T*)this)[i]; }

    /// Sets the ith component
    inline void		set(size_t i, T v)        { ((T*)this)[i] = v; }

    /// Returns a copy of the first two components as a Vector2
    /// @return New vector2
    inline Vector2T<T> vector2() const { return Vector2T<T>(x, y); }

    /// Makes a new Nimble::Vector2f of two freely selected components of vector4
    /// @param i0 Index of the first component,  vec2.x = vec4[i0], 0..3
    /// @param i1 Index of the second component, vec2.y = vec4[i1], 0..3
    /// @return New vector2
    inline Vector2T<T> vector2(size_t i0, size_t i1) const
    {
      return Vector2T<T>(get(i0), get(i1));
    }

    /// Returns a copy of the first three components as a Vector3
    /// @return New vector3
    inline Vector3T<T> vector3() const { return Vector3T<T>(x, y, z); }

    /// Makes a new vector3 of two freely selected components of vector4
    /// @param i0 Index of the first component,  vec3.x = vec4[i0], 0..3
    /// @param i1 Index of the second component, vec3.y = vec4[i1], 0..3
    /// @param i2 Index of the third component,  vec3.z = vec4[i2], 0..3
    /// @return New vector2
    inline Vector3T<T> vector3(size_t i0, size_t i1, size_t i2) const
    {
      return Vector3T<T>(get(i0), get(i1), get(i2));
    }

    /// Zero vector
    /// @return a zero vector
    static inline Vector4T<T> null() { return Vector4T<T>(0, 0, 0, 0); }

    /// Cast the vector to another type
    template<typename S>
    Nimble::Vector4T<S> cast() const
    {
      return Nimble::Vector4T<S>(S(x), S(y), S(z), S(w));
    }

    /// Cast the vector to another type and round the values with std::round
    template <typename S>
    Nimble::Vector4T<S> round() const
    {
      return Nimble::Vector4T<S>(S(Nimble::Math::Roundf(x)), S(Nimble::Math::Roundf(y)),
                                 S(Nimble::Math::Roundf(z)), S(Nimble::Math::Roundf(w)));
    }
  };

  /// Add two vectors
  template <class T> inline	Vector4T<T>	operator+	(const Vector4T<T>& v1, const Vector4T<T>& v2)	{ Vector4T<T> t(v1); t+=v2; return t; }
  /// Subtract two vectors
  template <class T> inline	Vector4T<T>	operator-	(const Vector4T<T>& v1, const Vector4T<T>& v2)	{ Vector4T<T> t(v1); t-=v2; return t; }

  /// Multiply vector with a scalar (v * s)
  /// @param v vector to multiply
  /// @param s scalar to multiply with
  /// @return scaled vector
  template<class T, class S, typename = typename std::enable_if<std::is_arithmetic<S>::value>::type>
  Vector4T<typename Decltype<T, S>::mul> operator* (const Nimble::Vector4T<T> & v, S s)
  {
    return Nimble::Vector4T<decltype(T()*S())>(v.x * s, v.y * s, v.z * s, v.w * s);
  }

  /// Multiply vector with a scalar (s * v)
  /// @param v vector to multiply
  /// @param s scalar to multiply with
  /// @return scaled vector
  template<class T, class S, typename = typename std::enable_if<std::is_arithmetic<S>::value>::type>
  Vector4T<typename Decltype<S, T>::mul> operator* (S s, const Nimble::Vector4T<T> & v)
  {
    return v * s;
  }

  /// Vector of four floats
  typedef Vector4T<float> Vector4;
  /// Vector of four floats
  typedef Vector4T<float> Vector4f;
  /// Vector of four unsigned chars
  typedef Vector4T<unsigned char> Vector4ub;
  /// Vector of four ints
  typedef Vector4T<int> Vector4i;
  /// Vector of four unsigned ints
  typedef Vector4T<unsigned int> Vector4u;
  /// Vector of four doubles
  typedef Vector4T<double> Vector4d;

  /// Get the length of the vector
  /// @param t vector whose length to get
  /// @return length of the vector
  template <class T>
  inline T abs(const Vector4T<T> & t)
  {
    return t.length();
  }

  /// Returns 4D dot product of the two Vector4T objects.
  template <class T>
  inline T dot(const Vector4T<T> & a, const Vector4T<T> & b)
  {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
  }

  /// Compute the projection of v onto u
  /// @param u vector to project onto
  /// @param v vector to project
  /// @return the projection of v onto u
  template <class T>
  inline Vector4T<T> projection(const Vector4T<T> & u, const Vector4T<T> & v)
  {
    return (dot(v, u)/u.lengthSqr())*u;
  }

  /// Serialize a 4D vector into a stream
  template <class T>
  inline std::ostream & operator<<(std::ostream & os, const Nimble::Vector4T<T> & t)
  {
    os << t.x << ' ' << t.y << ' ' << t.z << ' ' << t.w;
    return os;
  }

  /// De-serialize a 4D vector from a stream
  template <class T>
  inline std::istream & operator>>(std::istream & is, Nimble::Vector4T<T> & t)
  {
    is >> t.x;
    is >> t.y;
    is >> t.z;
    is >> t.w;
    return is;
  }
} // namespace

namespace std
{
  // Define std::abs for Vector4T
  template <typename T>
  inline T abs(const Nimble::Vector4T<T> & v)
  {
    return Nimble::abs(v);
  }
} 
#endif
