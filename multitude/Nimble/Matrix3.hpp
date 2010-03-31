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

#ifndef NIMBLE_MATRIX3T_HPP
#define NIMBLE_MATRIX3T_HPP

#include <Nimble/Export.hpp>
#include <Nimble/Matrix2.hpp>
#include <Nimble/Vector3.hpp>

#include <assert.h>

namespace Nimble {

  /// 3x3 transformation matrix
  /** This class is a row-major 3x3 matrix. The matrix functions
      (rotations etc.) assume right-handed coordinate system. */
  template <class T>
  class Matrix3T
  {
  public:
    /// Constructs the matrix without initializing any values.
    Matrix3T() {}
    template <class S>
    Matrix3T(const S * x)
    {
      m[0][0] = x[0]; m[0][1] = x[1]; m[0][2] = x[2];
      m[1][0] = x[3]; m[1][1] = x[4]; m[1][2] = x[5];
      m[2][0] = x[6]; m[2][1] = x[7]; m[2][2] = x[8];
    }
    Matrix3T(const Vector3T<T>& a, const Vector3T<T>& b, const Vector3T<T>& c) { m[0] = a; m[1] = b; m[2] = c; }
    Matrix3T(T v11, T v12, T v13, T v21, T v22, T v23, T v31, T v32, T v33)
    { m[0].make(v11, v12, v13); m[1].make(v21, v22, v23); m[2].make(v31, v32, v33); }
    /// Returns a reference to one row in the matrix
    Vector3T<T>&       row(int i)             { return m[i]; }
    /// Returns a constant reference to one row in the matrix
    const Vector3T<T>& row(int i) const       { return m[i]; }
    /// Returns one column of the matrix
    /** As the matrix is is of row-major type, this method returns a
    copy of the values of the column. */
    Vector3T<T>        column(int i) const    { return Vector3T<T>(m[0][i],m[1][i],m[2][i]); }
    Vector3T<T>&       operator[](int i)      { return row(i); }
    const Vector3T<T>& operator[](int i) const{ return row(i); }
    void               set(int r, int c, T v) { m[r][c] = v; }
    /// Gets one element from the matrix
    T &                get(int r, int c)       { return m[r][c]; }
    /// Gets one constant element from the matrix
    const T &          get(int r, int c) const { return m[r][c]; }
    /// Gets a pointer to the data
    T *                data()       { return m[0].data(); }
    /// Gets a constant pointer to the data
    const T *          data() const { return m[0].data(); }
    /// Copy argument values into this matrix
    void               make(T v11, T v12, T v13, T v21, T v22, T v23, T v31, T v32, T v33)
    { m[0].make(v11, v12, v13); m[1].make(v21, v22, v23); m[2].make(v31, v32, v33); }

    inline void               transpose();
    void                      clear() { m[0].clear(); m[1].clear(); m[2].clear(); }
    inline void               identity();
    /// Create a rotation matrix, around X axis
    inline void               rotateX(T a);
    /// Create a rotation matrix, around Y axis
    inline void               rotateY(T a);
    /// Create a rotation matrix, around Z axis
    inline void               rotateZ(T a);
    /// Assuming that this a rotation matrix, calculate rotation axis and angle
    inline void               getRotateAroundAxis(Vector3T<T>& axis, T & radians);
    /// Create a rotation axis, based on rotation axis and angle
    inline void               rotateAroundAxis(const Vector3T<T>& axis, T radians);
    /// Assuming that this a rotation matrix, calculate rotation around XYZ axis
    inline bool               getRotationXYZ (T & xa, T & ya, T & za);
    inline Matrix3T<T>&       operator*=(const Matrix3T<T>& that);
    inline bool               operator==(const Matrix3T<T>& that) const;
    inline bool               operator!=(const Matrix3T<T>& that) const;
    /// Run internal test function.
    inline static void        test();
    /// Returns the number of rows in the matrix (=3)
    /** This function can be used when you build template-based
    functions. */
    static int                rows() { return 3; }
    /// Returns the number of columns in the matrix (=3)
    /** This function can be used when you build template-based
    functions. */
    static int                columns() { return 3; }
    /// Inserts the argument matrix into the top-left corner of this matrix
    inline void               insert(const Matrix2T<T>& m);

