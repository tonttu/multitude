#ifndef NIMBLE_MATRIX4T_IMPL_HPP
#define NIMBLE_MATRIX4T_IMPL_HPP

#include <Nimble/Matrix4.hpp>

namespace Nimble {

  template <class T>
  const Matrix4T<T> Matrix4T<T>::IDENTITY(1, 0, 0, 0,
                      0, 1, 0, 0,
                      0, 0, 1, 0,
                      0, 0, 0, 1);

  template <class T>
  Matrix4T<T> Matrix4T<T>::translate3D(const Vector3T<T> & v)
  {
    return Matrix4T(1, 0, 0, v[0],
                    0, 1, 0, v[1],
                    0, 0, 1, v[2],
                    0, 0, 0, 1);
  }

  template <class T>
  Matrix4T<T> Matrix4T<T>::scale3D(const Vector3T<T> & v)
  {
    return Matrix4T(v[0], 0, 0, 0,
                    0, v[1], 0, 0,
                    0, 0, v[2], 0,
                    0, 0, 0, 1);

  }

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


}

#endif

