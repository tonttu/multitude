/* COPYRIGHT
 */

#ifndef NIMBLE_MATRIX4T_HPP
#define NIMBLE_MATRIX4T_HPP

#include "Nimble.hpp"
#include "Export.hpp"
#include "Matrix3.hpp"
#include "Vector4.hpp"

template <class T>
inline Nimble::Matrix4T<T> operator*(const Nimble::Matrix4T<T> &, const Nimble::Matrix4T<T> &);

template <class T>
inline Nimble::Vector4T<T> operator*(const Nimble::Matrix4T<T> &, const Nimble::Vector4T<T> &);

namespace Nimble {

  /// 4x4 transformation matrix
  /** This class is a row-major 4x4 matrix. The matrix functions
      (rotations etc.) assume right-handed coordinate system. */
  template <class T>
  class Matrix4T
  {
  public:
    typedef T type;

    enum { Elements = 4 };

    /// Constructs a matrix and fills it from memory
    template <class K>
    Matrix4T(const K * x) { const K * end = x + 16; T * my = data(); while(x!=end) *my++ = *x++; }
    /// Constructs a matrix without initializing it
    Matrix4T() {}
    /// Constructs a matrix and fills it with given values
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
    /// Constructs a matrix and initializes it with given row vectors
    Matrix4T(const Vector4T<T>& a, const Vector4T<T>& b, const Vector4T<T>& c, const Vector4T<T>& d)
    { m[0] = a; m[1] = b; m[2] = c; m[3] = d; }
    /// Returns the ith row
    Vector4T<T>&       row(int i)             { return m[i]; }
    /// Returns the ith row
    const Vector4T<T>& row(int i) const       { return m[i]; }

    /// Returns the ith column vector
    Vector4T<T>        column(int i) const    { return Vector4T<T>(m[0][i],m[1][i],m[2][i],m[3][i]); }
    /// Sets the ith column vector
    void               setColumn(int i, const Vector4T<T> &v) { m[0][i] = v[0]; m[1][i] = v[1]; m[2][i] = v[2]; m[3][i] = v[3]; }

    //void               setColumn3(int i, const Vector3T<T> &v) { m[0][i] = v[0]; m[1][i] = v[1]; m[2][i] = v[2]; m[3][i] = 1.0; }
    //void               setColumn3b(int i, const Vector3T<T> &v) { m[0][i] = v[0]; m[1][i] = v[1]; m[2][i] = v[2]; }
    //void               addToColumn(int i, const Vector4T<T> &v) { m[0][i] += v[0]; m[1][i] += v[1]; m[2][i] += v[2]; m[3][i] += v[3]; }
    //void               addToColumn(int i, const Vector3T<T> &v) { m[0][i] += v[0]; m[1][i] += v[1]; m[2][i] += v[2]; }

    /// Sets the diagonal to given vector
    void               setDiagonal(const Vector4T<T> &v) { m[0][0] = v[0]; m[1][1] = v[1]; m[2][2] = v[2]; m[3][3] += v[3]; }
    /// void               setDiagonal(const Vector3T<T> &v) { m[0][0] = v[0]; m[1][1] = v[1]; m[2][2] = v[2]; m[3][3] = (T) 1.0; }

    /// Returns the ith row
    /// @param i row number
    /// @return Reference to the row
    Vector4T<T>&       operator[](int i)      { return row(i); }
    /// Returns the ith row
    /// @param i row number
    /// @return Const reference to the row
    const Vector4T<T>& operator[](int i) const{ return row(i); }
    /// Replaces the upper-left 3x3 matrix
    inline void               setRotation(const Nimble::Matrix3T<T>& that);
    /// Returns the upper-left 3x3 matrix
    inline Matrix3T<T>        getRotation() const;
    /// Sets the translation part of a 4x4 transformation matrix
    void                      setTranslation(const Vector3T<T> & v)
    {
      m[0][3] = v.x;
      m[1][3] = v.y;
      m[2][3] = v.z;
    }

    /// Returns the translation part of a 4x4 matrix
    Vector3T<T>               getTranslation() const
    {
      return Nimble::Vector3T<T>(m[0][3], m[1][3], m[2][3]);
    }

