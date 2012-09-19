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

#include <Radiant/StringUtils.hpp>

#include <Nimble/Matrix4.hpp>

namespace Valuable
{

  /// A matrix value object
  template<class MatrixType, typename ElementType, int N>
  class AttributeMatrix : public AttributeT<MatrixType>
  {
    typedef AttributeT<MatrixType> Base;
  public:
    using Base::operator =;

    /// Create a new AttributeMatrix
    /// @param host host object
    /// @param name name of the value
    /// @param v the default/original value of the object
    /// @param transit ignored
    AttributeMatrix(Node * host, const QByteArray & name, const MatrixType & v = MatrixType(), bool transit = false)
      : Base(host, name, v, transit) {}

    AttributeMatrix() : Base() {}
    virtual ~AttributeMatrix();

    /// Returns the data in its native format
    const ElementType * data() const
    { return this->value().data(); }

    // virtual void processMessage(const QByteArray & id, Radiant::BinaryData & data);
    virtual bool deserialize(const ArchiveElement & element) OVERRIDE;
    virtual const char * type() const OVERRIDE { return "Matrix"; }
    virtual QString asString(bool * const ok = 0) const OVERRIDE;
  };


  
  template <class T, typename S, int N>
  AttributeMatrix<T,S,N>::~AttributeMatrix()
  {}

  /*
  template <class T, typename S, int N>
  void AttributeMatrix<T,S,N>::processMessage(const QByteArray & id,
                      Radiant::BinaryData & data)
  {
    if(id && strlen(id)) {
      int index = strtol(id, 0, 10);
      if(index >= N) {
        return;
      }

      bool ok = true;

      S v = data.read<S>(&ok);

      if(ok) {
        T tmp = Base::m_value;
        tmp.data()[index] = v;
        (*this) = tmp;
      }
    }
    else {

      bool ok = true;

      T v = data.read<T>(&ok);

      if(ok)
        (*this) = v;
    }
  }
  */

  template<class MatrixType, typename ElementType, int N>
  bool AttributeMatrix<MatrixType, ElementType, N>::deserialize(const ArchiveElement & element) {
    std::stringstream in(element.get().toStdString());

    MatrixType m;
    for(int i = 0; i < N; i++)
      in >> m.data()[i];

    *this = m;
    return true;
  }

  template<class MatrixType, typename ElementType, int N>
  QString AttributeMatrix<MatrixType, ElementType, N>::asString(bool * const ok) const {
    if(ok) *ok = true;

    const ElementType * buf = data();
    QString r = Radiant::StringUtils::stringify(buf[0]);

    for(int i = 1; i < N; i++)
      r += " " + Radiant::StringUtils::stringify(buf[i]);

    return r;
  }

  /// A float Matrix2 value object
  typedef AttributeMatrix<Nimble::Matrix2f, float, 4> AttributeMatrix2f;
  /// A float Matrix3 value object
  typedef AttributeMatrix<Nimble::Matrix3f, float, 9> AttributeMatrix3f;
  /// A float Matrix4 value object
  typedef AttributeMatrix<Nimble::Matrix4f, float, 16> AttributeMatrix4f;
}

#endif // VALUEMATRIX_HPP