    /// Get the 2x2 upper-left matrix
    inline Matrix2T<T> upperLeft() const {
      return Matrix2T<T>(get(0, 0), get(0, 1), get(1, 0), get(1, 1));
    }
    /** Calculates the inverse of this matrix.
      @param ok Returns the success value of the inversion operation
      @param tolerance if determinant smaller than tolerance, abort
      @return the inverted matrix */
    inline Matrix3T<T>        inverse(bool * ok = 0, T tolerance = 1.0e-8) const;

    /// Create a matrix that performs 2D translation
    static Matrix3T<T> translation(const Vector2T<T> & t) { Matrix3T<T> m; m.identity(); m.set(0, 2, t.x); m.set(1, 2, t.y); return m; }
    static Matrix3T<T> translation(const T & x, const T & y) { Matrix3T<T> m; m.identity(); m.set(0, 2, x); m.set(1, 2, y); return m; }
    /// Create a matrix that performs 2D translation
    inline static Matrix3T<T> translate2D(const Vector2T<T> & t);
    /// Create a matrix that performs 2D translation
    inline static Matrix3T<T> translate2D(const T & x, const T & y)
    { return translate2D(Vector2T<T>(x, y)); }
    /// Create a matrix that performs 2D scaling
    inline static Matrix3T<T> scale2D(const Vector2T<T> & s);
    inline static Matrix3T<T> scale2D(const T & xscale, const T & yscale);
    /// Create a matrix that performs 2D scaling
    inline static Matrix3T<T> scaleUniform2D(const T & s)
    { return scale2D(Vector2T<T>(s, s)); }
    NIMBLE_API static Matrix3T<T> scaleUniformAroundPoint2D(Vector2T<T> p,
                                                     T s);

    /// Create a matrix that performs 2D rotation
    inline static Matrix3T<T> rotate2D(T radians);

    /// Rotate around a given point
    /** @arg p The center point of rotation
        @arg radians The amount of roration, in radians
    */
    NIMBLE_API static Matrix3T<T> rotateAroundPoint2D(Vector2T<T> p,
                                           T radians);

    inline static Matrix3T<T> makeRotation(T radians, const Vector3T<T> & axis);

    /// Extract the scaling factor from a homogenous 2D transformation matrix
    inline T extractScale() const;

    inline Vector2T<T> project(const Vector2T<T> & v) const;
    inline Vector2T<T> project(const T & x, const T & y) const;

    /** Identity matrix. */
    NIMBLE_API static const Matrix3T<T> IDENTITY;

  private:
    inline static void swap(T &a, T& b);

    Vector3T<T> m[3];
  };

  template <class T>
  inline void Matrix3T<T>::test()
  {
    Matrix3T<T> a;
    int i,j;
    /* STORING & INDEXING ELEMENTS */
    for(i = 0; i < 3; i++)
      for(j = 0; j < 3; j++)
    a[i][j] = T(3*i+j);

    for(i = 0; i < 3; i++)
      for(j = 0; j < 3; j++)
    assert(a[i][j] == 3*i+j);

    /* CLEAR */
    a.clear();
    for(i = 0; i < 3; i++)
      for(j = 0; j < 3; j++)
    assert(a[i][j] == 0);

    /* ROW & COLUMN OPERATORS */
    for(i = 0; i < 3; i++)
      for(j = 0; j < 3; j++)
    a[i][j] = T(3*i+j);

    for(i = 0; i < 3; i++)
      for(j = 0; j < 3; j++)
    {
      assert(a.row(i)[j] == 3*i+j);
      assert(a.column(j)[i] == 3*i+j);
    }
    /* TRANSPOSE */
    a.transpose();
    for(i = 0; i < 3; i++)
      for(j = 0; j < 3; j++)
    assert(a[j][i] == 3*i+j);

    /* IDENTITY */
    a.identity();

    for(i = 0; i < 3; i++)
      for(j = 0; j < 3; j++)
    {
      if( i == j ) assert(a[i][j] == 1);
      else assert(a[i][j] == 0);
    }

    /* COPY OPERATOR, CONSTRUCTOR AND EQUALITY OPERATOR */

    for(i = 0; i < 3; i++)
      for(j = 0; j < 3; j++)
    a[i][j] = T(3*i+j);

    Matrix3T<T> b(a);
    assert(a == b);
    assert(!(a != b));

    /* MATRIX MULTIPLICATION */

    Matrix3T<T> c;
    c.identity();
    b *= c;
    assert(a == b);

    c.clear();
    b *= c;
    for(i = 0; i < 3; i++)
      for(j = 0; j < 3; j++)
    assert(b[i][j] == 0);
  }

