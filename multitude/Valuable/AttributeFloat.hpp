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

#include <Radiant/StringUtils.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/AttributeNumeric.hpp>

namespace Valuable
{
  /// Template class for floating-point values.
  /** The actual value objects are created by using AttributeFloatT<float>
      etc.

      @see AttributeFloat. */
  template<class T>
  class AttributeFloatT : public AttributeNumeric<T>
  {
    typedef AttributeNumeric<T> Base;

    public:
      using Base::value;
      using Base::operator =;

      AttributeFloatT() : Base(), m_src(1)
      {
        for(int i = 0; i < Attribute::LAYER_COUNT; ++i)
          m_factors[i] = std::numeric_limits<float>::quiet_NaN();
      }
      /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
      /// @param v The numeric value of this object
      AttributeFloatT(Node * host, const QByteArray & name, T v = T(0), bool transit = false)
      : AttributeNumeric<T>(host, name, v, transit),
        m_src(1)
      {
        for(int i = 0; i < Attribute::LAYER_COUNT; ++i)
          m_factors[i] = std::numeric_limits<float>::quiet_NaN();
      }

      /// Assignment by subtraction
      AttributeFloatT<T> & operator -= (T i) { *this = value() - i; return *this; }
      /// Assignment by addition
      AttributeFloatT<T> & operator += (T i) { *this = value() + i; return *this; }
      /// Assignment by multiplication
      AttributeFloatT<T> & operator *= (T i) { *this = value() * i; return *this; }
      /// Assignment by division
      AttributeFloatT<T> & operator /= (T i) { *this = value() / i; return *this; }

      /// Sets the numeric value
      inline virtual bool set(int v, Attribute::Layer layer = Attribute::USER,
                              Attribute::ValueUnit = Attribute::VU_UNKNOWN)
      {
        m_factors[layer] = std::numeric_limits<float>::quiet_NaN();
        this->setValue(v, layer);
        return true;
      }
      /// @copydoc set
      inline virtual bool set(float v, Attribute::Layer layer = Attribute::USER,
                              Attribute::ValueUnit unit = Attribute::VU_UNKNOWN)
      {
        if(unit == Attribute::VU_PERCENTAGE) {
          setPercentage(v, layer);
          this->setValue(v * m_src, layer);
        } else {
          this->setValue(v, layer);
        }
        return true;
      }

      void setSrc(float src)
      {
        m_src = src;
        for(Attribute::Layer l = Attribute::DEFAULT; l < Attribute::LAYER_COUNT;
            l = Attribute::Layer(l + 1)) {
          if(!this->m_valueSet[l]) continue;
          if(!Nimble::Math::isNAN(m_factors[l]))
            this->setValue(m_factors[l] * src, l);
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

      virtual void processMessage(const QByteArray &, Radiant::BinaryData & data) OVERRIDE
      {
        bool ok = true;
        T v = data.read<T>( & ok);

        if(ok)
          *this = v;
      }

  private:
      float m_factors[Attribute::LAYER_COUNT];
      float m_src;
  };

  /// Float value object
  typedef AttributeFloatT<float> AttributeFloat;
}

#endif
