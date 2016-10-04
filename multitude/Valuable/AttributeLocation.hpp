/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTE_LOCATION_HPP
#define VALUABLE_ATTRIBUTE_LOCATION_HPP

#include "AttributeTuple.hpp"
#include <Nimble/Vector2.hpp>

namespace Valuable
{

  /// This class is needed as template parameter to differentiate with AttributeVectors
  class Separate {};

  /// This class provides an attribute for location
  template <typename T>
  class AttributeT<Nimble::Vector2T<T>, Separate> :
      public AttributeTuple<Nimble::Vector2T<T>, AttributeT<Nimble::Vector2T<T>, Separate>>
  {
  public:
    typedef AttributeT<Nimble::Vector2T<T>, Separate> AttributeType;
    typedef AttributeTuple<Nimble::Vector2T<T>, AttributeType> Base;

    using Base::operator=;


    AttributeT(Node * host, const QByteArray & name,
               const Nimble::Vector2T<T> & v = Nimble::Vector2T<T>(0, 0))
      : Base(host, name, v)
    {
    }

    virtual void priv_setWrapped(Nimble::Vector2T<T> &v, int index, T elem) const OVERRIDE
    {
      v[index] = elem;
    }

    T x() const
    {
      return this->m_values[0]->value();
    }

    T y() const
    {
      return this->m_values[1]->value();
    }

    void setX(float x, Attribute::Layer layer = Attribute::USER, Attribute::ValueUnit unit = Attribute::VU_PXS)
    {
      Base::beginChangeTransaction();
      Base::m_values[0]->set(x, layer, unit);
      Base::endChangeTransaction();
    }

    void setY(float y, Attribute::Layer layer = Attribute::USER, Attribute::ValueUnit unit = Attribute::VU_PXS)
    {
      Base::beginChangeTransaction();
      Base::m_values[1]->set(y, layer, unit);
      Base::endChangeTransaction();
    }

  };

  typedef AttributeT<Nimble::Vector2T<float>, Separate> AttributeLocation2f;
}

#endif // VALUABLE_ATTRIBUTE_LOCATION_HPP
