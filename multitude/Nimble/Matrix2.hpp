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

#ifndef NIMBLE_MATRIX2T_HPP
#define NIMBLE_MATRIX2T_HPP

#include <Nimble/Export.hpp>
#include <Nimble/Math.hpp>
#include <Nimble/Vector2.hpp>

#include <assert.h>

namespace Nimble {

  /// 2x2 transformation matrix
  /** The rows of this matrix are of type Nimble::Vector2T<T>. */
  template <class T>
  class Matrix2T
  {
  public:
    /// Create a matrix without initializing the elements
    Matrix2T() {}
    Matrix2T(T v11, T v12, T v21, T v22) { m[0].make(v11, v12); m[1].make(v21, v22); }
    Matrix2T(const Matrix2T &that) { m[0] = that.m[0]; m[1] = that.m[1]; }
    template <class S>
    Matrix2T(const Vector2T<S> & r1, const Vector2T<S> & r2)
    { m[0] = r1; m[1] = r2; }
    /// Empty destructor
    /** This method is defined because otherwise so compilers might
	  complain. We expect that a decent compiler knows how to
	  eliminate this function. */
    ~Matrix2T() {}

    void make(T v11, T v12, T v21, T v22) { m[0].make(v11, v12); m[1].make(v21, v22); }

    /// Returns a reference to a row
    Nimble::Vector2T<T>&       row(int i)             { return m[i]; }
    /// Returns a constant reference to a row
    const Nimble::Vector2T<T>& row(int i) const       { return m[i]; }
    /// Returns a copy of a column
    Nimble::Vector2T<T>        column(int i) const    { return Vector2T<T>(m[0][i],m[1][i]); }
    /// Returns a reference to a row
    Nimble::Vector2T<T>&       operator[](int i)      { return row(i); }
    /// Returns a constant reference to a row
    const Nimble::Vector2T<T>& operator[](int i) const{ return row(i); }
    /// Sets the given matrix element
    void               set(int r, int c, T v) { m[r][c] = v; }
    /// Gets the given matrix element
    T                  get(int r, int c) const{ return m[r][c]; }
    /// Returns a pointer to the matrix data.
    T *                data() { return m[0].data(); }
    /// Returns a constant pointer to the matrix data.
    const T *          data() const { return m[0].data(); }

    /// Transpose this matrix
    void               transpose()            { swap(m[0][1],m[1][0]); }
    /// Transpose this matrix
    Matrix2T           transposed() const
    { return Matrix2T(m[0][0], m[1][0], m[0][1], m[1][1]); }
    /// Set all matrix elements to zero
    void               clear()                { m[0].clear(); m[1].clear(); } 
    /// Create an identity matrix
    void               identity()             { m[0].make(1.0, 0.0); m[1].make(0.0, 1.0);}
    /// Create a rotation matrix
    inline void        rotate(T);
    /// Create a scaling matrix
    void               scale(T s)             { m[0].make(s, 0.0); m[1].make(0.0, s); }

    void               add(T v)        { m[0][0]+=v;m[0][1]+=v;m[1][0]+=v;m[1][1]+=v; }

    inline Matrix2T inverse() const;

    float det() const                  { return m[0][0]*m[1][1] - m[0][1] * m[1][0]; }

    Matrix2T operator *= (T s)         { m[0] *= s; m[1] *= s; return * this; }

    /// Returns the number of rows in this matrix type
    static int         rows() { return 2; }
    /// Returns the number of columns in this matrix type
    static int         columns() { return 2; }

    /// Returns a rotation matrix
    static Matrix2T rotation(T r) { T c = Math::Cos(r); T s = Math::Sin(r); return Matrix2T(c, -s, s, c); }
    /// Returns a scaling matrix
    static Matrix2T scaling(T s)  { Matrix2T m; m.identity(); m.set(0, 0, s); m.set(1, 1, s); return m; }
    
    /** Identity matrix. */
    NIMBLE_API static const Matrix2T<T> IDENTITY;

  private:
    inline static void swap(T &a, T& b);

    Vector2T<T> m[2];
  };

  template <class T>
  inline void Matrix2T<T>::swap(T &a, T& b)
  {
    T t = a;
    a = b;
    b = t;
  }

  template <class T>
  inline void Matrix2T<T>::rotate(T a)
  {
    T ca = Math::Cos(a);
    T sa = Math::Sin(a);
    m[0].make(ca, -sa);
    m[1].make(sa, ca);
  }

  template <class T>
  inline Matrix2T<T> Matrix2T<T>::inverse() const
  {
    Matrix2T<T> inv;

    // Code from WidMagic4

    T fDet = det();
    if (Math::Abs(fDet) > 1.0e-8) {
        T fInvDet = ((T)1.0)/fDet;
        inv.data()[0] =  data()[3]*fInvDet;
        inv.data()[1] = -data()[1]*fInvDet;
        inv.data()[2] = -data()[2]*fInvDet;
        inv.data()[3] =  data()[0]*fInvDet;
    }
    else
    {
        inv.data()[0] = (T)0.0;
        inv.data()[1] = (T)0.0;
        inv.data()[2] = (T)0.0;
        inv.data()[3] = (T)0.0;
    }

    return inv;
  }

  /// Multiply two matrices together
  template <class T>
  inline Matrix2T<T> operator*(const Matrix2T<T>& m1, const Matrix2T<T>& m2)
  {
    Matrix2T<T> res;

    for(int i = 0; i < 2; i++) {
      Vector2T<T> t = m2.column(i);
      res[0][i] = dot(m1.row(0),t);
      res[1][i] = dot(m1.row(1),t);
    }

    return res;
  }

  /// Multiply a matrix and a vector
  template <class T>
  inline Vector2T<T> operator*(const Matrix2T<T>& m1,const Vector2T<T>& m2)
  {
    Vector2T<T> res;
    for(int i = 0; i < 2; i++)
      res[i] = dot(m1.row(i),m2);
    return res;
  }

  /// Multiply a matrix and a vector
  template <class T>
  inline Vector2T<T> operator*(const Vector2T<T>& m2, const Matrix2T<T>& m1)
  {
    Vector2T<T> res;
    for(int i = 0; i < 2; i++)
      res[i] = dot(m1.column(i),m2);
    return res;
  }

  /// Add two matrices together
  template <class T>
  inline Matrix2T<T> operator+(const Matrix2T<T>& m1, const Matrix2T<T>& m2)
  {
    return Matrix2T<T>(m1[0] + m2[0], m1[1] + m2[1]);
  }

  /// Subtract a matrix from another
  template <class T>
  inline Matrix2T<T> operator-(const Matrix2T<T>& m1, const Matrix2T<T>& m2)
  {
    return Matrix2T<T>(m1[0] - m2[0], m1[1] - m2[1]);
  }

  /// 2x2 matrix of floats
  typedef Matrix2T<float> Matrix2;
  /// 2x2 matrix of floats
  typedef Matrix2T<float> Matrix2f;

}

template <class S, class T>
inline S &operator<<(S &os, const Nimble::Matrix2T<T> &t)
{
  os << t[0].x << ' ' << t[0].y << " ; " << t[1].x << ' ' << t[1].y;
  return os;
}


template <class T>
inline Nimble::Matrix2T<T> mulColByRowVector
(const Nimble::Vector2T<T>& v1, const Nimble::Vector2T<T>& v2)
{
  return Nimble::Matrix2T<T>(v1.x * v2.x, v1.y * v2.x,
                             v1.x * v2.y, v1.y * v2.y);
}


#endif
