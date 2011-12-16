/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef VALUEMATRIX_HPP
#define VALUEMATRIX_HPP

#include <Valuable/Export.hpp>
#include <Valuable/AttributeObject.hpp>

#include <Nimble/Matrix4.hpp>

namespace Valuable
{

  /// A matrix value object
  template<class MatrixType, typename ElementType, int N>
  class VALUABLE_API AttributeMatrix : public AttributeT<MatrixType>
  {
    typedef AttributeT<MatrixType> Base;
  public:
    using Base::operator =;

    /// Create a new AttributeMatrix
    /// @param host host object
    /// @param name name of the value
    /// @param v the default/original value of the object
    /// @param transit ignored
    AttributeMatrix(Node * host, const QString & name, const MatrixType & v = MatrixType(), bool transit = false)
      : Base(host, name, v, transit) {}

    AttributeMatrix() : Base() {}
    virtual ~AttributeMatrix();

    /// Returns the data in its native format
    const ElementType * data() const
    { return this->value().data(); }

    // virtual void processMessage(const char * id, Radiant::BinaryData & data);
    virtual bool deserialize(const ArchiveElement & element) OVERRIDE;
    virtual const char * type() const OVERRIDE;
    virtual QString asString(bool * const ok = 0) const OVERRIDE;
  };

  /// A float Matrix2 value object
  typedef AttributeMatrix<Nimble::Matrix2f, float, 4> AttributeMatrix2f;
  /// A float Matrix3 value object
  typedef AttributeMatrix<Nimble::Matrix3f, float, 9> AttributeMatrix3f;
  /// A float Matrix4 value object
  typedef AttributeMatrix<Nimble::Matrix4f, float, 16> AttributeMatrix4f;
}

#endif // VALUEMATRIX_HPP
