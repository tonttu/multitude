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

#ifndef NIMBLE_MATRIX4T_HPP
#define NIMBLE_MATRIX4T_HPP

#include <Nimble/Export.hpp>
#include <Nimble/Matrix3.hpp>
#include <Nimble/Vector4.hpp>

namespace Nimble {

  /// 4x4 transformation matrix
  /** This class is a row-major 4x4 matrix. The matrix functions
      (rotations etc.) assume right-handed coordinate system. */
  template <class T>
  class Matrix4T
  {
  public:
    template <class S>
    Matrix4T(const S * x) { const S * end = x + 16; T * my = data(); while(x!=end) *my++ = *x++; }

    Matrix4T() {}
    Matrix4T(T x11, T x12, T x13, T x14,
         T x21, T x22, T x23, T x24,
         T x31, T x32, T x33, T x34,
         T x41, T x42, T x43, T x44)
    {
      m[0].make(x11, x12, x13, x14);
      m[1].make(x21, x22, x23, x24);
      m[2].make(x31, x32, x33, x34);
      m[3].make(x41, x42, x43, x44);
    }
    Matrix4T(const Vector4T<T>& a, const Vector4T<T>& b, const Vector4T<T>& c, const Vector4T<T>& d)
    { m[0] = a; m[1] = b; m[2] = c; m[3] = d; }
    Vector4T<T>&       row(int i)             { return m[i]; }
    const Vector4T<T>& row(int i) const       { return m[i]; }

    Vector4T<T>        column(int i) const    { return Vector4T<T>(m[0][i],m[1][i],m[2][i],m[3][i]); }
    void               setColumn(int i, const Vector4T<T> &v) { m[0][i] = v[0]; m[1][i] = v[1]; m[2][i] = v[2]; m[3][i] = v[3]; }
    void               setColumn3(int i, const Vector3T<T> &v) { m[0][i] = v[0]; m[1][i] = v[1]; m[2][i] = v[2]; m[3][i] = 1.0; }
    void               setColumn3b(int i, const Vector3T<T> &v) { m[0][i] = v[0]; m[1][i] = v[1]; m[2][i] = v[2]; }
    void               addToColumn(int i, const Vector4T<T> &v) { m[0][i] += v[0]; m[1][i] += v[1]; m[2][i] += v[2]; m[3][i] += v[3]; }
    void               addToColumn(int i, const Vector3T<T> &v) { m[0][i] += v[0]; m[1][i] += v[1]; m[2][i] += v[2]; }

    void               setDiagonal(const Vector4T<T> &v) { m[0][0] = v[0]; m[1][1] = v[1]; m[2][2] = v[2]; m[3][3] += v[3]; }
    void               setDiagonal(const Vector3T<T> &v) { m[0][0] = v[0]; m[1][1] = v[1]; m[2][2] = v[2]; m[3][3] = (T) 1.0; }

    Vector4T<T>&       operator[](int i)      { return row(i); }
    const Vector4T<T>& operator[](int i) const{ return row(i); }
    inline void               setRotation(const Nimble::Matrix3T<T>& that);
    inline Matrix3T<T>        getRotation() const;
    void                      setTranslation(const Vector3T<T> & v);
    Vector3T<T>               getTranslation() const;

    inline Matrix4T<T>&       transpose();
    void                      clear()         { m[0].clear(); m[1].clear(); m[2].clear(); m[3].clear(); }
    inline void               identity();
    inline void               scalingMatrix(const Vector3T<T> &);
    void                      make(T x11, T x12, T x13, T x14,
                T x21, T x22, T x23, T x24,
                T x31, T x32, T x33, T x34,
                T x41, T x42, T x43, T x44)
      {
    m[0].make(x11, x12, x13, x14);
    m[1].make(x21, x22, x23, x24);
    m[2].make(x31, x32, x33, x34);
    m[3].make(x41, x42, x43, x44);
      }
    inline Matrix4T<T>        inverse(bool * ok) const;

    inline Matrix4T<T>&       operator*=(const Matrix4T<T>& that);
    Matrix4T<T>&              operator *= (T s) { T * p = data(); for(unsigned i=0; i < 16; i++) p[i] *= s; return * this; }
    inline bool               operator==(const Matrix4T<T>& that) const;
    inline bool               operator!=(const Matrix4T<T>& that) const;

    static int                rows() { return 4; }
    static int                columns() { return 4; }
    NIMBLE_API static void    test();

    T *       data()       { return m[0].data(); }
    const T * data() const { return m[0].data(); }

    template <class S>
    void copy (const S * x) { const S * end = x + 16; T * my = data(); while(x!=end) *my++ = (T) *x++; }
    template <class S>
    void copyTranspose (const S * x) { for(int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) m[j][i] = (T) x[i*4+j]; }

