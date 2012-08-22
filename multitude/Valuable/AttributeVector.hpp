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

#ifndef VALUABLE_VALUE_VECTOR_HPP
#define VALUABLE_VALUE_VECTOR_HPP

#include <Valuable/Export.hpp>
#include <Valuable/AttributeObject.hpp>

#include <Nimble/Vector4.hpp>

#include <Radiant/StringUtils.hpp>

#include "DOMElement.hpp"

#include <string.h>
#include <sstream>

namespace Valuable
{

  /** A template class for vector values.

      This class is used to implement all the normal vector value
      objects.
   */
  template<class VectorType>
  class AttributeVector : public AttributeT<VectorType>
  {
    typedef AttributeT<VectorType> Base;
    typedef typename VectorType::type ElementType;
    enum { N = VectorType::Elements };

    public:
      using Base::operator =;
      using Base::value;

      AttributeVector() : Base(0, "", VectorType::null(), false) {}
      /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
      /// @param v The value of this object
      AttributeVector(Node * host, const QString & name, const VectorType & v = VectorType::null(), bool transit = false)
        : Base(host, name, v, transit) {}

      virtual ~AttributeVector();

      /// Assigns by addition
      AttributeVector<VectorType> & operator += (const VectorType & v) { *this = value() + v; return *this; }
      /// Assigns by subtraction
      AttributeVector<VectorType> & operator -= (const VectorType & v) { *this = value() - v; return *this; }

      /// Subtraction operator
      VectorType operator -
      (const VectorType & v) const { return value() - v; }
      /// Addition operator
      VectorType operator +
      (const VectorType & v) const { return value() + v; }

      /// Access vector elements by their index.
      /// @param i Index, starting from zero
      /// @return Returns the ith element.
      ElementType operator [] (int i) const { return value()[i]; }

      /// Returns the data in its native format
      const ElementType * data() const
      { return value().data(); }

      virtual void processMessage(const QByteArray & id, Radiant::BinaryData & data) OVERRIDE;
      virtual bool deserialize(const ArchiveElement & element) OVERRIDE;

      virtual const char * type() const OVERRIDE;

      /// Sets the value
      // In some cases this is a override function, but not always
      /// @todo This should be fixed properly, but it's not important and just
      ///       fills the compiler output with the same warning
      virtual bool set(const VectorType & v, Attribute::Layer layer = Attribute::USER,
                                    QList<Attribute::ValueUnit> units = QList<Attribute::ValueUnit>());

      /// Returns the internal vector object as a constant reference.
      /// @return The wrapped vector value
      const VectorType & asVector() const { return value(); }

      virtual QString asString(bool * const ok = 0) const OVERRIDE;

      /// Returns the ith element
      inline const ElementType & get(int i) const { return value()[i]; }

      /// Returns the first component
      inline const ElementType & x() const { return value()[0]; }
      /// Returns the second component
      inline const ElementType & y() const { return value()[1]; }

      /// Normalizes the vector
      inline void normalize(ElementType len = 1.0)
      {
        VectorType vector = value();
        vector.normalize(len);
        *this = vector;
      }
  };

  /// An integer vector2 value object
  typedef AttributeVector<Nimble::Vector2i> AttributeVector2i;
  /// An integer vector3 value object
  typedef AttributeVector<Nimble::Vector3i> AttributeVector3i;
  /// An integer vector4 value object
  typedef AttributeVector<Nimble::Vector4i> AttributeVector4i;

  /// A float vector2 value object
  typedef AttributeVector<Nimble::Vector2f> AttributeVector2f;
  /// A float vector3 value object
  typedef AttributeVector<Nimble::Vector3f> AttributeVector3f;
  /// A float vector4 value object
  typedef AttributeVector<Nimble::Vector4f> AttributeVector4f;

  template<class VectorType>
  bool AttributeVector<VectorType>::deserialize(const ArchiveElement & element) {
    std::stringstream in(element.get().toUtf8().data());

    VectorType vector;
    for(int i = 0; i < N; i++)
      in >> vector[i];

    *this = vector;
    return true;
  }

  template<class VectorType>
  QString AttributeVector<VectorType>::asString(bool * const ok) const {
    if(ok) *ok = true;

    QString r = Radiant::StringUtils::stringify(value()[0]);

    for(int i = 1; i < N; i++)
      r += QString(" ") + Radiant::StringUtils::stringify(value()[i]);

    return r;
  }

  template<class VectorType>
  bool AttributeVector<VectorType>::set(const VectorType & v, Attribute::Layer layer,
    QList<Attribute::ValueUnit>)
  {
    this->setValue(v, layer);
    return true;
  }

  template <class T>
  AttributeVector<T>::~AttributeVector()
  {}

  template <class T>
  void AttributeVector<T>::processMessage(const QByteArray & id,
    Radiant::BinaryData & data)
  {
    /// @todo this isn't how processMessage should be used
    if(!id.isEmpty()) {
      int index = id.toInt();
      if(index >= N || index < 0) {
        return;
      }

      bool ok = true;

      ElementType v = data.read<ElementType>(&ok);

      if(ok) {
        T tmp = value();
        tmp[index] = v;
        *this = tmp;
      }
    }
    else {

      bool ok = true;

      T v = data.read<T>(&ok);

      if(ok)
        (*this) = v;
    }
  }

  template<class VectorType> const char *  AttributeVector<VectorType>::type() const { return "vector"; }
}

#endif
