/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_MATRIX3T_HPP
#define NIMBLE_MATRIX3T_HPP

#include "Export.hpp"
#include "Matrix2.hpp"
#include "Vector3.hpp"

#include <array>
#include <cassert>

namespace Nimble {

  /// 3x3 transformation matrix
  /** This class is a row-major 3x3 matrix. The matrix functions
      (rotations etc.) assume right-handed coordinate system. */
  template <class T>
  class Matrix3T
  {
  public:
    /// Data-type of the matrix
    typedef T type;

    /// Constructs the matrix without initializing any values.
    Matrix3T() = default;
    /// Constructs a matrix and initializes it from memory
    template <class K>
    Matrix3T(const K * x)
    {
      m[0][0] = x[0]; m[0][1] = x[1]; m[0][2] = x[2];
      m[1][0] = x[3]; m[1][1] = x[4]; m[1][2] = x[5];
      m[2][0] = x[6]; m[2][1] = x[7]; m[2][2] = x[8];
    }
    /// Constructs a matrix and initializes it with the given rows
    Matrix3T(const Vector3T<T>& a, const Vector3T<T>& b, const Vector3T<T>& c) { m[0] = a; m[1] = b; m[2] = c; }
    /// Constructs a matrix and initializes it with the given values
    Matrix3T(T v11, T v12, T v13, T v21, T v22, T v23, T v31, T v32, T v33)
    { m[0].make(v11, v12, v13); m[1].make(v21, v22, v23); m[2].make(v31, v32, v33); }
    /// Returns a reference to one row in the matrix
    Vector3T<T>&       row(int i)             { return m[i]; }
    /// Returns a constant reference to one row in the matrix
    const Vector3T<T>& row(int i) const       { return m[i]; }
    /// Returns one column of the matrix
    /// As the matrix is is of row-major type, this method returns a
    /// copy of the values of the column.
    /// @param i column number
    /// @return Copy of column i as a vector
    Vector3T<T>        column(int i) const    { return Vector3T<T>(m[0][i],m[1][i],m[2][i]); }
    /// Returns the ith row
    Vector3T<T>&       operator[](int i)      { return row(i); }
    /// Returns the ith row
    const Vector3T<T>& operator[](int i) const{ return row(i); }
    /// Set the value of the given element
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

    /// Transposes the matrix
    inline void               transpose();
    /// Returns a transposed matrix
    inline Matrix3T transposed() const;
    /// Fills the matrix with zeroes
    void                      clear() { m[0].clear(); m[1].clear(); m[2].clear(); }
    /// Makes the matrix an identity matrix
    inline void               identity();
    /// Create a rotation matrix, around X axis
    inline void               rotateX(T a);
    /// Create a rotation matrix, around Y axis
    inline void               rotateY(T a);
    /// Create a rotation matrix, around Z axis
    inline void               rotateZ(T a);
    /// Assuming that this a rotation matrix, calculate rotation axis and angle
    inline void               getRotateAroundAxis(Vector3T<T>& axis, T & radians) const;
    /// Create a rotation axis, based on rotation axis and angle
    inline void               rotateAroundAxis(const Vector3T<T>& axis, T radians);
    /// Assuming that this a rotation matrix, calculate rotation around XYZ axis
    inline bool               getRotationXYZ (T & xa, T & ya, T & za);
    /// Multiplies two matrices together
    inline Matrix3T<T>&       operator*=(const Matrix3T<T>& that);
    /// Compares if two matrices are equal
    inline bool               operator==(const Matrix3T<T>& that) const;
    /// Compares if two matrices are different
    inline bool               operator!=(const Matrix3T<T>& that) const;

    /// Returns the number of rows in the matrix
    /// This function can be used when you build template-based functions.
    /// @return 3
    static int                rows() { return 3; }
    /// Returns the number of columns in the matrix
    /// This function can be used when you build template-based functions.
    /// @return 3
    static int                columns() { return 3; }
    /// Inserts the argument matrix into the top-left corner of this matrix
    inline void               insert(const Matrix2T<T>& m);

    /// Get the 2x2 upper-left matrix
    inline Matrix2T<T> upperLeft() const {
      return Matrix2T<T>(get(0, 0), get(0, 1), get(1, 0), get(1, 1));
    }

