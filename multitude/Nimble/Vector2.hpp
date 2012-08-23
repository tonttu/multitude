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

#ifndef NIMBLE_VECTOR2T_HPP
#define NIMBLE_VECTOR2T_HPP

#include "Export.hpp"
#include "Math.hpp"

#include <limits>
#include <iostream>

namespace Nimble {

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

    enum { Elements = 2 };

    /// X-component of the vector
    T		x;
    /// Y-component of the vector
    T		y;

    /** Default constructor, does \b not initialize the values. */
    inline Vector2T () {}
    /// Constructs a vector initializing it to given values
    inline Vector2T (T cx, T cy) : x(cx), y(cy) {}

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
    /// Checks if both components are one
    inline bool	isOne		(void) const					        { return (x == (T) 1 && y == (T) 1); }
    /// Checks if both components are zero
    inline bool	isZero		(void) const					      { return (x == (T) 0 && y == (T) 0); }

    /// Returns the length of the vector
    auto length() const -> decltype(T() * 1.f)          { return Math::Sqrt(lengthSqr()); }
    /// Returns the squared length of the vector
    inline T      	lengthSqr	(void) const				      { return x*x+y*y; }
    /// Negates the vector
    inline Vector2T&	negate		(void)						      { x=-x; y=-y; return *this; }
    /// Normalizes the vector to the given length
    Vector2T& normalize(T len = T(1)) { auto l = length(); if(l != 0) *this *= (len / l); return *this; }
    /// Normalizes the vector to the given length if it is longer
    inline Vector2T&	limitLength	(T len)	 { auto l = length(); if (l > len) *this *= (len/l); return *this; }
    /// Scales the vector
    inline Vector2T&	scale		(const Vector2T& v)				{ x *= v.x; y *= v.y; return *this; }
    /// Scales the vector
    inline Vector2T&	scale		(const T & xs, const T & ys)				{ x *= xs; y *= ys; return *this; }

    /// Scales the vector with inverse of v
    inline Vector2T&	descale		(const Vector2T& v)			{ x /= v.x; y /= v.y; return *this; }
    /// Rotates the vector given the sine and cosine of the rotation angle
    inline Vector2T&	rotate		(double s, double c)		{ T t = x; x = (T)(x*c+y*-s); y = (T)(t*s+y*c); return *this; }
    /// Rotate the vector by given radians
    inline Vector2T&	rotate		(double angle)					{ return rotate(sin(angle), cos(angle)); }
    /// Returns atan2(y/x)
    inline double	angle		(void) const				        { return atan2(double(y), double(x)); }
    /// Clamps both components to the range [0,1]
    inline Vector2T&	clampUnit	(void)						{ return clamp(T(0.0), T(1.0)); }
    /// Clamps both components to the range [low, high]
    inline Vector2T&	clamp (T low, T high)       { x = Math::Clamp(x, low, high); y = Math::Clamp(y, low, high); return * this; }

    /// Returns the smaller component
    inline T           minimum         (void) const { return x < y ? x : y; }
    /// Returns the larger component
    inline T           maximum         (void) const { return x > y ? x : y; }
    /// Returns the sum of components
    inline T           sum             (void) const { return x + y; }
    /// Returns a vector with components reordered.
    inline Vector2T    shuffle         (int i1 = 1, int i2 = 0) const { return Vector2T(get(i1), get(i2)); }
    /// Returns a perpendicular vector
    inline Vector2T    perpendicular   () const { return Vector2T(-y, x); }

    /// Returns the ith component
    inline T&            get(int i)        { return ((T*)this)[i]; }
    /// Returns the ith component
    inline const T&      get(int i) const  { return ((T*)this)[i]; }

    /// Returns the ith component
    inline const	T&			operator[]	(int i) const		{ return ((T*)this)[i]; }
    /// Returns the ith component
    inline T&			        operator[]	(int i)				{ return ((T*)this)[i]; }

