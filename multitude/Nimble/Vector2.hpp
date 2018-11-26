/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_VECTOR2T_HPP
#define NIMBLE_VECTOR2T_HPP

#include "Export.hpp"
#include "Nimble.hpp"
#include "Math.hpp"

#include <limits>
#include <iostream>

namespace Nimble {

  template <class T>
  class SizeT;

  /** Two-dimensional vector class for 2D mathematics.

      Like all classed in Nimble Vector2T has been optimized for
      speed. In general, there are no safety checks in any
      functions. */
  template <class T>
  class Vector2T
  {
  public:
    /// Data type of the vector
    typedef T type;

    enum { ELEMENTS = 2 };

    /// X-component of the vector
    T		x;
    /// Y-component of the vector
    T		y;

    /** Default constructor, does \b not initialize the values. */
    inline Vector2T () {}
    /// Constructs a vector initializing it to given values
    inline Vector2T (T cx, T cy) : x(cx), y(cy) {}

    /// Constructs vector from the size.
    /// @param s Size which width and height are copied to x and y values of vector.
    inline explicit Vector2T(const SizeT<T> & s) : x(s.width()), y(s.height()) {}

    /// Fill the vector with zeroes
    inline void clear		(void)					{ x = (T)(0); y = (T)(0); }
    /// Set the vector to given values
    inline Vector2T<T> &	make		(T cx, T cy)		{ x = cx; y = cy; return *this; }
    /// Set both components to the given value
    inline Vector2T<T> &	make		(T xy)					{ x = (xy); y = (xy); return *this; }
    /// Returns a pointer to the first element
    inline  T *         data() { return &x; }
    /// Returns a pointer to the first element
    inline  const T *   data() const { return &x; }
    /// Compares if two vectors are equal
   	inline bool operator==  (const Vector2T& src) const
	{
      static const T eps = std::numeric_limits<T>::epsilon();
      return
        x >= src.x - eps && x<= src.x + eps && y >= src.y - eps && y<= src.y + eps;
    }

    /// Compares if two vectors differ
    inline bool	operator!=  (const Vector2T& src) const { return !operator==(src); }
    /// Adds two vectors
    inline Vector2T&	operator+=	(const Vector2T& v)				{ x += v.x, y += v.y; return *this; }
    /// Subtracts two vectors
    inline Vector2T&	operator-=	(const Vector2T& v)				{ x -= v.x, y -= v.y; return *this; }
    /// Multiplies a vector with a scalar
    inline Vector2T&	operator*=	(T s)					        { x = (x*s), y = (T)(y*s); return *this; }
    /// Divides a vector with a scalar
    inline Vector2T&	operator/=	(T s)					        { x /= s; y /= s; return *this; }
    /// Divide a vector by scalar
    inline Vector2T   operator/ (T s) const { return Vector2T<T>(x / s, y / s); }
    /// Returns the negation of a vector
    inline	Vector2T	operator-	() const { return Vector2T<T>(-x, -y); }

    /// Checks if both components are one
    inline bool	isOne		(void) const					        { return (x == (T) 1 && y == (T) 1); }
    /// Checks if both components are zero
    inline bool	isZero		(void) const					      { return (x == (T) 0 && y == (T) 0); }

    /// Returns the length of the vector
    typename Decltype<T, float>::mul length() const                 { return std::sqrt(lengthSqr()); }
    /// Returns the squared length of the vector
    inline T      	lengthSqr	(void) const				      { return x*x+y*y; }
    /// Negates the vector
    inline Vector2T&	negate		(void)						      { x=-x; y=-y; return *this; }
    /// Normalizes the vector to the given length
    /// @param len length to normalize to
    /// @return reference to this
    Vector2T& normalize(T len = T(1)) { auto l = length(); if(l != 0) *this *= (len / l); return *this; }
    /// Get a vector normalized to given length
    /// @param len length to normalize to
    /// @return normalized vector
    Vector2T normalized(T len = T(1)) const { auto v = *this; v.normalize(len); return v; }
    /// Normalizes the vector to the given length if it is longer
    inline Vector2T&	limitLength	(T len)	 { auto l = length(); if (l > len) *this *= (len/l); return *this; }
    /// Scales the vector
    inline Vector2T&	scale		(const Vector2T& v)				{ x *= v.x; y *= v.y; return *this; }
    /// Scales the vector
    inline Vector2T&	scale		(const T & xs, const T & ys)				{ x *= xs; y *= ys; return *this; }

    /// Scales the vector with inverse of v
    inline Vector2T&	descale		(const Vector2T& v)			{ x /= v.x; y /= v.y; return *this; }
    /// Rotates the vector given the sine and cosine of the rotation angle
    template <typename F>
    inline Vector2T & rotate (F s, F c) { T t = x; x = (T)((x*c)+(y*-s)); y = (T)((t*s)+(y*c)); return *this; }
    /// Rotate the vector by given radians
    template <typename F>
    inline Vector2T & rotate(F angle) { return rotate(std::sin(angle), std::cos(angle)); }
    /// Returns atan2(y/x)
    template <typename F = float>
    inline F angle() const { return std::atan2(F(y), F(x)); }
    /// Clamps both components to the range [0,1]
    inline Vector2T&	clampUnit	(void)						{ return clamp(T(0.0), T(1.0)); }
    /// Clamps both components to the range [low, high]
    inline Vector2T&	clamp (T low, T high)       { x = Nimble::Math::Clamp(x, low, high); y = Nimble::Math::Clamp(y, low, high); return * this; }