    /// @todo duplicates (makeTranslation vs. translate3D)
    static Matrix4T<T> makeRotation(T radians, const Vector3T<T> & axis);
    static Matrix4T<T> makeTranslation(const Vector3T<T> & v);
    NIMBLE_API static Matrix4T<T> translate3D(const Vector3T<T> & v);
    NIMBLE_API static Matrix4T<T> scale3D(const Vector3T<T> & v);

    /** Identity matrix. */
    NIMBLE_API static const Matrix4T<T> IDENTITY;

  private:
    inline static void swap(T &a, T& b);

    Vector4T<T> m[4];
  };

  template <class T>
  inline void Matrix4T<T>::swap(T &a, T& b)
  {
    T t = a;
    a = b;
    b = t;
  }

  template <class T>
  inline void Matrix4T<T>::setRotation(const Matrix3T<T>& that)
  {
    for(int i = 0; i < 3; i++)
      for(int j = 0; j < 3; j++)
    m[i][j] = that[i][j];
  }

  template <class T>
  inline Matrix3T<T> Matrix4T<T>::getRotation() const
  {
    Matrix3T<T> res;
    for(int i = 0; i < 3; i++)
      for(int j = 0; j < 3; j++)
    res[i][j] = m[i][j];
    return res;
  }

  template <class T>
  inline Matrix4T<T>& Matrix4T<T>::transpose()
  {
    swap(m[0][1],m[1][0]);
    swap(m[0][2],m[2][0]);
    swap(m[0][3],m[3][0]);
    swap(m[1][2],m[2][1]);
    swap(m[1][3],m[3][1]);
    swap(m[2][3],m[3][2]);
    return *this;
  }

  template <class T>
  inline void Matrix4T<T>::identity()
  {
    for(int i = 0; i < rows(); i++)
      for(int j = 0; j < columns(); j++)
    m[i][j] = (i == j) ? 1.0f : 0.0f;
  }

  template <class T>
  inline void Matrix4T<T>::scalingMatrix(const Vector3T<T> &s)
  {
    m[0].make(s[0], 0,    0,    0);
    m[1].make(0,    s[1], 0,    0);
    m[2].make(0,    0,    s[2], 0);
    m[3].make(0,    0,    0,    1);
  }

  /** Inverts the matrix. The boolean argument is set to true or false
      depending on how well the operation went. */

  template <class T>
  Matrix4T<T> Matrix4T<T>::inverse(bool * ok) const
  {
    const T * my = data();
    T fA0 = my[ 0]*my[ 5] - my[ 1]*my[ 4];
    T fA1 = my[ 0]*my[ 6] - my[ 2]*my[ 4];
    T fA2 = my[ 0]*my[ 7] - my[ 3]*my[ 4];
    T fA3 = my[ 1]*my[ 6] - my[ 2]*my[ 5];
    T fA4 = my[ 1]*my[ 7] - my[ 3]*my[ 5];
    T fA5 = my[ 2]*my[ 7] - my[ 3]*my[ 6];
    T fB0 = my[ 8]*my[13] - my[ 9]*my[12];
    T fB1 = my[ 8]*my[14] - my[10]*my[12];
    T fB2 = my[ 8]*my[15] - my[11]*my[12];
    T fB3 = my[ 9]*my[14] - my[10]*my[13];
    T fB4 = my[ 9]*my[15] - my[11]*my[13];
    T fB5 = my[10]*my[15] - my[11]*my[14];

    T fDet = fA0*fB5-fA1*fB4+fA2*fB3+fA3*fB2-fA4*fB1+fA5*fB0;
    if ( Math::Abs(fDet) <= 1.0e-10 ) {
      *ok = false;
      Matrix4T<T> tmp;
      tmp.identity();
      return tmp;
    }

    *ok = true;

    Matrix4T<T> inv;
    inv[0][0] = + my[ 5]*fB5 - my[ 6]*fB4 + my[ 7]*fB3;
    inv[1][0] = - my[ 4]*fB5 + my[ 6]*fB2 - my[ 7]*fB1;
    inv[2][0] = + my[ 4]*fB4 - my[ 5]*fB2 + my[ 7]*fB0;
    inv[3][0] = - my[ 4]*fB3 + my[ 5]*fB1 - my[ 6]*fB0;
    inv[0][1] = - my[ 1]*fB5 + my[ 2]*fB4 - my[ 3]*fB3;
    inv[1][1] = + my[ 0]*fB5 - my[ 2]*fB2 + my[ 3]*fB1;
    inv[2][1] = - my[ 0]*fB4 + my[ 1]*fB2 - my[ 3]*fB0;
    inv[3][1] = + my[ 0]*fB3 - my[ 1]*fB1 + my[ 2]*fB0;
    inv[0][2] = + my[13]*fA5 - my[14]*fA4 + my[15]*fA3;
    inv[1][2] = - my[12]*fA5 + my[14]*fA2 - my[15]*fA1;
    inv[2][2] = + my[12]*fA4 - my[13]*fA2 + my[15]*fA0;
    inv[3][2] = - my[12]*fA3 + my[13]*fA1 - my[14]*fA0;
    inv[0][3] = - my[ 9]*fA5 + my[10]*fA4 - my[11]*fA3;
    inv[1][3] = + my[ 8]*fA5 - my[10]*fA2 + my[11]*fA1;
    inv[2][3] = - my[ 8]*fA4 + my[ 9]*fA2 - my[11]*fA0;
    inv[3][3] = + my[ 8]*fA3 - my[ 9]*fA1 + my[10]*fA0;

    T fInvDet = ((T)1.0)/fDet;
    for (int iRow = 0; iRow < 4; iRow++)
      for (int iCol = 0; iCol < 4; iCol++)
    inv[iRow][iCol] *= fInvDet;

    return inv;
  }

