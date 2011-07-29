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

#include "Export.hpp"
#include "Vector3.hpp"

#include <limits>
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
    /// Data type of the vector
    typedef T type;
    /// The x-component
    T		x;
    /// The y-component
    T		y;
    /// The z-component
    T		z;
    /// The w-component
    T   w;

    inline Vector4T	() {}
    /// Constructs a vector copying the given value to all vector values
    /// @param xyzw Value to use to initialize all vector values
    inline explicit Vector4T(T xyzw) : x(xyzw), y(xyzw), z(xyzw), w(xyzw) { }
    /// Constructs a vector and initializes it with the given values
    inline Vector4T (T cx, T cy, T cz, T cw) : x(cx), y(cy), z(cz), w(cw) {}
    /// Copy constructor
    template <class S> inline Vector4T(const Vector4T<S>& v)	       { x = (T)v.x;   y = (T)v.y;  z = (T)v.z;  w = (T) v.w; }
    /// @todo remove the conversion (make static functions)
    /// Constructs a vector from memory
    /// @param v array of four decimals
    template <class S> inline Vector4T(const S * v)	               { x = (T)v[0];  y = (T)v[1]; z = (T)v[2]; w = (T) v[3]; }
    //template <class S> Vector4T& operator=(const Vector4T<S>& v) { x = (T)v.x; y = (T)v.y; z = (T)v.z; w = (T) v.w; return *this; }
    /// Fills the vector with zeroes
    inline Vector4T&	clear(void)                                    { x = (T)(0);  y = (T)(0); z = (T)(0); w = (T)(0); return *this;	}
	
    /// Compares if two vectors are equal
    inline bool operator==  (const Vector4T& src) const
    {
      static const float eps = std::numeric_limits<T>::epsilon();
      return
        x >= src.x - eps && x<= src.x + eps && y >= src.y - eps && y<= src.y + eps &&
        z >= src.z - eps && z<= src.z + eps && w >= src.w - eps && w<= src.w + eps;
    }

    /// Compares if two vectors differ
    inline bool         operator!=  (const Vector4T& src) const        { return !operator==(src); }
    /// Adds two vectors
    inline Vector4T&	operator+=  (const Vector4T& v)	               { x += v.x; y += v.y; z += v.z;  w += v.w; return *this; }
    /// Subtracts two vectors
    inline Vector4T&	operator-=  (const Vector4T& v)                { x -= v.x; y -= v.y; z -= v.z;  w -= v.w; return *this; }
    /// Multiplies a vector by scalar
    inline Vector4T&	operator*=  (T s)		               { x = (x*s), y = (y*s); z = (z*s); w = (w*s); return *this; }
    /// Divides a vector by scalar
    inline Vector4T&	operator/=  (T s)		               { s = T(1)/s; x = (x*s), y = (y*s); z = (z*s); w = (w*s); return *this; }
    /// Checks if all components are one
    inline bool		isOne	    (void) const		       { return (x == 1.0f && y == 1.0f && z == 1.0f && w == 1.0f); }
    /// Checks if all components are zero
    inline bool		isZero	    (void) const		       { return (x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f); }
    /// Returns the length of the vector
    inline double	length	    (void) const		       { return sqrt(double(x*x+y*y+z*z+w*w)); }
   /// Returns the squared length of the vector
    inline double	lengthSqr   (void) const		       { return x*x+y*y+z*z+w*w; }
    /// Normalizes the vector to given length
    inline Vector4T&	normalize   (double len = 1.0)		       { double l = length(); if (l!=0.0) *this *= T(len/l); return *this; }
    /// Normalizes the first three components to given length
    inline Vector4T&	normalize3   (double len = 1.0)		       { vector3().normalize(len); return *this; }
    /// Multiplies the vector component-wise
    inline Vector4T&	scale		(const Vector4T& v)	       { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    /// Divides the vector component-wise
    inline Vector4T&	descale		(const Vector4T& v)	       { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }

	/// Clamps both components to the range [0,1]
	inline Vector4T&	clampUnit	(void)						{ return clamp(T(0.0), T(1.0)); }
	/// Clamps both components to the range [low, high]
	inline Vector4T&	clamp (T low, T high)       { x = Math::Clamp(x, low, high); y = Math::Clamp(y, low, high); z = Math::Clamp(z, low, high); w = Math::Clamp(w, low, high); return * this; }

    /// Returns a vector with components reordered.
    inline Vector4T    shuffle         (int i1, int i2, int i3, int i4) const { return Vector4T(get(i1), get(i2), get(i3), get(i4)); }

    /// Returns the ith component
    inline const	T&	operator[]	(int i) const		       { return ((T*)this)[i]; }
    /// Returns the ith component
    inline T&		operator[]	(int i)			       { return ((T*)this)[i]; }

    /// Sets the vector to given values
    inline void 	        make(T cx, T cy, T cz, T cw)                   { x = cx; y = cy; z = cz; w = cw; }

    /// Returns a pointer to the first component
    inline T *           data() { return &x; }
    /// Returns a pointer to the first component
    inline const T *     data() const { return &x; }

    /// Returns the ith component
    inline T&            get(int i)        { return ((T*)this)[i]; }
    /// Returns the ith component
    inline const T&      get(int i) const  { return ((T*)this)[i]; }

    /// Returns the largest component
    inline T             maximum() const { T q = x>y?x:y; T a = z>w?z:w; return q>a?q:a; }

    /// Casts the first two components to Vector2
    /// @return Const reference to the XY -components
    inline const Vector2T<T> & vector2() const { return * (Vector2T<T> *) this; }
    /// Makes a new vector2 of two freely selected components of vector4
    /// @param i0 Index of the first component,  vec2.x = vec4[i0], 0..3
    /// @param i1 Index of the second component, vec2.y = vec4[i1], 0..3
    /// @return New vector2
    inline Vector2T<T> vector2(int i0, int i1) const
    {
      return Vector2T<T>(get(i0), get(i1));
    }
    /// Casts the first three components to Vector3
    /// @return Const reference to the XYZ -components
    inline const Vector3T<T> & vector3() const { return * (Vector3T<T> *) this; }
    /// Makes a new vector3 of two freely selected components of vector4
    /// @param i0 Index of the first component,  vec3.x = vec4[i0], 0..3
    /// @param i1 Index of the second component, vec3.y = vec4[i1], 0..3
    /// @param i2 Index of the third component,  vec3.z = vec4[i2], 0..3
    /// @return New vector2
    inline Vector3T<T> vector3(int i0, int i1, int i2) const
    {
      return Vector3T<T>(get(i0), get(i1), get(i2));
    }
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


  namespace Math {
    /// Specialize Abs
    template <class T>
    inline float Abs(const Vector4T<T>& t)
    {
      return t.length();
    }
  }
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

template <class T>
inline Nimble::Vector4T<T> projection(const Nimble::Vector4T<T>& u, const Nimble::Vector4T<T>& v)
{
  return (dot(v, u)/u.lengthSqr())*u;
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

// These are needed under Windows
#ifdef WIN32
#   ifdef NIMBLE_EXPORT
        template Nimble::Vector4T<float>;
        template Nimble::Vector4T<unsigned char>;
        template Nimble::Vector4T<int>;
        template Nimble::Vector4T<double>;
#   endif
#endif

#endif
