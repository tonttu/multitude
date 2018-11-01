/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_FLOAT_HPP
#define VALUABLE_VALUE_FLOAT_HPP

#include "SimpleExpression.hpp"
#include "StyleValue.hpp"

#include <Radiant/StringUtils.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/AttributeNumeric.hpp>

namespace Valuable
{
  /// Template class for floating-point values.
  /** The actual value objects are created by using AttributeFloatT<float>
      etc.

      @see AttributeFloat. */
  template <typename T>
  class AttributeT<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
      : public AttributeNumericT<T>
  {
    typedef AttributeNumericT<T> Base;

    public:
      using Base::value;
      using Base::operator =;

      AttributeT() : Base(), m_src(1)
      {
      }
      /// @copydoc Attribute::Attribute(Node *, const QString &)
      /// @param v The numeric value of this object
      AttributeT(Node * host, const QByteArray & name, T v = T(0))
      : AttributeNumericT<T>(host, name, v),
        m_src(1)
      {
      }

      /// Assignment by subtraction
      AttributeT & operator -= (T i) { *this = value() - i; return *this; }
      /// Assignment by addition
      AttributeT & operator += (T i) { *this = value() + i; return *this; }
      /// Assignment by multiplication
      AttributeT & operator *= (T i) { *this = value() * i; return *this; }
      /// Assignment by division
      AttributeT & operator /= (T i) { *this = value() / i; return *this; }

      /// Sets the numeric value
      inline virtual bool set(int v, Attribute::Layer layer = Attribute::USER,
                              Attribute::ValueUnit = Attribute::VU_UNKNOWN) OVERRIDE
      {
        if (layer >= Attribute::CURRENT_LAYER) layer = Base::currentLayer();
        if (m_exprs[layer]) m_exprs[layer].reset();
        Base::setValue(v, layer);
        return true;
      }
      /// @copydoc set
      inline virtual bool set(float v, Attribute::Layer layer = Attribute::USER,
                              Attribute::ValueUnit unit = Attribute::VU_UNKNOWN) OVERRIDE
      {
        if (layer >= Attribute::CURRENT_LAYER) layer = Base::currentLayer();
        if (m_exprs[layer]) m_exprs[layer].reset();
        if(unit == Attribute::VU_PERCENTAGE) {
          setPercentage(v, layer);
        } else {
          Base::setValue(v, layer);
        }
        return true;
      }

      virtual bool set(const StyleValue & value, Attribute::Layer layer = Attribute::USER) OVERRIDE
      {
        if (layer >= Attribute::CURRENT_LAYER) layer = Base::currentLayer();
        if (value.size() == 1 && value.type() == StyleValue::TYPE_EXPR) {
          m_exprs[layer].reset(new SimpleExpression(value.asExpr()));
          Base::setValue(m_exprs[layer]->evaluate(&m_src, 1), layer);
          return true;
        } else if (value.size() == 1 && value.isNumber() && value.unit() == Attribute::VU_PERCENTAGE) {
          setPercentage(value.asFloat(), layer);
          return true;
        } else {
          return Base::set(value, layer);
        }
      }

      void setSrc(float src)
      {
        m_src = src;
        for(Attribute::Layer l = Attribute::DEFAULT; l < Attribute::LAYER_COUNT;
            l = Attribute::Layer(l + 1)) {
          if(!this->isValueDefinedOnLayer(l)) continue;
          if(m_exprs[l]) {
            Base::setValue(m_exprs[l]->evaluate(&m_src, 1), l);
          }
        }
      }

      void setPercentage(float factor, Attribute::Layer layer = Attribute::USER)
      {
        if (layer >= Attribute::CURRENT_LAYER) layer = Base::currentLayer();
        SimpleExpression expr(factor);
        expr.replace(SimpleExpression::OP_MUL, SimpleExpression::Param(0));
        m_exprs[layer].reset(new SimpleExpression(expr));
        Base::setValue(m_exprs[layer]->evaluate(&m_src, 1), layer);
      }

      float percentage(Attribute::Layer layer) const
      {
        if (layer >= Attribute::CURRENT_LAYER) layer = Base::currentLayer();
        if(!m_exprs[layer] || m_exprs[layer]->isConstant())
          return std::numeric_limits<float>::quiet_NaN();
        return m_exprs[layer]->evaluate({1.f});
      }

      virtual void setValue(const T & v, Attribute::Layer layer = Attribute::USER) override
      {
        layer = layer == Attribute::CURRENT_LAYER ? Base::currentLayer() : layer;
        if (m_exprs[layer]) m_exprs[layer].reset();
        Base::setValue(v, layer);
      }

      virtual void clearValue(Attribute::Layer layer = Attribute::USER) OVERRIDE
      {
        if (layer >= Attribute::CURRENT_LAYER) layer = Base::currentLayer();
        if (m_exprs[layer]) m_exprs[layer].reset();
        Base::clearValue(layer);
      }

      virtual void eventProcess(const QByteArray &, Radiant::BinaryData & data) OVERRIDE
      {
        bool ok = true;
        T v = data.read<T>( & ok);

        if(ok)
          *this = v;
      }

      virtual void setAsDefaults() OVERRIDE
      {
        if (!this->isValueDefinedOnLayer(Attribute::USER))
          return;
        m_exprs[Attribute::DEFAULT] = std::move(m_exprs[Attribute::USER]);
        if (m_exprs[Attribute::USER]) m_exprs[Attribute::USER].reset();
        Base::setValue(this->value(Attribute::USER), Attribute::DEFAULT);
        clearValue(Attribute::USER);
      }

  private:
      float m_src;
      std::unique_ptr<SimpleExpression> m_exprs[Attribute::LAYER_COUNT];
  };

  /// Float value object
  typedef AttributeT<float> AttributeFloat;
}

#endif
