/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_NUMERIC_HPP
#define VALUABLE_VALUE_NUMERIC_HPP

#include <Radiant/StringUtils.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/Attribute.hpp>
#include <Valuable/StyleValue.hpp>
#include <Valuable/SimpleExpression.hpp>


namespace Valuable
{
  /// A template base class for numeric values.
  /** The actual value classes are inherited from this template
      class. */
  template<class T>
  class AttributeNumericT : public AttributeBaseT<T>
  {
    typedef AttributeBaseT<T> Base;

  public:
      using Base::value;
      using Base::operator =;

      AttributeNumericT() : Base() {}
      /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
      /// @param v The numeric value of this object.
      AttributeNumericT(Node * host, const QByteArray & name, T v)
      : Base(host, name, v)
      {}

    /// Converts the numeric value to float
    virtual float asFloat(bool * const ok = nullptr, Attribute::Layer layer = Attribute::CURRENT_VALUE) const OVERRIDE
    {
      if (ok) *ok = true;
      return static_cast<float> (value(layer));
    }

    /// Converts the numeric value to integer
    virtual int asInt(bool * const ok = nullptr, Attribute::Layer layer = Attribute::CURRENT_VALUE) const OVERRIDE
    {
      if (ok) *ok = true;
      return static_cast<int> (value(layer));
    }

    virtual bool set(const StyleValue &value, Attribute::Layer layer) OVERRIDE
    {
      if(value.size() != 1)
        return false;

      auto type = value.type();
      if(type == StyleValue::TYPE_EXPR) {
        SimpleExpression expr = value.asExpr();
        this->setValue(static_cast<T>(expr.evaluate(nullptr, 0)), layer);
        return true;
      } else if(type == StyleValue::TYPE_INT || type == StyleValue::TYPE_FLOAT) {
        this->setValue(static_cast<T>(value.asFloat()), layer);
        return true;
      }
      return false;
    }

    /// Converts the numeric value to string
    virtual QString asString(bool * const ok = nullptr, Attribute::Layer layer = Attribute::CURRENT_VALUE) const OVERRIDE
    {
      if (ok) *ok = true;
      return Radiant::StringUtils::toString(value(layer));
    }
  };

}

#endif
