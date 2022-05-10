/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_MATRIX2T_HPP
#define NIMBLE_MATRIX2T_HPP

#include "Export.hpp"
#include "Math.hpp"
#include "Vector2.hpp"

#include <cassert>
#include <ostream>

namespace Nimble {

  /// 2x2 transformation matrix
  /** The rows of this matrix are of type Nimble::Vector2T<T>. */
  template <class T>
  class Matrix2T
  {
  public:
    /// Data-type of the matrix
    typedef T type;

    /// Creates a matrix without initializing the elements
    Matrix2T() = default;
    /// Constructs a matrix and initializes it to the given values
    Matrix2T(T v11, T v12, T v21, T v22) { m[0].make(v11, v12); m[1].make(v21, v22); }
    /// Copy constructor
    Matrix2T(const Matrix2T &that) { m[0] = that.m[0]; m[1] = that.m[1]; }
    /// Creates a matrix and initializes it with the given row vectors
    template <class K>
        Matrix2T(const Vector2T<K> & r1,
                 const Vector2T<K> & r2)
    { m[0] = r1; m[1] = r2; }
    /// Empty destructor
    /** This method is defined because otherwise some compilers might
      complain. We expect that a decent compiler knows how to
      eliminate this function. */
    ~Matrix2T() {}

    /// Fills the matrix with the given values
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
    void               transpose()            { std::swap(m[0][1],m[1][0]); }
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
    /// Adds the given scalar to each element
    void               add(T v)        { m[0][0]+=v;m[0][1]+=v;m[1][0]+=v;m[1][1]+=v; }
    /// Returns the inverse of the matrix
    inline Matrix2T inverse(bool * ok = 0, T tolerance = 1.0e-8) const;
    /// Returns the determinant of the matrix
    float det() const                  { return m[0][0]*m[1][1] - m[0][1] * m[1][0]; }
    /// Multiplies the matrix by a scalar
    Matrix2T operator *= (T s)         { m[0] *= s; m[1] *= s; return * this; }

    /// Compare if two matrices are equal
    /// Compares each element of this matrix to the given matrix
    /// @param other matrix to compare to
    /// @return true if the matrices are the same
    bool operator==(const Matrix2T & other) const { return m[0] == other.m[0] && m[1] == other.m[1]; }
    /// Compare if two matrices are not equal
    /// Compares each element of this matrix to the given matrix
    /// @param other matrix to compare to
    /// @return true if the matrices differ
    bool operator!=(const Matrix2T & other) const { return !(*this == other); }

    /// Returns the number of rows in this matrix type
    static int         rows() { return 2; }
    /// Returns the number of columns in this matrix type
    static int         columns() { return 2; }

    /// Returns a rotation matrix
    static Matrix2T makeRotation(T r) { T c = std::cos(r); T s = std::sin(r); return Matrix2T(c, -s, s, c); }
    /// Returns a scaling matrix
    static Matrix2T makeScale(T s)  { return Matrix2T<T>(s, 0, 0, s); }

    static const Matrix2T<T> IDENTITY;

  private:
    Vector2T<T> m[2];
  };


  template<typename T> const Matrix2T<T> Matrix2T<T>::IDENTITY(
    1, 0,
    0, 1);

  /// Creates a rotation matrix
  /// @param a rotation in radians
  template <class T>
  inline void Matrix2T<T>::rotate(T a)
  {
    T ca = std::cos(a);
    T sa = std::sin(a);
    m[0].make(ca, -sa);
    m[1].make(sa, ca);
  }

  /// Returns the inverse of the matrix
  template <class T>
  inline Matrix2T<T> Matrix2T<T>::inverse(bool * ok, T tolerance) const
  {
    Matrix2T<T> inv;

    // Code from WidMagic4

    T fDet = det();
    if (std::abs(fDet) > tolerance) {
      T fInvDet = ((T)1.0)/fDet;
      inv.data()[0] =  data()[3]*fInvDet;
      inv.data()[1] = -data()[1]*fInvDet;
      inv.data()[2] = -data()[2]*fInvDet;
      inv.data()[3] =  data()[0]*fInvDet;
      if(ok) *ok = true;
    } else {
      if(ok) *ok = false;
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

  /// Output the given matrix to a stream
  /// @param os stream to output to
  /// @param m matrix to output
  /// @return reference to the stream
  template <class T>
  inline std::ostream & operator<<(std::ostream & os, const Nimble::Matrix2T<T> & m)
  {
    os << m[0] << std::endl << m[1];
    return os;
  }

  /// Read a matrix from a stream
  /// @param is stream to read from
  /// @param m matrix to read to
  /// @return reference to the input stream
  template <class T>
  inline std::istream & operator>>(std::istream & is, Nimble::Matrix2T<T> & m)
  {
    is >> m[0] >> m[1];
    return is;
  }

  /// 2x2 matrix of floats
  typedef Matrix2T<float> Matrix2;
  /// 2x2 matrix of floats
  typedef Matrix2T<float> Matrix2f;
  /// 2x2 matrix of doubles
  typedef Matrix2T<double> Matrix2d;
}


#endif