    /// Transposes the matrix
    inline Matrix4T<T>&       transpose();
    /// Returns a transposed matrix
    inline Matrix4T<T> transposed() const { Matrix4T<T> m(*this); m.transpose(); return m; }
    /// Fills the matrix with zeroes
    void                      clear()         { m[0].clear(); m[1].clear(); m[2].clear(); m[3].clear(); }
    /// Sets the matrix to identity
    inline void               identity();
    /// Sets the matrix to a scaling matrix
    inline void               scalingMatrix(const Vector3T<T> &);
    /// Fills the matrix with given values
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
    /// Returns the inverse of the matrix
    inline Matrix4T<T>        inverse(bool * ok = 0) const;

    /// Multiplies two matrices together
    inline Matrix4T<T>&       operator*=(const Matrix4T<T>& that);
    /// Multiplies the matrix with a scalar
    Matrix4T<T>&              operator *= (T s) { T * p = data(); for(unsigned i=0; i < 16; i++) p[i] *= s; return * this; }
    /// Compares if two matrices are equal
    inline bool               operator==(const Matrix4T<T>& that) const;
    /// Compares if two matrices differ
    inline bool               operator!=(const Matrix4T<T>& that) const;

    /// Returns the number of rows (4)
    static int                rows() { return 4; }
    /// Returns the number of columns (4)
    static int                columns() { return 4; }

    /// Returns a pointer to the first element
    T *       data()       { return m[0].data(); }
    /// Returns a pointer to the first element
    const T * data() const { return m[0].data(); }

        /// Returns an orthonormalized version of this matrix.
        /// @todo could improve numerical stability easily etc.
        /// @return Normalized matrix
        Matrix4T<T> orthoNormalize() const
        {
            Matrix4T<T> tmp(transposed());
            Matrix4T<T> res(tmp);
            for (int i=0; i < 4; ++i) {
                Vector4T<T> & v = res.row(i);
                for (int j=0; j < i; ++j) {
                    v -= projection(res.row(j), tmp.row(i));
                }
            }

            for (int i=0; i < 4; ++i)
                res.row(i).normalize();

            return res.transposed();
        }

    /// Fills the matrix by copying values from memory
    template <class K>
    void copy (const K * x) { const K * end = x + 16; T * my = data(); while(x!=end) *my++ = (T) *x++; }
    /// Fills the matrix by copying transposed values from memory
    template <class K>
    void copyTranspose (const K * x) { for(int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) m[j][i] = (T) x[i*4+j]; }

    /// Apply the matrix on a 4D vector.
    /// @param v homogenous 3D vector
    /// @return projected 3D vector
    inline Vector3T<T> project(const Vector4T<T> & v) const
    {
      /// This is needed because there is an another operator* in Nimble-namespace in Vector3.hpp
      using ::operator*;
      Nimble::Vector4T<T> p = *this * v;
      return Nimble::Vector3T<T>(p.x / p.w, p.y / p.w, p.z / p.w);
    }

    /// Apply the matrix on a 3D vector.
    /// @param v 3D vector
    /// @return transformed 3D vector
    inline Vector3T<T> project(const Vector3T<T> & v) const
    {
      return project(Vector4T<T>(v.x, v.y, v.z, T(1.0)));
    }

    /// Creates a new WPCV-matrix (window-projection-camera-view -matrix)
    /// Camera is positioned so that 0,0,0 is mapped to 0,0,0 and
    /// w,h,0 is mapped to w,h,0. The projection matrix doesn't have the third
    /// row, no depth information, so no near/far values needed.
    /// (0,0,0) is lower left bottom, (w,h,0) is in right top, z increases towards the camera
    /// @param width Width of the viewport
    /// @param height Height of the viewport
    /// @param fovy Field of view in y-direction in radians
    /// @return New projection matrix
    static Matrix4T<T> simpleProjection(T width, T height, T fovy = Math::PI*0.5)
    {
      using ::operator*;

      // Camera distance to the center widget center point (assuming it's resting).
      T dist = height * T(.5) / Nimble::Math::Tan(fovy * T(.5));
      T aspect = width/height;

      // we won't be needing depth, so the third column is just zero unlike normally
      T f = T(1.0) / Nimble::Math::Tan(fovy*T(.5));
      Nimble::Matrix4T<T> projection(f/aspect, 0, 0, 0,
                                     0, f, 0, 0,
                                     0, 0, 0, 0,
                                     0, 0, -1, 0);

      // could just change the projection plane in projection matrix,
      // but maybe this is a bit more clear
      Matrix4T<T> camera = makeTranslation(Vector3f(0, 0, -dist));

      Matrix4T<T> window(width*0.5, 0, 0, 0.0 + width*0.5,
                         0, height * 0.5, 0, 0.0+height*0.5,
                         0, 0, 1, 0,
                         0, 0, 0, 1);

      Matrix4T<T> view = makeTranslation(Vector3f(-width*.5f, -height*.5f, 0));

      return window * projection * camera * view;
    }

