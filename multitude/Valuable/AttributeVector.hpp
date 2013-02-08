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

#include "Export.hpp"
#include "AttributeObject.hpp"
#include "StyleValue.hpp"

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
      AttributeVector(Node * host, const QByteArray & name, const VectorType & v = VectorType::null(), bool transit = false)
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

      /// Sets the value
      virtual bool set(const StyleValue & value, Attribute::Layer layer = Attribute::USER) OVERRIDE;

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

  template <template <typename Y> class VectorT, typename T>
  class AttributeVectorT : public AttributeVector<VectorT<T> >
  {
    typedef VectorT<T> VectorType;
    typedef AttributeVector<VectorType> Base;
    enum { N = VectorType::Elements };

  public:
    using Base::operator =;
    using Base::value;

    AttributeVectorT() : Base() {}
    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param v The value of this object
    AttributeVectorT(Node * host, const QByteArray & name, const VectorType & v = VectorType::null(), bool transit = false)
      : Base(host, name, v, transit) {}

    virtual bool set(const VectorT<float> & v, Attribute::Layer layer = Attribute::USER,
                     QList<Attribute::ValueUnit> = QList<Attribute::ValueUnit>()) OVERRIDE
    {
      this->setValue(v.template cast<T>(), layer);
      return true;
    }

    virtual ~AttributeVectorT() {}
  };

  /// An integer Nimble::Vector2f value object
  typedef AttributeVectorT<Nimble::Vector2T, int> AttributeVector2i;
  /// An integer vector3 value object
  typedef AttributeVectorT<Nimble::Vector3T, int> AttributeVector3i;
  /// An integer vector4 value object
  typedef AttributeVectorT<Nimble::Vector4T, int> AttributeVector4i;

  /// A float Nimble::Vector2f value object
  typedef AttributeVectorT<Nimble::Vector2T, float> AttributeVector2f;
  /// A float vector3 value object
  typedef AttributeVectorT<Nimble::Vector3T, float> AttributeVector3f;
  /// A float vector4 value object
  typedef AttributeVectorT<Nimble::Vector4T, float> AttributeVector4f;

  template<class VectorType>
  QString AttributeVector<VectorType>::asString(bool * const ok) const {
    if(ok) *ok = true;

    return Radiant::StringUtils::toString(value());
  }

  template<class VectorType>
  bool AttributeVector<VectorType>::set(const StyleValue & value, Attribute::Layer layer)
  {
    if (value.size() != N || !value.uniform() || !value.isNumber())
      return false;

    VectorType vector;
    for (int i = 0; i < N; ++i)
      vector[i] = value.asFloat(i);

    this->setValue(vector, layer);
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

}

#endif
