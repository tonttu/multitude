/* COPYRIGHT
*/


#include "Matrix2.hpp"
#include "Matrix3.hpp"
#include "Matrix4Impl.hpp"

namespace Nimble
{

  template<> const Matrix2T<float> Matrix2T<float>::IDENTITY(
        1, 0,
        0, 1);

  template<> const Matrix2T<double> Matrix2T<double>::IDENTITY(
        1, 0,
        0, 1);

  template<> const Matrix3T<float> Matrix3T<float>::IDENTITY(
        1, 0, 0,
        0, 1, 0,
        0, 0, 1);

  template<> const Matrix3T<double> Matrix3T<double>::IDENTITY(
        1, 0, 0,
        0, 1, 0,
        0, 0, 1);

  template<> const Matrix4T<float> Matrix4T<float>::IDENTITY(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);

  template<> const Matrix4T<double> Matrix4T<double>::IDENTITY(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
}


//template class Nimble::Matrix2T<float>;
//template class Nimble::Matrix2T<double>;
//
//template class Nimble::Matrix3T<float>;
//template class Nimble::Matrix3T<double>;
//
template class Nimble::Matrix4T<float>;
template class Nimble::Matrix4T<double>;
//
