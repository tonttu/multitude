
#ifndef NIMBLE_MATRIX3T_IMPL_HPP
#define NIMBLE_MATRIX3T_IMPL_HPP

#include <Nimble/Matrix3.hpp>

namespace Nimble {

  template <class T>
  Matrix3T<T> Matrix3T<T>::rotateAroundPoint2D(Vector2T<T> p,
                                               T radians)
  {
    return translate2D(p) * rotate2D(radians) * translate2D(-p);
  }

  template <class T>
  Matrix3T<T> Matrix3T<T>::scaleUniformAroundPoint2D(Vector2T<T> p,
                                                     T s)
  {
    return translate2D(p) * scaleUniform2D(s) * translate2D(-p);
  }

  template <class T>
  const Matrix3T<T> Matrix3T<T>::IDENTITY(1, 0, 0,
                                          0, 1, 0,
                                          0, 0, 1);

}

#endif