    /// @todo duplicates (makeTranslation vs. translate3D)
    /// Create a rotation matrix
    /// @param radians angle in radians
    /// @param axis axis to rotate around
    /// @return New rotation matrix
    static Matrix4T<T> makeRotation(T radians, const Vector3T<T> & axis)
    {
      Nimble::Matrix4T<T> mm;
      mm.identity();

      mm.setRotation(Nimble::Matrix3T<T>::makeRotation(radians, axis));
      return mm;
    }

    /// Create a translation matrix
    /// @param v Translation vector
    /// @return New translation matrix
    static Matrix4T<T> makeTranslation(const Vector3T<T> & v)
    {
      Nimble::Matrix4T<T> mm;
      mm.identity();

      mm.setTranslation(v);
      return mm;
    }
    /// Create a translation matrix
    /// @param v Translation vector
    /// @return New translation matrix
    static Matrix4T<T> translate3D(const Vector3T<T> & v)
    {
      return Matrix4T(1, 0, 0, v[0],
                      0, 1, 0, v[1],
                      0, 0, 1, v[2],
                      0, 0, 0, 1);
    }

    /// Create a non-uniform scaling matrix
    /// @param v XYZ scaling factors
    /// @return new Scaling matrix
    static Matrix4T<T> scale3D(const Vector3T<T> & v)
    {
      return Matrix4T(v[0], 0, 0, 0,
                      0, v[1], 0, 0,
                      0, 0, v[2], 0,
                      0, 0, 0, 1);
    }

    /// Create a uniform scaling matrix
    /// @param s Scaling factor
    /// @return new Scaling matrix
    inline static Matrix4T<T> scaleUniform3D(const T & s)
    { return scale3D(Vector3T<T>(s, s, s)); }

    /// Creates a perspective projection matrix
    /// @param fovY field of view in degress in the Y direction
    /// @param aspect aspect ratio (width / height)
    /// @param nearPlane distance to the near clipping plane, always positive
    /// @param farPlane distance to the far clipping plane, always positive
    /// @return New projection matrix
    static Matrix4T<T> perspectiveProjection(T fovY, T aspect, T nearPlane, T farPlane)
    {
      assert(nearPlane > T(0));
      assert(farPlane > T(0));

      fovY = Nimble::Math::degToRad(fovY);

      const T f = T(1) / T(tan(fovY / T(2)));

      Nimble::Matrix4T<T> result;
      result.clear();

      result[0][0] = f / aspect;
      result[1][1] = f;
      result[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
      result[2][3] = T(2) * (farPlane * nearPlane) / (nearPlane - farPlane);
      result[3][2] = T(-1);

      return result;
    }

    /// Creates an orthogonal projection matrix
    /// @param left Left clip plane location
    /// @param right Right clip plane location
    /// @param bottom Bottom clip plane location
    /// @param top Top clip plane location
    /// @param near Near clip plane location
    /// @param far Far clip plane location
    /// @return New projection matrix
    static Matrix4T<T> orthogonalProjection(T left, T right, T bottom, T top, T near, T far)
    {
      Nimble::Matrix4T<T> result;
      result.clear();
      result[0][0] = T(2)/(right-left);
      result[1][1] = T(2)/(top-bottom);
      result[2][2] = -T(2)/(far-near);
      result[3][3] = T(1);
      result[0][3] = -(right+left)/(right-left);
      result[1][3] = -(top+bottom)/(top-bottom);
      result[2][3] = -(far+near)/(far-near);
      return result;
    }

    /// Creates an orthogonal projection matrix in 3D
    /** This function works in a way similar to glOrtho
        (http://lmb.informatik.uni-freiburg.de/people/reisert/opengl/doc/glOrtho.html).
    */
    static Matrix4T<T> ortho3D(T left, T right, T bottom, T top, T near, T far);

    /** Identity matrix. */
    static const Matrix4T<T> IDENTITY;

  private:
    inline static void swap(T &a, T& b);

    Vector4T<T> m[4];
  };

