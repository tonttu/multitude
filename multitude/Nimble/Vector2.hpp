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

#include <Nimble/Export.hpp>
#include <Nimble/Math.hpp>

#include <iostream>

namespace Nimble {

  /** Two-dimensional vector class for 2D mathematics.

      Like all classed in Nimble Vector2T has been optimized for
      speed. In general, there are no safety checks in any
      functions. */
  template <class T>
  class NIMBLE_API Vector2T
  {
  public:
    /// Data type of the vector
    typedef T type;

    /// X-component of the vector
    T		x;
    /// Y-component of the vector
    T		y;

    /** Default constructor, does \b not initialize the values. */
    Vector2T () {}
    /// Constructs a vector initializing it to given values
    Vector2T (T cx, T cy) { x = (T)cx;	y = (T)cy; }
    /// Constructs a vector initializing from memory
    template <class S> Vector2T(const S * v) { x = v[0]; y = v[1]; }
    /// Copy constructor
    template <class S> Vector2T	(const Vector2T<S>& v) { x = (T)v.x; y = (T)v.y; }

    //template <class S> Vector2T& operator=  (const Vector2T<S>& v)	{ x = (T)v.x; y = (T)v.y; return *this; }

    /// Fill the vector with zeroes
    Vector2T&	clear		(void)					{ x = (T)(0); y = (T)(0); return *this; }
    /// Set the vector to given values
    Vector2T&	make		(T cx, T cy)		{ x = cx; y = cy; return *this; }
    /// Set both components to the given value
    Vector2T&	make		(T xy)					{ x = (xy); y = (xy); return *this; }
    /// Returns a pointer to the first element
    T *         data() { return &x; }
    /// Returns a pointer to the first element
    const T *   data() const { return &x; }
    /// Compares if two vectors are equal
    bool	operator==  (const Vector2T& src) const		                { return (x == src.x && y == src.y); }
    /// Compares if two vectors differ
    bool	operator!=  (const Vector2T& src) const		                { return !(x == src.x && y == src.y); }
    /// Adds two vectors
    Vector2T&	operator+=	(const Vector2T& v)				{ x += v.x, y += v.y; return *this; }
    /// Subtracts two vectors
    Vector2T&	operator-=	(const Vector2T& v)				{ x -= v.x, y -= v.y; return *this; }
    /// Multiplies a vector with a scalar
    Vector2T&	operator*=	(T s)					        { x = (x*s), y = (T)(y*s); return *this; }
    /// Divides a vector with a scalar
    Vector2T&	operator/=	(T s)					        { s = T(1)/s; x = (x*s), y = (y*s); return *this; }
    /// Checks if both components are one
    bool	isOne		(void) const					        { return (x == (T) 1 && y == (T) 1); }
    /// Checks if both components are zero
    bool	isZero		(void) const					      { return (x == (T) 0 && y == (T) 0); }
    /// Returns the length of the vector
    T    	length		(void) const				        { return (T)Math::Sqrt(x*x+y*y); }
    /// Returns the squared length of the vector
    T      	lengthSqr	(void) const				      { return x*x+y*y; }
    /// Negates the vector
    Vector2T&	negate		(void)						      { x=-x; y=-y; return *this; }
    /// Normalizes the vector to the given length
    Vector2T&	normalize	(T len = T(1))			{ T l = length(); if (l!= T(0)) *this *= (len/l); return *this; }
    /// Normalizes the vector to the given length if it is longer
    Vector2T&	limitLength	(T len)	 { T l = length(); if (l > len) *this *= (len/l); return *this; }
    /// Scales the vector
    Vector2T&	scale		(const Vector2T& v)				{ x *= v.x; y *= v.y; return *this; }
    /// Scales the vector
    Vector2T&	scale		(const T & xs, const T & ys)				{ x *= xs; y *= ys; return *this; }

    /// Scales the vector with inverse of v
    Vector2T&	descale		(const Vector2T& v)			{ x /= v.x; y /= v.y; return *this; }
    /// Rotates the vector given the sine and cosine of the rotation angle
    Vector2T&	rotate		(double s, double c)		{ T t = x; x = (T)(x*c+y*-s); y = (T)(t*s+y*c); return *this; }
    /// Rotate the vector by given radians
    Vector2T&	rotate		(double angle)					{ return rotate(sin(angle), cos(angle)); }
    /// Returns atan2(y/x)
    double	angle		(void) const				        { return atan2(double(y), double(x)); }
    /// Clamps both components to the range [0,1]
    Vector2T&	clampUnit	(void)						{ if(x <= (T)0.0) x = (T)0.0; else if(x >= (T)1.0) x = (T)1.0; if(y <= (T)0.0) y = (T)0.0; else if(y >= (T)1.0) y = (T)1.0; return *this; }
    /// Clamps both components to the range [low, high]
    Vector2T&	clamp (T low, T high)       { x = Math::Clamp(x, low, high); y = Math::Clamp(y, low, high); return * this; }

    /// Returns the smaller component
    T           minimum         (void) const { return x < y ? x : y; }
    /// Returns the larger component
    T           maximum         (void) const { return x > y ? x : y; }
    /// Returns the sum of components
    T           sum             (void) const { return x + y; }
    /// Returns a vector with components reordered.
    Vector2T    shuffle         (int i1 = 1, int i2 = 0) const { return Vector2T(get(i1), get(i2)); }
    /// Returns a perpendicular vector
    Vector2T    perpendicular   () const { return Vector2T(-y, x); }