    /// Check that vector elements are finite
    /** This function can be useful if you suspect that contents of the
        vector might be corrupt floating point numbers.

        @return True if the vector elements are finite, false if are non-finite
        (i.e. infinite or nan).
    */
    inline bool isFinite() const { return Math::isFinite(x) && Math::isFinite(y); }

    /// Less-than operator, with arbitrary internal logic. This method is used
    /// if you want to sort vectors.
    /// @param v2 Other vector to compare to
    /// @return True if this vector should be sorted before v2
    inline bool operator< (const Vector2T<T>& v2) const
    {
      return x == v2.x ? y < v2.y : x < v2.x;
    }

    static inline Vector2T<T> null() { return Vector2T<T>(0, 0); }

    /// Cast the vector to another type
    template<typename S>
    Nimble::Vector2T<S> cast() const
    {
      return Nimble::Vector2T<S>(S(x), S(y));
    }
  };

  /// Add two vectors
  template <class T> inline	Vector2T<T>	operator+	(const Vector2T<T>& v1, const Vector2T<T>& v2) { return Vector2T<T>(v1.x+v2.x, v1.y+v2.y); }

  /// Subract two vectors
  template <class T> inline	Vector2T<T>	operator-	(const Vector2T<T>& v1, const Vector2T<T>& v2) { return Vector2T<T>(v1.x-v2.x, v1.y-v2.y); }

  /// Divide a vector by scalar
  template <class T> inline	Vector2T<T>	operator/	(const Vector2T<T>& v, const double s) { T r = T(1.0/s); return v*r; }
  /// Divide a vector by scalar
  template <class T> inline Vector2T<T> operator/ (const Vector2T<T>& v, const T s) { return Vector2T<T>(v.x / s, v.y / s); }
  /// Returns the negation of a vector
  template <class T> inline	Vector2T<T>	operator-	(const Vector2T<T>& v) { return Vector2T<T>(-v.x, -v.y); }

  namespace Math {
    /// Specialize Abs
    template <class T>
    inline T Abs(const Vector2T<T>& t)
    {
      return t.length();
    }
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

  // Multiply vector with a scalar (v * s)
  template<class T, class S>
  auto operator* (const Nimble::Vector2T<T> & v, S s) -> Nimble::Vector2T<decltype(T()*S())>
  {
    static_assert(std::is_arithmetic<S>::value, "vector multiplication operator is only defined to arithmetic types");
    return Nimble::Vector2T<decltype(T()*S())>(v.x * s, v.y * s);
  }

  // Multiply vector with a scalar (s * v)
  template<class T, class S>
  auto operator* (S s, const Nimble::Vector2T<T> & v) -> Nimble::Vector2T<decltype(T()*S())>
  {
    static_assert(std::is_arithmetic<S>::value, "vector multiplication operator is only defined to arithmetic types");
    return v * s;
  }

  namespace Math {

    /// Round the vector component-wise to the given vector
    template <class T>
    inline Vector2T<int> Round(const Vector2T<T>  & that)
    {
      return Vector2T<int>(Math::Round(that.x), Math::Round(that.y));
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
  /// Vector of two doubles
  typedef Vector2T<double> Vector2d;

  /// @todo this line stuff really doesn't belong in here!
  /// Line slope types.
  enum LineSlopeType
  {
    LS_VERTICAL,
    LS_SLOPING,
    LS_HORIZONTAL
  };

  /// Compute slope of line.
  /// @param lineStart Line starting point
  /// @param lineEnd Line end point
  /// @param slopeType reference to int to receive slope type.
  /// @param delta reference to Vector2f to receive delta.
  /// @return Slope value.
  inline float lineSlope(const Vector2f lineStart, const Vector2f lineEnd,
    int & slopeType, Vector2f & delta)
  {
    float   m = 0.0f;

    delta = lineEnd - lineStart;

    if(delta.x == 0.0f)
    {
      slopeType = LS_VERTICAL;
    }
    else if(delta.y == 0.0f)
    {
      slopeType = LS_HORIZONTAL;
    }
    else
    {
      slopeType = LS_SLOPING;
      m = delta.y / delta.x;
    }

    return m;
  }

  /// Test for intersection of line segments.
  /// @param line1Start, line1End first line.
  /// @param line2Start, line2End second line.
  /// @param interPoint optional pointer to vector to receive the intersection point.
  /// @return true if line segments intersect.
  template <typename T>
  bool linesIntersect(Vector2T<T> line1Start, Vector2T<T> line1End,
    Vector2T<T> line2Start, Vector2T<T> line2End,
    Vector2T<T> * interPoint = 0)
  {
    // Check if either line has zero length
    if((line1Start == line1End) || (line2Start == line2End))
      return false;

    // Get slope and deltas of first line
    int   slopeType1 = 0;
    Vector2f  delta1;
    const float   m1 = lineSlope(line1Start, line1End, slopeType1, delta1);

    // Get slope and deltas of second line
    int   slopeType2 = 0;
    Vector2f  delta2;
    const float   m2 = lineSlope(line2Start, line2End, slopeType2, delta2);

    // Determine whether infinite lines cross: if so compute line parameters
    bool  cross = false;
    T   t1(0);
    T   t2(0);

    switch(slopeType1)
    {
    case LS_VERTICAL:
      switch(slopeType2)
      {
      case LS_VERTICAL:
        // Lines parallel - no intersection point
        break;

      case LS_SLOPING:
        {
          cross = true;
          t2 = (line1Start.x - line2Start.x) / delta2.x;
          t1 = (line2Start.y + (t2 * delta2.y) - line1Start.y) / delta1.y;
        }
        break;

      case LS_HORIZONTAL:
        {
          cross = true;
          t1 = (line2Start.y - line1Start.y) / delta1.y;
          t2 = (line1Start.x - line2Start.x) / delta2.x;
        }
        break;
      }
      break;

    case LS_SLOPING:
      switch(slopeType2)
      {
      case LS_VERTICAL:
        {
          cross = true;
          t1 = (line2Start.x - line1Start.x) / delta1.x;
          t2 = (line1Start.y + (t1 * delta1.y) - line2Start.y) / delta2.y;
        }
        break;

      case LS_SLOPING:
        {
          if(m1 != m2) {
            cross = true;
            const float   value = delta2.x * delta1.y;
            const float   divisor = 1.0f - (delta1.x * delta2.y) / value;
            t1 = (line2Start.y / delta1.y + (line1Start.x * delta2.y) / value
              - (line2Start.x * delta2.y) / value - line1Start.y / delta1.y) / divisor;
            t2 = (line1Start.x + t1 * delta1.x - line2Start.x) / delta2.x;
          }
        }
        break;

      case LS_HORIZONTAL:
        {
          cross = true;
          t1 = (line2Start.y - line1Start.y) / delta1.y;
          t2 = (line1Start.x + (t1 * delta1.x) - line2Start.x) / delta2.x;
        }
        break;
      };
      break;

    case LS_HORIZONTAL:
      switch(slopeType2)
      {
      case LS_VERTICAL:
        {
          cross = true;
          t1 = (line2Start.x - line1Start.x) / delta1.x;
          t2 = (line1Start.y - line2Start.y) / delta2.y;
        }
        break;

      case LS_SLOPING:
        {
          cross = true;
          t2 = (line1Start.y - line2Start.y) / delta2.y;
          t1 = (line2Start.x + t2 * delta2.x - line1Start.x) / delta1.x;
        }
        break;

      case LS_HORIZONTAL:
        // Lines parallel - no intersection point
        break;
      }
      break;
    }

    if(!cross)
      return false;

    // Compute point of intersection
    if(interPoint)
      * interPoint = Vector2f(line1Start.x + t1 * delta1.x, line1Start.y + t1 * delta1.y);

    // Return true only if point of intersection is on both lines
    return(t1 >= 0.0f && t1 <= 1.0f && t2 >= 0.0f && t2 <= 1.0f);
  }

  template <class K, class T>
  inline K &operator<<(K &os, const Nimble::Vector2T<T> &t)
  {
    os << t.x << ' ' << t.y;
    return os;
  }

} // namespace

#endif