    /// Calculates the inverse of this matrix.
    /// @param ok Returns the success value of the inversion operation
    /// @param tolerance if determinant smaller than tolerance, abort
    /// @return the inverted matrix
    inline Matrix3T<T>        inverse(bool * ok = 0, T tolerance = 1.0e-8) const;

    /// Calculate the inverse of this matrix. This function is an optimized
    /// version of inverse() that assumes the bottom row of the matrix is 0 0
    /// 1.
    /// @return inverted matrix
    inline Matrix3T<T>        inverse23() const;

    /// Create a matrix that performs 2D translation
    inline static Matrix3T<T> makeTranslation(const Vector2T<T> & v)
    { return makeTranslation(v.x, v.y); }
    /// Create a matrix that performs 2D translation
    inline static Matrix3T<T> makeTranslation(const T & x, const T & y);
    /// Create a matrix that performs 2D scaling
    inline static Matrix3T<T> makeScale(const Vector2T<T> & v)
    { return makeScale(v.x, v.y); }
    /// Create a matrix that performs 2D scaling
    inline static Matrix3T<T> makeScale(const T & xscale, const T & yscale);
    /// Create a matrix that performs 2D scaling
    inline static Matrix3T<T> makeUniformScale(const T & s)
    { return makeScale(s,s); }
    /// Create a matrix that performs uniform scaling around the given point
    static Matrix3T<T> makeUniformScaleAroundPoint(Vector2T<T> p,
                                                     T s)
    {
        return makeTranslation(p) * makeUniformScale(s) * makeTranslation(-p);
    }

    /// Create a matrix that performs uniform scaling around the given point
    static Matrix3T<T> makeScaleAroundPoint(Vector2T<T> p,
                                                     const T & xscale, const T & yscale)
    {
      return makeTranslation(p) * makeScale(xscale, yscale) * makeTranslation(-p);
    }

    /// Create a matrix that performs 2D rotation
    inline static Matrix3T<T> makeRotation(T radians);

    /// Rotate around a given point
    /** @param p The center point of rotation
        @param radians The amount of roration, in radians
        @return New rotation matrix
    */
    static Matrix3T<T> makeRotationAroundPoint(Vector2T<T> p,
                                           T radians)
    {
        return makeTranslation(p) * makeRotation(radians) * makeTranslation(-p);
    }

    /// Create a rotation matrix
    inline static Matrix3T<T> makeRotation(T radians, const Vector3T<T> & axis);

    /// Multiply the given point with the matrix and perform the homogenous divide
    inline Vector2T<T> project(const Vector2T<T> & v) const;
    /// Multiply the given point with the matrix and perform the homogenous divide
    inline Vector2T<T> project(const T & x, const T & y) const;

    /** Identity matrix. */
    static const Matrix3T<T> IDENTITY;

    /// Returns a 2d transformation matrix that does scale, rotate & translation (in this order)
    /// @param rad rotation angle (counter-clockwise)
    /// @param sx x scale
    /// @param sy y scale
    /// @param tx x translate
    /// @param ty y translate
    /// @return New transformation matrix
    inline static Matrix3T<T> makeTransformation(float rad, float sx, float sy, float tx, float ty)
    {
      const T st = rad == 0.0f ? 0.0f : std::sin(rad);
      const T ct = rad == 0.0f ? 1.0f : std::cos(rad);

      return Matrix3T<T>(
          sx*ct, -sy*st, tx,
          sx*st,  sy*ct, ty,
          0    ,      0,  1
          );
    }

    /// Create a projection matrix which maps the unit square to given vertices
    static Nimble::Matrix3T<T> makeProjectionMatrix(const std::array<Nimble::Vector2T<T>, 4> & vertices)
    {
      float dx1 = vertices[1].x - vertices[2].x;
      float dx2 = vertices[3].x - vertices[2].x;
      float dy1 = vertices[1].y - vertices[2].y;
      float dy2 = vertices[3].y - vertices[2].y;

      float sx = vertices[0].x - vertices[1].x +
                 vertices[2].x - vertices[3].x;

      float sy = vertices[0].y - vertices[1].y +
                 vertices[2].y - vertices[3].y;

      float del = Nimble::Math::Det(dx1, dx2, dy1, dy2);

      float g = Nimble::Math::Det(sx, dx2, sy, dy2) / del;
      float h = Nimble::Math::Det(dx1, sx, dy1, sy) / del;

      float a = vertices[1].x - vertices[0].x + g * vertices[1].x;
      float b = vertices[3].x - vertices[0].x + h * vertices[3].x;
      float c = vertices[0].x;

      float d = vertices[1].y - vertices[0].y + g * vertices[1].y;
      float e = vertices[3].y - vertices[0].y + h * vertices[3].y;
      float f = vertices[0].y;

      return Matrix3T<T>(a, b, c,
                     d, e, f,
                     g, h, 1);
    }

