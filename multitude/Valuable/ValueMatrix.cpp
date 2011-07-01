#include "ValueMatrix.hpp"
#include "ValueMatrixImpl.hpp"

namespace Valuable
{


  template class ValueMatrix<Nimble::Matrix2f, float, 4>;
  template class ValueMatrix<Nimble::Matrix3f, float, 9>;
  template class ValueMatrix<Nimble::Matrix4f, float, 16>;

}