  /// 3x3 matrix of floats
  typedef Matrix3T<float> Matrix3;
  /// 3x3 matrix of floats
  typedef Matrix3T<float> Matrix3f;

  template <class T>
  inline void Matrix3T<T>::swap(T &a, T& b)
  {
    T t = a;
    a = b;
    b = t;
  }

  template <class T>
  inline void Matrix3T<T>::transpose()
  {
    swap(m[0][1],m[1][0]);
    swap(m[0][2],m[2][0]);
    swap(m[1][2],m[2][1]);
  }

  template <class T>
  inline void Matrix3T<T>::identity()
  {
    for(int i = 0; i < 3; i++) m[i][i] = 1.0f;
    m[1][0] = m[2][0] = m[2][1] = m[0][1] = m[0][2] = m[1][2] = 0.0f;
  }

  template <class T>
  inline void Matrix3T<T>::rotateX(T a)
  {
    T ca = Math::Cos(a);
    T sa = Math::Sin(a);
    m[0].make(1.0, 0.0, 0.0);
    m[1].make(0.0, ca, -sa);
    m[2].make(0.0, sa, ca);
  }

  template <class T>
  inline void Matrix3T<T>::rotateY(T a)
  {
    T ca = Math::Cos(a);
    T sa = Math::Sin(a);
    m[0].make(ca,  0.0, sa);
    m[1].make(0.0, 1.0, 0.0);
    m[2].make(-sa, 0.0, ca);
  }


  template <class T>
  inline void Matrix3T<T>::rotateZ(T a)
  {
    T ca = Math::Cos(a);
    T sa = Math::Sin(a);
    m[0].make(ca, -sa, 0.0);
    m[1].make(sa, ca, 0.0);
    m[2].make(0.0, 0.0, 1.0);
  }