    /// Create a projective matrix which maps from[i] to to[i] for i=0..3
    /// @param from The four source points
    /// @param to The four targets points
    /// @param ok if defined; set to true if the projection could be created
    /// @return projection matrix
    static Nimble::Matrix3T<T> mapCorrespondingPoints(const std::array<Nimble::Vector2T<T>, 4> & from,
                                                  const std::array<Nimble::Vector2T<T>, 4> & to,
                                                  bool * ok = 0)
    {
      return  makeProjectionMatrix(to) * makeProjectionMatrix(from).inverse(ok);
    }


  private:
    Vector3T<T> m[3];
  };

  template<typename T> const Matrix3T<T> Matrix3T<T>::IDENTITY(
    1, 0, 0,
    0, 1, 0,
    0, 0, 1);

  /// 3x3 matrix of floats
  typedef Matrix3T<float> Matrix3;
  /// 3x3 matrix of floats
  typedef Matrix3T<float> Matrix3f;
  /// 3x3 matrix of doubles
  typedef Matrix3T<double> Matrix3d;

  template <class T>
  inline void Matrix3T<T>::transpose()
  {
    std::swap(m[0][1],m[1][0]);
    std::swap(m[0][2],m[2][0]);
    std::swap(m[1][2],m[2][1]);
  }

  template <class T>
  inline Matrix3T<T> Matrix3T<T>::transposed() const
  {
    Matrix3T<T> r;

    for(int i = 0; i < 3; i++) {
      for(int j = 0; j < 3; j++) {
        r[i][j] = m[j][i];
      }
    }

    return r;
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
    T ca = std::cos(a);
    T sa = std::sin(a);
    m[0].make(1.0, 0.0, 0.0);
    m[1].make(0.0, ca, -sa);
    m[2].make(0.0, sa, ca);
  }

  template <class T>
  inline void Matrix3T<T>::rotateY(T a)
  {
    T ca = std::cos(a);
    T sa = std::sin(a);
    m[0].make(ca,  0.0, sa);
    m[1].make(0.0, 1.0, 0.0);
    m[2].make(-sa, 0.0, ca);
  }


  template <class T>
  inline void Matrix3T<T>::rotateZ(T a)
  {
    T ca = std::cos(a);
    T sa = std::sin(a);
    m[0].make(ca, -sa, 0.0);
    m[1].make(sa, ca, 0.0);
    m[2].make(0.0, 0.0, 1.0);
  }

  template <class T>
  inline void Matrix3T<T>::getRotateAroundAxis(Vector3T<T>& axis, T & radians) const
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
    radians = std::acos(fCos);  // in [0,PI]