    /// Returns the ith component
    T&            get(int i)        { return ((T*)this)[i]; }
    /// Returns the ith component
    const T&      get(int i) const  { return ((T*)this)[i]; }

    /// Returns the ith component
    const	T&			operator[]	(int i) const		{ return ((T*)this)[i]; }
    /// Returns the ith component
    T&			        operator[]	(int i)				{ return ((T*)this)[i]; }

    /// Check that vector elements are finite
    /** This function can be useful if you suspect that contents of the
        vector might be corrupt floating point numbers.

        @return True if the vector elements are finite, false if are non-finite
        (i.e. infinite or nan).
    */
    bool isFinite() const { return Math::isFinite(x) && Math::isFinite(y); }

    /** Less-than operator, with arbitrary internal logic. This method is used
        if you want to sort vectors. */
    bool operator< (const Vector2T<T>& v2) const
    {
      return x == v2.x ? y < v2.y : x < v2.x;
    }
    //template <class S>
    //void copy(const S * data) { x = data[0]; y = data[1]; }
  };

  /// Add two vectors
  template <class T> inline	Vector2T<T>	operator+	(const Vector2T<T>& v1, const Vector2T<T>& v2) { return Vector2T<T>(v1.x+v2.x, v1.y+v2.y); }

  //template <class T> inline	Vector2T<T>	operator+	(const Vector2T<T>& v1, T v2) { return Vector2T<T>(v1.x+v2, v1.y+v2); }
  /// Subract two vectors
  template <class T> inline	Vector2T<T>	operator-	(const Vector2T<T>& v1, const Vector2T<T>& v2) { return Vector2T<T>(v1.x-v2.x, v1.y-v2.y); }
  //template <class T> inline	Vector2T<T>	operator-	(const Vector2T<T>& v1, T v2) { return Vector2T<T>(v1.x-v2, v1.y-v2); }
  /// Multiply a vector by scalar
  template <class T> inline	Vector2T<T>	operator*	(const Vector2T<T>& v, const T s) { return Vector2T<T>((T)(v.x*s), (T)(v.y*s)); }
  /// Multiply a vector by scalar
  template <class T> inline	Vector2T<T>	operator*	(const T s, const Vector2T<T>& v) { return v*s; }
  /// Divide a vector by scalar
  template <class T> inline	Vector2T<T>	operator/	(const Vector2T<T>& v, const double s) { T r = T(1.0/s); return v*r; }
  /// Divide a vector by scalar
  template <class T> inline Vector2T<T> operator/ (const Vector2T<T>& v, const T s) { return Vector2T<T>(v.x / s, v.y / s); }
  /// Returns the negation of a vector
  template <class T> inline	Vector2T<T>	operator-	(const Vector2T<T>& v) { return Vector2T<T>(-v.x, -v.y); }

  /// @cond
  template <>
  inline Vector2T<int> & Vector2T<int>::operator /= (int s)
  {
    x = x/s;
    y = y/s;
    return *this;
  }
  /// @endcond

/*
  template <class T>
  inline float abs(Vector2T<T> t)
  {
    return t.length();
  }
*/
  /// Compute the dot product of two vectors
  /// @param t1 first dot product vector
  /// @param t2 second dot product vector
  template <class T>
  inline float dot(const Vector2T<T> &t1, const Vector2T<T> &t2)
  {
    return t1.x * t2.x + t1.y * t2.y;
  }

  /* Note that these overloads are NOT redundant, integer math is
     different from floating point math. */
  /// Divide a vector by scalar
  inline Vector2T<short> operator / (const Vector2T<short>& v, const short s)
  {
    return Vector2T<short>(v.x / s, v.y / s);
  }
  /// Divide a vector by scalar
  inline Vector2T<int> operator / (const Vector2T<int>& v, const int s)
  {
    return Vector2T<int>(v.x / s, v.y / s);
  }
  /// Divide a vector by scalar
  inline Vector2T<long> operator / (const Vector2T<long>& v, const long s)
  {
    return Vector2T<long>(v.x / s, v.y / s);
  }
  /// Multiply a vector by scalar
  inline Vector2T<short> operator * (const Vector2T<short>& v, const short s)
  {
    return Vector2T<short>(v.x * s, v.y * s);
  }
  /// Multiply a vector by scalar
  inline Vector2T<int> operator * (const Vector2T<int>& v, const int s)
  {
    return Vector2T<int>(v.x * s, v.y * s);
  }
  /// Multiply a vector by scalar
  inline Vector2T<long> operator * (const Vector2T<long>& v, const long s)
  {
    return Vector2T<long>(v.x * s, v.y * s);
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
  NIMBLE_API bool linesIntersect(Nimble::Vector2f line1Start, Nimble::Vector2f line1End,
                      Nimble::Vector2f line2Start, Nimble::Vector2f line2End,
                      Nimble::Vector2f * interPoint = 0);

} // namespace


template <class S, class T>
inline S &operator<<(S &os, const Nimble::Vector2T<T> &t)
{
  os << t.x << ' ' << t.y;
  return os;
}

// These are needed under Windows
#ifdef WIN32
#   ifdef NIMBLE_EXPORT
        template Nimble::Vector2T<float>;
        template Nimble::Vector2T<unsigned char>;
        template Nimble::Vector2T<int>;
        template Nimble::Vector2T<double>;
#   endif
#endif

#endif