  template <class T>
  inline void Matrix3T<T>::getRotateAroundAxis(Vector3T<T>& axis, T & radians)
  {
    // Let (x,y,z) be the unit-length axis and let A be an angle of rotation.
    // The rotation matrix is R = I + sin(A)*P + (1-cos(A))*P^2 where
    // I is the identity and
    //
    //       +-        -+
    //   P = |  0 -z +y |
    //       | +z  0 -x |
    //       | -y +x  0 |
    //       +-        -+
    //
    // If A > 0, R represents a counterclockwise rotation about the axis in
    // the sense of looking from the tip of the axis vector towards the
    // origin.  Some algebra will show that
    //
    //   cos(A) = (trace(R)-1)/2  and  R - R^t = 2*sin(A)*P
    //
    // In the event that A = pi, R-R^t = 0 which prevents us from extracting
    // the axis through P.  Instead note that R = I+2*P^2 when A = pi, so
    // P^2 = (R-I)/2.  The diagonal entries of P^2 are x^2-1, y^2-1, and
    // z^2-1.  We can solve these for axis (x,y,z).  Because the angle is pi,
    // it does not matter which sign you choose on the square roots.

    T fTrace = data()[0] + data()[4] + data()[8];
    T fCos = ((T)0.5)*(fTrace-((T)1.0));
    radians = Math::ACos(fCos);  // in [0,PI]

    if ( radians > (T)0.0 ) {
      if ( radians < Math::PI ) {
    axis[0] = data()[7]-data()[5];
    axis[1] = data()[2]-data()[6];
    axis[2] = data()[3]-data()[1];
    axis.normalize();
      }
      else {
    // angle is PI
    T fHalfInverse;
    if ( data()[0] >= data()[4] ) {
      // r00 >= r11
      if ( data()[0] >= data()[8] ) {
        // r00 is maximum diagonal term
        axis[0] = ((T)0.5)*Math::Sqrt(data()[0] -
                      data()[4] - data()[8] + (T)1.0);
        fHalfInverse = ((T)0.5)/axis[0];
        axis[1] = fHalfInverse*data()[1];
        axis[2] = fHalfInverse*data()[2];
      }
      else {
        // r22 is maximum diagonal term
        axis[2] = ((T)0.5)*Math::Sqrt(data()[8] -
                      data()[0] - data()[4] + (T)1.0);
        fHalfInverse = ((T)0.5)/axis[2];
        axis[0] = fHalfInverse*data()[2];
        axis[1] = fHalfInverse*data()[5];
      }
    }
    else {
      // r11 > r00
      if ( data()[4] >= data()[8] ) {
        // r11 is maximum diagonal term
        axis[1] = ((T)0.5)*Math::Sqrt(data()[4] -
                      data()[0] - data()[8] + (T)1.0);
        fHalfInverse  = ((T)0.5)/axis[1];
        axis[0] = fHalfInverse*data()[1];
        axis[2] = fHalfInverse*data()[5];
      }
      else {
        // r22 is maximum diagonal term
        axis[2] = ((T)0.5)*Math::Sqrt(data()[8] -
                      data()[0] - data()[4] + (T)1.0);
        fHalfInverse = ((T)0.5)/axis[2];
        axis[0] = fHalfInverse*data()[2];
        axis[1] = fHalfInverse*data()[5];
      }
    }
      }
    }
    else {
      // The angle is 0 and the matrix is the identity.  Any axis will
      // work, so just use the x-axis.
      axis[0] = (T)1.0;
      axis[1] = (T)0.0;
      axis[2] = (T)0.0;
      radians = (T)0.0;
    }

  }

  template <class T>
  inline void Matrix3T<T>::rotateAroundAxis(const Vector3T<T>& axis, T radians)
  {
    T ca = Math::Cos(radians);
    T sa = Math::Sin(radians);
    // T fCos = cos(radians);
    // T sa = sin(radians);
    T fOneMinusCos = ((T) 1.0) -ca;
    T fX2 = axis.x*axis.x;
    T fY2 = axis.y*axis.y;
    T fZ2 = axis.z*axis.z;
    T fXYM = axis.x*axis.y*fOneMinusCos;
    T fXZM = axis.x*axis.z*fOneMinusCos;
    T fYZM = axis.y*axis.z*fOneMinusCos;
    T fXSin = axis.x*sa;
    T fYSin = axis.y*sa;
    T fZSin = axis.z*sa;

    m[0][0] = fX2*fOneMinusCos+ca;
    m[0][1] = fXYM-fZSin;
    m[0][2] = fXZM+fYSin;
    m[1][0] = fXYM+fZSin;
    m[1][1] = fY2*fOneMinusCos+ca;
    m[1][2] = fYZM-fXSin;
    m[2][0] = fXZM-fYSin;
    m[2][1] = fYZM+fXSin;
    m[2][2] = fZ2*fOneMinusCos+ca;
  }

