#include "AttributeMatrix.hpp"
#include "AttributeMatrixImpl.hpp"

namespace Valuable
{


  template class AttributeMatrix<Nimble::Matrix2f, float, 4>;
  template class AttributeMatrix<Nimble::Matrix3f, float, 9>;
  template class AttributeMatrix<Nimble::Matrix4f, float, 16>;

}