  template <class T>
  inline Matrix4T<T>& Matrix4T<T>::operator *= (const Matrix4T<T>& that)
  {
    for(int i = 0; i < columns(); i++)
      {
    const Vector4T<T> t = column(i);
    m[0][i] = ::dot(that[0],t);
    m[1][i] = ::dot(that[1],t);
    m[2][i] = ::dot(that[2],t);
    m[3][i] = ::dot(that[3],t);
      }
    return *this;
  }

  /// 4x4 matrix of floats
  typedef Matrix4T<float> Matrix4;
  /// 4x4 matrix of floats
  typedef Matrix4T<float> Matrix4f;
  /// 4x4 matrix of doubles
  typedef Matrix4T<double> Matrix4d;

}

template <class T>
inline bool Nimble::Matrix4T<T>::operator==(const Nimble::Matrix4T<T>& that) const
{
  return m[0] == that.m[0] && m[1] == that.m[1] && m[2] == that.m[2] && m[3] == that.m[3];
}

template <class T>
inline bool Nimble::Matrix4T<T>::operator!=(const Nimble::Matrix4T<T>& that) const
{
  return !(*this == that);
}

template <class T>
inline Nimble::Matrix4T<T> operator*(const Nimble::Matrix4T<T>& m1,const Nimble::Matrix4T<T>& m2)
{
  Nimble::Matrix4T<T> res;
  for(int j = 0; j < 4; j++) {
    Nimble::Vector4T<T> t = m2.column(j);
    for(int i = 0; i < 4; i++)
      res[i][j] = dot(m1.row(i),t);
  }
  return res;
}

template <class T>
inline Nimble::Vector4T<T> operator*(const Nimble::Matrix4T<T>& m1,const Nimble::Vector4T<T>& m2)
{
  Nimble::Vector4T<T> res;
  for(int i = 0; i < 4; i++)
    res[i] = dot(m1.row(i),m2);
  return res;
}


template <class T>
inline Nimble::Vector3T<T> operator*(const Nimble::Matrix4T<T>& m1,const Nimble::Vector3T<T>& m2)
{
  Nimble::Vector3T<T> res;
  for(int i = 0; i < 3; i++)
    res[i] = dot4(m1.row(i),m2);
  return res;
}

/// @todo Vector4 * Matrix4 is not defined. This implicitly transposes the vector. This should not 
/// operator should not be defined. Also check other Matrix classes.
template <class T>
inline Nimble::Vector4T<T> operator*(const Nimble::Vector4T<T>& m2, const Nimble::Matrix4T<T>& m1)
{
  Nimble::Vector4T<T> res;
  for(int i = 0; i < 4; i++)
    res[i] = dot(m1.column(i),m2);
  return res;
}

template <class T>
inline Nimble::Vector3T<T> operator*(const Nimble::Vector3T<T>& m2, const Nimble::Matrix4T<T>& m1)
{
  Nimble::Vector3T<T> res;
  for(int i = 0; i < 3; i++)
    res[i] = dot4(m1.column(i),m2);
  return res;
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const Nimble::Matrix4T<T>& m)
{
  os << m[0] << ", " << m[1] << ", " << m[2] << ", " << m[3];
  return os;
}

template<class T>
void Nimble::Matrix4T<T>::setTranslation(const Nimble::Vector3T<T> & v)
{
  m[3][0] = v.x;
  m[3][1] = v.y;
  m[3][2] = v.z;
}

template<class T>
Nimble::Vector3T<T> Nimble::Matrix4T<T>::getTranslation() const
{
  return Nimble::Vector3T<T>(m[3][0], m[3][1], m[3][2]);
}

template<class T>
Nimble::Matrix4T<T> Nimble::Matrix4T<T>::makeRotation(T radians, const Nimble::Vector3T<T> & axis)
{
  Nimble::Matrix4T<T> mm;
  mm.identity();

  mm.setRotation(Nimble::Matrix3T<T>::makeRotation(radians, axis));
  return mm;
}

template<class T>
Nimble::Matrix4T<T> Nimble::Matrix4T<T>::makeTranslation(const Nimble::Vector3T<T> & v)
{
  Nimble::Matrix4T<T> mm;
  mm.identity();

  mm.setTranslation(v);
  return mm;
}

#endif

