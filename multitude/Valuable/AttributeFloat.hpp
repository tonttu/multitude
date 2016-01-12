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
        for(int i = 0; i < Attribute::LAYER_COUNT; ++i)
          m_factors[i] = std::numeric_limits<float>::quiet_NaN();
      }
      /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
      /// @param v The numeric value of this object
      AttributeT(Node * host, const QByteArray & name, T v = T(0), bool transit = false)
      : AttributeNumericT<T>(host, name, v, transit),
        m_src(1)
      {
        for(int i = 0; i < Attribute::LAYER_COUNT; ++i)
          m_factors[i] = std::numeric_limits<float>::quiet_NaN();
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
        m_exprs[layer].reset();
        m_factors[layer] = std::numeric_limits<float>::quiet_NaN();
        this->setValue(v, layer);
        return true;
      }
      /// @copydoc set
      inline virtual bool set(float v, Attribute::Layer layer = Attribute::USER,
                              Attribute::ValueUnit unit = Attribute::VU_UNKNOWN) OVERRIDE
      {
        m_exprs[layer].reset();
        if(unit == Attribute::VU_PERCENTAGE) {
          setPercentage(v, layer);
          this->setValue(v * m_src, layer);
        } else {
          m_factors[layer] = std::numeric_limits<float>::quiet_NaN();
          this->setValue(v, layer);
        }
        return true;
      }

      virtual bool set(const StyleValue & value, Attribute::Layer layer = Attribute::USER) OVERRIDE
      {
        if (value.size() == 1 && value.type() == StyleValue::TYPE_EXPR) {
          setPercentage(std::numeric_limits<float>::quiet_NaN(), layer);
          m_exprs[layer].reset(new SimpleExpression(value.asExpr()));
          this->setValue(m_exprs[layer]->evaluate({m_src}), layer);
          return true;
        }
        return false;
      }

      void setSrc(float src)
      {
        m_src = src;
        for(Attribute::Layer l = Attribute::DEFAULT; l < Attribute::LAYER_COUNT;
            l = Attribute::Layer(l + 1)) {
          if(!this->isValueDefinedOnLayer(l)) continue;
          if(m_exprs[l]) {
            this->setValue(m_exprs[l]->evaluate({m_src}), l);
          } else if(!Nimble::Math::isNAN(m_factors[l])) {
            this->setValue(m_factors[l] * src, l);
          }
        }
      }

      void setPercentage(float factor, Attribute::Layer layer = Attribute::USER)
      {
        m_factors[layer] = factor;
      }

      float percentage(Attribute::Layer layer) const
      {
        return m_factors[layer];
      }

      virtual void clearValue(Attribute::Layer layer = Attribute::USER) OVERRIDE
      {
        m_factors[layer] = std::numeric_limits<float>::quiet_NaN();
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
        m_factors[Attribute::DEFAULT] = m_factors[Attribute::USER];
        this->setValue(this->value(Attribute::USER), Attribute::DEFAULT);
        clearValue(Attribute::USER);
      }

  private:
      float m_factors[Attribute::LAYER_COUNT];
      float m_src;
      std::unique_ptr<SimpleExpression> m_exprs[Attribute::LAYER_COUNT];
  };

  /// Float value object
  typedef AttributeT<float> AttributeFloat;
}

#endif
