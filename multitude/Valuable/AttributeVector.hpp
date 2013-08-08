/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_VECTOR_HPP
#define VALUABLE_VALUE_VECTOR_HPP

#include "Export.hpp"
#include "Attribute.hpp"
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
  class AttributeVectorT : public AttributeT<VectorType>
  {
    /// GetVector<Nimble::Vector2i>::FloatVector == Nimble::Vector2f
    template <typename T> struct GetVector
    {
      typedef Nimble::Vector4f FloatVector;
    };

    template <typename Y, template <typename> class V>
    struct GetVector<V<Y>>
    {
      typedef V<float> FloatVector;
    };

    typedef AttributeT<VectorType> Base;
    typedef typename VectorType::type ElementType;
    enum { N = VectorType::ELEMENTS };

    public:
      using Base::operator =;
      using Base::value;

      AttributeVectorT() : Base(0, "", VectorType::null(), false) {}
      /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
      /// @param v The value of this object
      AttributeVectorT(Node * host, const QByteArray & name, const VectorType & v = VectorType::null(), bool transit = false)
        : Base(host, name, v, transit) {}

      virtual ~AttributeVectorT();

      /// Assigns by addition
      AttributeVectorT & operator += (const VectorType & v) { *this = value() + v; return *this; }
      /// Assigns by subtraction
      AttributeVectorT & operator -= (const VectorType & v) { *this = value() - v; return *this; }

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

      virtual void eventProcess(const QByteArray & id, Radiant::BinaryData & data) OVERRIDE;

      /// Sets the value
      virtual bool set(const StyleValue & value, Attribute::Layer layer = Attribute::USER) OVERRIDE;

      virtual bool set(const typename GetVector<VectorType>::FloatVector & v, Attribute::Layer layer = Attribute::USER,
                       QList<Attribute::ValueUnit> = QList<Attribute::ValueUnit>()) OVERRIDE
      {
        this->setValue(v.template cast<ElementType>(), layer);
        return true;
      }

      /// Returns the internal vector object as a constant reference.
      /// @return The wrapped vector value
      const VectorType & asVector() const { return value(); }

      virtual QString asString(bool * const ok, Attribute::Layer layer) const OVERRIDE;

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

  /// An integer Nimble::Vector2f value object
  typedef AttributeVectorT<Nimble::Vector2i> AttributeVector2i;
  /// An integer vector3 value object
  typedef AttributeVectorT<Nimble::Vector3i> AttributeVector3i;
  /// An integer vector4 value object
  typedef AttributeVectorT<Nimble::Vector4i> AttributeVector4i;

  /// A float Nimble::Vector2f value object
  typedef AttributeVectorT<Nimble::Vector2f> AttributeVector2f;
  /// A float vector3 value object
  typedef AttributeVectorT<Nimble::Vector3f> AttributeVector3f;
  /// A float vector4 value object
  typedef AttributeVectorT<Nimble::Vector4f> AttributeVector4f;

  template<class VectorType>
  QString AttributeVectorT<VectorType>::asString(bool * const ok, Attribute::Layer layer) const {
    if(ok) *ok = true;

    return Radiant::StringUtils::toString(value(layer));
  }

  template<class VectorType>
  bool AttributeVectorT<VectorType>::set(const StyleValue & value, Attribute::Layer layer)
  {
    if (value.size() != N || !value.isUniform() || !value.isNumber())
      return false;

    VectorType vector;
    for (int i = 0; i < N; ++i)
      vector[i] = value.asFloat(i);

    this->setValue(vector, layer);
    return true;
  }

  template <class T>
  AttributeVectorT<T>::~AttributeVectorT()
  {}

  template <class T>
  void AttributeVectorT<T>::eventProcess(const QByteArray & id,
    Radiant::BinaryData & data)
  {
    /// @todo this isn't how eventProcess should be used
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