  template <class T>
  bool Matrix3T<T>::getRotationXYZ (T & xa, T & ya, T & za)
  {
    // rot =  cy*cz          -cy*sz           sy
    //        cz*sx*sy+cx*sz  cx*cz-sx*sy*sz -cy*sx
    //       -cx*cz*sy+sx*sz  cz*sx+cx*sy*sz  cx*cy

    if ( m[0][2] < 1.0f )
      {
    if ( m[0][2] > -1.0f )
      {
        xa = Math::ATan2(-m[1][2],m[2][2]);
        ya = (T)asin(m[0][2]);
        za = Math::ATan2(-m[0][1],m[0][0]);
        return true;
      }
        else
      {
            // WARNING.  Not unique.  XA - ZA = -atan2(r10,r11)
            xa = -Math::ATan2(m[1][0],m[1][1]);
            ya = -(T)Math::HALF_PI;
            za = 0.0f;
            return false;
      }
      }
    else
      {
    // WARNING.  Not unique.  XAngle + ZAngle = atan2(r10,r11)
    xa = Math::ATan2(m[1][0],m[1][1]);
    ya = (T)Math::HALF_PI;
    za = 0.0f;
    return false;
      }

  }

  /** this = this * that. */
  template <class T>
  inline Matrix3T<T>& Matrix3T<T>::operator*= (const Matrix3T<T>& that)
  {
    *this = *this * that;
    return *this;
  }

  template <class T>
  inline bool Matrix3T<T>::operator==(const Matrix3T<T>& that) const
  {
    return (m[0] == that.m[0] &&  m[1] == that.m[1] &&  m[2] == that.m[2]);
  }

  template <class T>
  inline bool Matrix3T<T>::operator!=(const Matrix3T<T>& that) const
  {
    return !(*this == that);
  }