  template<typename T> const Matrix4T<T> Matrix4T<T>::IDENTITY(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1);

  /// Swaps two matrices
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
  inline Matrix4T<T> & Matrix4T<T>::transpose()
  {
    swap(m[0][1],m[1][0]);
    swap(m[0][2],m[2][0]);
    swap(m[0][3],m[3][0]);
    swap(m[1][2],m[2][1]);
    swap(m[1][3],m[3][1]);
    swap(m[2][3],m[3][2]);
    return * this;
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

  /// Inverts the matrix. The boolean argument is set to true or false
  /// depending on how well the operation went.
  /// @param[out] ok (optional) false if the inversion fails, otherwise true
  /// @return Inverse matrix
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
      if(ok) *ok = false;
      Matrix4T<T> tmp;
      tmp.identity();
      return tmp;
    }

    if(ok) *ok = true;

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

  template <class T> Matrix4T<T> Matrix4T<T>::ortho3D
    (T left, T right, T bottom, T top, T zNear, T zFar)
  {
    Matrix4T m1 = scale3D(Nimble::Vector3T<T>(1.0 / (right - left),
      1.0 / (top - bottom),
      1.0 / (zFar - zNear)));
    Matrix4T m2 = translate3D(Nimble::Vector3T<T>(-left, -bottom, -zNear));

    Matrix4T m3 = scale3D(Nimble::Vector3T<T>(2, 2, 2));
    Matrix4T m4 = translate3D(Nimble::Vector3T<T>(-1, -1, -1));

    return mul(mul(m4, m3), mul(m1, m2));
  }

#if 0
  /// @todo Move test out of the header
  template <class T>
  void Matrix4T<T>::test()
  {
    Matrix4T<T> a;
    int i,j;
    /* STORING & INDEXING ELEMENTS */
    for(i = 0; i < 4; i++)
      for(j = 0; j < 4; j++)
        a[i][j] = T(4*i+j);

    for(i = 0; i < 4; i++)
      for(j = 0; j < 4; j++)
        assert(a[i][j] == 4*i+j);

    /* CLEAR */
    a.clear();
    for(i = 0; i < 4; i++)
      for(j = 0; j < 4; j++)
        assert(a[i][j] == 0);

    /* ROW & COLUMN OPERATORS */
    for(i = 0; i < 4; i++)
      for(j = 0; j < 4; j++)
        a[i][j] = T(4*i+j);

    for(i = 0; i < 4; i++)
      for(j = 0; j < 4; j++)
      {
        assert(a.row(i)[j] == 4*i+j);
        assert(a.column(j)[i] == 4*i+j);
      }
      /* TRANSPOSE */
      a.transpose();
      for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
          assert(a[j][i] == 4*i+j);

      /* IDENTITY */
      a.identity();

      for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
        {
          if( i == j ) assert(a[i][j] == 1);
          else assert(a[i][j] == 0);
        }

        /* COPY OPERATOR, CONSTRUCTOR AND EQUALITY OPERATOR */

        for(i = 0; i < 4; i++)
          for(j = 0; j < 4; j++)
            a[i][j] = T(4*i+j);

        Matrix4T<T> b(a);
        assert(a == b);
        assert(!(a != b));

        /* MATRIX MULTIPLICATION */

        Matrix4T<T> c;
        c.identity();
        b *= c;
        assert(a == b);

        c.clear();
        b *= c;
        for(i = 0; i < 4; i++)
          for(j = 0; j < 4; j++)
            assert(b[i][j] == 0);

        /* Test setRotation/getRotation */

        Matrix3T<T> a3,b3;
        for(i = 0; i < 3; i++)
          for(j = 0; j < 3; j++)
            a[i][j] = T(3*i+j);

        a.setRotation(a3);
        b3 = a.getRotation();
        assert(a3 == b3);
  }
#endif
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
inline Nimble::Matrix4T<T> mul(const Nimble::Matrix4T<T>& m1,const Nimble::Matrix4T<T>& m2)
{
  return m1 * m2;
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
  os << m[0] << std::endl << m[1] << std::endl << m[2] << std::endl << m[3] << std::endl;
  return os;
}

#endif

