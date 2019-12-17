/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTESTYLEVALUE_HPP
#define VALUABLE_ATTRIBUTESTYLEVALUE_HPP

#include "Attribute.hpp"
#include "StyleValue.hpp"

namespace Valuable
{
  /// This class provides a StyleValue attribute.
  template <>
  class AttributeT<StyleValue> : public AttributeBaseT<StyleValue>
  {
  public:
    using AttributeBaseT<StyleValue>::operator=;

    AttributeT() {}
    AttributeT(Node * host, const QByteArray & name,
                        const StyleValue & v = StyleValue())
      : AttributeBaseT<StyleValue>(host, name, v)
    {}

    virtual bool set(float v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE
    {
      setValue(StyleValue(v, unit), layer);
      return true;
    }

    virtual bool set(int v, Layer layer = USER, ValueUnit = VU_UNKNOWN) OVERRIDE
    {
      setValue(StyleValue(v), layer);
      return true;
    }

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit = VU_UNKNOWN) OVERRIDE
    {
      setValue(StyleValue(v), layer);
      return true;
    }

    virtual bool set(const Nimble::Vector2f &, Layer, QList<ValueUnit>) OVERRIDE
    {
      return false;
    }

    virtual bool set(const Nimble::Vector3f &, Layer, QList<ValueUnit>) OVERRIDE
    {
      return false;
    }

    virtual bool set(const Nimble::Vector4f &, Layer, QList<ValueUnit>) OVERRIDE
    {
      return false;
    }

    virtual bool set(const StyleValue & value, Layer layer = USER) OVERRIDE
    {
      setValue(value, layer);
      return true;
    }

    virtual QString asString(bool * const ok, Layer layer) const OVERRIDE
    {
      if (ok) *ok = true;
      return value(layer).stringify();
    }

    virtual QByteArray type() const override
    {
      return "stylevalue";
    }

    static inline StyleValue interpolate(StyleValue a, StyleValue b, float m)
    {
      return m >= 0.5f ? b : a;
    }
  };
  typedef AttributeT<StyleValue> AttributeStyleValue;
} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESTYLEVALUE_HPP
