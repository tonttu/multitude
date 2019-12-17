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
  /// This class provides an attribute for location
  class AttributeLocation2f : public AttributeTuple<Nimble::Vector2T<float>, AttributeLocation2f>
  {
  public:
    typedef AttributeTuple<Nimble::Vector2T<float>, AttributeLocation2f> Base;

    using Base::operator=;

    AttributeLocation2f() : Base(nullptr, QByteArray()) {}

    AttributeLocation2f(Node * host, const QByteArray & name,
                        const Nimble::Vector2T<float> & v = Nimble::Vector2T<float>(0, 0))
      : Base(host, name, v)
    {
    }

    virtual void priv_setWrapped(Nimble::Vector2T<float> &v, int index, float elem) const OVERRIDE
    {
      v[index] = elem;
    }

    float x() const
    {
      return m_values[0]->value();
    }

    float y() const
    {
      return m_values[1]->value();
    }

    void setX(float x, Attribute::Layer layer = Attribute::USER, Attribute::ValueUnit unit = Attribute::VU_PXS)
    {
      beginChangeTransaction();
      m_values[0]->set(x, layer, unit);
      endChangeTransaction();
    }

    void setY(float y, Attribute::Layer layer = Attribute::USER, Attribute::ValueUnit unit = Attribute::VU_PXS)
    {
      beginChangeTransaction();
      m_values[1]->set(y, layer, unit);
      endChangeTransaction();
    }

    virtual QByteArray type() const override
    {
      return "location2:float";
    }
  };
}

#endif // VALUABLE_ATTRIBUTE_LOCATION_HPP