  template <class T>
  Matrix3T<T> Matrix3T<T>::inverse(bool * ok, T tolerance) const
  {

    Matrix3T<T> res;

    // Invert a 3x3 using cofactors.  This is about 8 times faster than
    // the Numerical Recipes code which uses Gaussian elimination.

    // Code from Wild Magic library.

    res[0][0] = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    res[0][1] = m[0][2] * m[2][1] - m[0][1] * m[2][2];
    res[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    res[1][0] = m[1][2] * m[2][0] - m[1][0] * m[2][2];
    res[1][1] = m[0][0] * m[2][2] - m[0][2] * m[2][0];
    res[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];
    res[2][0] = m[1][0] * m[2][1] - m[1][1] * m[2][0];
    res[2][1] = m[0][1] * m[2][0] - m[0][0] * m[2][1];
    res[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];

    T fDet = m[0][0] * res[0][0] + m[0][1] * res[1][0] + m[0][2] * res[2][0];

    if(Math::Abs(fDet) <= tolerance ) {
      if(ok)
    *ok = false;
      return res;
    }
    else if(ok)
      *ok = true;

    T fInvDet = 1.0f / fDet;
    for (int iRow = 0; iRow < 3; iRow++) {
      for (int iCol = 0; iCol < 3; iCol++)
    res[iRow][iCol] *= fInvDet;
    }
    return res;
  }

  /// Multiply two matrices together
template <class T>
inline Nimble::Matrix3T<T> operator * (const Nimble::Matrix3T<T>& m1,
                     const Nimble::Matrix3T<T>& m2)
{
  Nimble::Matrix3T<T> res;

  for(int i = 0; i < 3; i++) {
    Nimble::Vector3T<T> t = m2.column(i);
    res[0][i] = ::dot(m1.row(0),t);
    res[1][i] = ::dot(m1.row(1),t);
    res[2][i] = ::dot(m1.row(2),t);
  }

  return res;
}

/// Multiply a matrix and a vector
template <class S, class T>
inline Nimble::Vector3T<T> operator*(const Nimble::Matrix3T<S>& m1,
                   const Nimble::Vector3T<T>& m2)
{
  Nimble::Vector3T<T> res;
  for(int i = 0; i < 3; i++)
    res[i] = ::dot(m1.row(i),m2);
  return res;
}

/// Multiply a matrix with a vector by implicitly adding one as the third component of the vector
template <class T>
inline Nimble::Vector3T<T> operator*(const Nimble::Matrix3T<T>& m1,
                   const Nimble::Vector2T<T>& m2)
{
  Nimble::Vector3T<T> res;
  for(int i = 0; i < 3; i++)
    res[i] = ::dot3(m1.row(i),m2);
  return res;
}


/// Multiply a matrix and a vector
template <class T>
inline Nimble::Vector3T<T> operator*(const Nimble::Vector3T<T>& m2,
                   const Nimble::Matrix3T<T>& m1)
{
  Nimble::Vector3T<T> res;
  for(int i = 0; i < 3; i++)
    res[i] = ::dot(m1.column(i),m2);
  return res;
}

/// Insert a 2x2 matrix to the upper-left corner of the 3x3 matrix
template<class T>
inline void Matrix3T<T>::insert(const Matrix2T<T>& b)
{
  m[0].x = b.get(0, 0);
  m[0].y = b.get(0, 1);

  m[1].x = b.get(1, 0);
  m[1].y = b.get(1, 1);
}

template<class T>
inline Matrix3T<T> Matrix3T<T>::translate2D(const Vector2T<T> & t)
{
  Matrix3T<T> m;
  m.identity();

  m.set(0, 2, t.x);
  m.set(1, 2, t.y);

  return m;
}

template<class T>
inline Matrix3T<T> Matrix3T<T>::scale2D(const Vector2T<T> & s)
{
  Matrix3T<T> m;
  m.identity();

  m.set(0, 0, s.x);
  m.set(1, 1, s.y);

  return m;
}

template<class T>
inline Matrix3T<T> Matrix3T<T>::scale2D(const T & xscale, const T & yscale)
{
  Matrix3T<T> m;
  m.identity();

  m.set(0, 0, xscale);
  m.set(1, 1, yscale);

  return m;
}


template<class T>
inline Matrix3T<T> Matrix3T<T>::rotate2D(T radians)
{
  Matrix3T<T> m;
  m.rotateZ(radians);

  return m;
}

template<class T>
T Matrix3T<T>::extractScale() const
{
  Vector3T<T> u(T(1), T(0), T(0));
  Vector3T<T> v = *this * u;
  T s = Math::Sqrt(v.x * v.x + v.y * v.y);

  return s;
}

template<class T>
Matrix3T<T> Matrix3T<T>::makeRotation(T radians, const Vector3T<T> & axis)
{
  T c = T(cos(radians));
  T t = T(1) - c;
  T s = T(sin(radians));

  Vector3T<T> vn(axis);
  vn.normalize();

  Matrix3T<T> m;

  T aa[9];
  aa[0] = t * vn.x * vn.x + c;
  aa[1] = t * vn.x * vn.y - s * vn.z;
  aa[2] = t * vn.x * vn.z + s * vn.y;

  aa[3] = t * vn.x * vn.y + s * vn.z;
  aa[4] = t * vn.y * vn.y + c;
  aa[5] = t * vn.y * vn.z - s * vn.x;

  aa[6] = t * vn.x * vn.z - s * vn.y;
  aa[7] = t * vn.y * vn.z + s * vn.x;
  aa[8] = t * vn.z * vn.z + c;

  return Matrix3T<T>(aa);
}


template<class T>
inline Vector2T<T> Matrix3T<T>::project(const Vector2T<T> & v) const
{
  Vector3T<T> p = *this * v;
  return Vector2T<T>(p.x / p.z, p.y / p.z);
}

template<class T>
inline Vector2T<T> Matrix3T<T>::project(const T & x, const T & y) const
{
  Vector3T<T> p = *this * Vector2T<T>(x, y);
  return Vector2T<T>(p.x / p.z, p.y / p.z);
}

} // namespace

template <class T>
inline std::ostream& operator<<(std::ostream& os, const Nimble::Matrix3T<T>& m)
{
  os << m[0] << ", " << m[1] << ", " << m[2];
  return os;
}

using Nimble::operator *;
using Nimble::operator /;
using Nimble::operator <<;
using Nimble::operator >>;

#endif