    if ( radians > (T)0.0 ) {
      if ( radians < Nimble::Math::PI ) {
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
        axis[0] = ((T)0.5)*std::sqrt(data()[0] -
                      data()[4] - data()[8] + (T)1.0);
        fHalfInverse = ((T)0.5)/axis[0];
        axis[1] = fHalfInverse*data()[1];
        axis[2] = fHalfInverse*data()[2];
      }
      else {
        // r22 is maximum diagonal term
        axis[2] = ((T)0.5)*std::sqrt(data()[8] -
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
        axis[1] = ((T)0.5)*std::sqrt(data()[4] -
                      data()[0] - data()[8] + (T)1.0);
        fHalfInverse  = ((T)0.5)/axis[1];
        axis[0] = fHalfInverse*data()[1];
        axis[2] = fHalfInverse*data()[5];
      }
      else {
        // r22 is maximum diagonal term
        axis[2] = ((T)0.5)*std::sqrt(data()[8] -
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
    T ca = std::cos(radians);
    T sa = std::sin(radians);
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
        xa = std::atan2(-m[1][2],m[2][2]);
        ya = (T)asin(m[0][2]);
        za = std::atan2(-m[0][1],m[0][0]);
        return true;
      }
      else
      {
        // Not unique.  XA - ZA = -atan2(r10,r11)
        xa = -std::atan2(m[1][0],m[1][1]);
        ya = -(T)Math::HALF_PI;
        za = 0.0f;
        return false;
      }
    }
    else
    {
      // Not unique.  XAngle + ZAngle = atan2(r10,r11)
      xa = std::atan2(m[1][0],m[1][1]);
      ya = (T)Math::HALF_PI;
      za = 0.0f;
      return false;
    }
  }

  /// Assign multiplication
  /// @param that matrix to multiply with
  /// @return Reference to self
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

    if(std::abs(fDet) <= tolerance ) {
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

  template <class T>
  Matrix3T<T> Matrix3T<T>::inverse23() const
  {

    Matrix3T<T> res;

    // Invert a 3x3 using cofactors.  This is about 8 times faster than
    // the Numerical Recipes code which uses Gaussian elimination.

    // Code from Wild Magic library.

    res[0][0] = m[1][1];
    res[0][1] = - m[0][1];
    res[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    res[1][0] = - m[1][0];
    res[1][1] = m[0][0];
    res[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];
    res[2][0] = 0;
    res[2][1] = 0;
    res[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];

    T fDet = m[0][0] * res[0][0] + m[0][1] * res[1][0] + m[0][2] * res[2][0];

    T fInvDet = 1.0f / fDet;
    for (int iRow = 0; iRow < 3; iRow++)
      for (int iCol = 0; iCol < 3; iCol++)
        res[iRow][iCol] *= fInvDet;
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
    res[0][i] = dot(m1.row(0),t);
    res[1][i] = dot(m1.row(1),t);
    res[2][i] = dot(m1.row(2),t);
  }

  return res;
}

/// Multiply a matrix and a vector
template <class K, class T>
inline Nimble::Vector3T<T> operator*(const Nimble::Matrix3T<K>& m1,
                   const Nimble::Vector3T<T>& m2)
{
  Nimble::Vector3T<T> res;
  for(int i = 0; i < 3; i++)
    res[i] = dot(m1.row(i),m2);
  return res;
}

/// Insert a 2x2 matrix to the upper-left corner of the 3x3 matrix
/// @param b matrix to insert
template<class T>
inline void Matrix3T<T>::insert(const Matrix2T<T>& b)
{
  m[0].x = b.get(0, 0);
  m[0].y = b.get(0, 1);

  m[1].x = b.get(1, 0);
  m[1].y = b.get(1, 1);
}

template<class T>
inline Matrix3T<T> Matrix3T<T>::makeTranslation(const T & x, const T & y)
{
  return Matrix3T<T>(
    1, 0, x,
    0, 1, y,
    0, 0, 1);
}

template<class T>
inline Matrix3T<T> Matrix3T<T>::makeScale(const T & xscale, const T & yscale)
{
  return Matrix3T<T>(
    xscale, 0, 0,
    0, yscale, 0,
    0, 0, 1);
}


template<class T>
inline Matrix3T<T> Matrix3T<T>::makeRotation(T radians)
{
  Matrix3T<T> m;
  m.rotateZ(radians);

  return m;
}

template<class T>
Matrix3T<T> Matrix3T<T>::makeRotation(T radians, const Vector3T<T> & axis)
{
  T c = T(cos(radians));
  T t = T(1) - c;
  T s = T(sin(radians));

  Vector3T<T> vn(axis);
  vn.normalize();

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
  Vector3T<T> p = *this * Nimble::Vector3T<T>(v, 1.f);
  return Vector2T<T>(p.x / p.z, p.y / p.z);
}

template<class T>
inline Vector2T<T> Matrix3T<T>::project(const T & x, const T & y) const
{
  Vector3T<T> p = *this * Vector3T<T>(x, y, 1);
  return Vector2T<T>(p.x / p.z, p.y / p.z);
}

/// Output the given matrix to a stream
/// @param os stream to output to
/// @param m matrix to output
/// @return reference to the stream
template <class T>
inline std::ostream& operator<<(std::ostream& os, const Nimble::Matrix3T<T>& m)
{
  os << m[0] << std::endl << m[1] << std::endl << m[2];
  return os;
}

/// Read a matrix from a stream
/// @param is stream to read from
/// @param m matrix to read to
/// @return reference to the input stream
template <class T>
inline std::istream & operator>>(std::istream & is, Nimble::Matrix3T<T> & m)
{
  is >> m[0] >> m[1] >> m[2];
  return is;
}

} // namespace

#endif