    /// Returns the smaller component
    inline T           minimum         (void) const { return x < y ? x : y; }
    /// Returns the larger component
    inline T           maximum         (void) const { return x > y ? x : y; }
    /// Returns the sum of components
    inline T           sum             (void) const { return x + y; }
    /// Returns a vector with components reordered.
    inline Vector2T    shuffle         (int i1 = 1, int i2 = 0) const { return Vector2T(get(i1), get(i2)); }
    /// Returns a perpendicular vector. Same as rotating the vector by 90 degrees.
    inline Vector2T    perpendicular   () const { return Vector2T(-y, x); }

    /// Returns the ith component
    inline T&            get(size_t i)        { return ((T*)this)[i]; }
    /// Returns the ith component
    inline const T&      get(size_t i) const  { return ((T*)this)[i]; }

    /// Sets the ith component
    inline void		set(size_t i, T v)              			   { ((T*)this)[i] = v; }

    /// Returns the ith component
    inline const	T&			operator[]	(size_t i) const		{ return ((T*)this)[i]; }
    /// Returns the ith component
    inline T&			        operator[]	(size_t i)				{ return ((T*)this)[i]; }

    /// Check that vector elements are finite
    /** This function can be useful if you suspect that contents of the
        vector might be corrupt floating point numbers.

        @return True if the vector elements are finite, false if are non-finite
        (i.e. infinite or nan).
    */
    inline bool isFinite() const { return Nimble::Math::isFinite(x) && Nimble::Math::isFinite(y); }

    /// Less-than operator, with arbitrary internal logic. This method is used
    /// if you want to sort vectors.
    /// @param v2 Other vector to compare to
    /// @return True if this vector should be sorted before v2
    inline bool operator< (const Vector2T<T>& v2) const
    {
      return x == v2.x ? y < v2.y : x < v2.x;
    }

    /// Zero vector
    /// @return a zero vector
    static inline Vector2T<T> null() { return Vector2T<T>(0, 0); }

    /// Cast the vector to another type
    template<typename S>
    Nimble::Vector2T<S> cast() const
    {
      return Nimble::Vector2T<S>(S(x), S(y));
    }

    /// Cast the vector to another type and round the values with std::round
    template<typename S>
    Nimble::Vector2T<S> round() const
    {
      return Nimble::Vector2T<S>(S(Nimble::Math::Roundf(x)), S(Nimble::Math::Roundf(y)));
    }
  };

  /// Add two vectors
  template <class T> inline	Vector2T<T>	operator+	(const Vector2T<T>& v1, const Vector2T<T>& v2) { return Vector2T<T>(v1.x+v2.x, v1.y+v2.y); }

  /// Subract two vectors
  template <class T> inline	Vector2T<T>	operator-	(const Vector2T<T>& v1, const Vector2T<T>& v2) { return Vector2T<T>(v1.x-v2.x, v1.y-v2.y); }

  /// Return the length of the vector
  /// @param t vector whose length to get
  /// @return length of the vector
  template <class T>
  inline T abs(const Nimble::Vector2T<T> & t)
  {
    return t.length();
  }
  /// Compute the dot product of two vectors
  /// @param t1 first dot product vector
  /// @param t2 second dot product vector
  /// @return Dot product
  template <class T>
  inline T dot(const Vector2T<T> &t1, const Vector2T<T> &t2)
  {
    return t1.x * t2.x + t1.y * t2.y;
  }

  /// Compute the cross product of two 2d vectors by assuming the z components are zero
  /// Returns the magnitude (z component) of the resulting vector
  template<class T>
  inline T cross(const Vector2T<T> & a, const Vector2T<T> & b)
  {
    return (a.x * b.y) - (a.y * b.x);
  }

  /// Write a vector into a stream
  template <class T>
  inline std::ostream &operator<<(std::ostream &os, const Nimble::Vector2T<T> &t)
  {
    os << t.x << ' ' << t.y;
    return os;
  }

  /// Read a vector from a stream
  template <class T>
  inline std::istream &operator>>(std::istream &is, Nimble::Vector2T<T> &t)
  {
    is >> t.x;
    is >> t.y;
    return is;
  }

  /// Multiply vector with a scalar (v * s)
  /// @param v vector to multiply
  /// @param s scalar to multiply with
  /// @return scaled vector
  template<class T, class S, typename = typename std::enable_if<std::is_arithmetic<S>::value>::type>
  Vector2T<typename Decltype<T, S>::mul> operator* (const Nimble::Vector2T<T> & v, S s)
  {
    return Nimble::Vector2T<decltype(T()*S())>(v.x * s, v.y * s);
  }

  /// Multiply vector with a scalar (s * v)
  /// @param v vector to multiply
  /// @param s scalar to multiply with
  /// @return scaled vector
  template<class T, class S, typename = typename std::enable_if<std::is_arithmetic<S>::value>::type>
  Vector2T<typename Decltype<S, T>::mul> operator* (S s, const Nimble::Vector2T<T> & v)
  {
    return v * s;
  }

  namespace Math {

    /// Round the vector component-wise to the given vector
    template <class T>
    inline Vector2T<int> Round(const Vector2T<T>  & that)
    {
      return Vector2T<int>(Math::Round(that.x), Nimble::Math::Round(that.y));
    }
  }

  /// Vector of two floats
  typedef Vector2T<float> Vector2;
  /// Vector of two floats
  typedef Vector2T<float> Vector2f;
  /// Vector of two unsigned chars
  typedef Vector2T<unsigned char> Vector2ub;
  /// Vector of two ints
  typedef Vector2T<int> Vector2i;
  /// Vector of two unsigned ints
  typedef Vector2T<unsigned int> Vector2u;
  /// Vector of two doubles
  typedef Vector2T<double> Vector2d;
} // namespace

namespace std
{
  // Define std::abs for Vector2T
  template <typename T>
  inline T abs(const Nimble::Vector2T<T> & v)
  {
    return Nimble::abs(v);
  }
} 
#endif
