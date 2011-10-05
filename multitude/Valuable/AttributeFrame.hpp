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

#ifndef VALUABLE_ATTRIBUTE_FRAME_HPP
#define VALUABLE_ATTRIBUTE_FRAME_HPP

#include <Nimble/Frame4.hpp>

#include "AttributeVector.hpp"

namespace Valuable
{
  class VALUABLE_API AttributeFrame : public AttributeVector<Nimble::Frame4f>
  {
  public:
    using AttributeVector<Nimble::Frame4f>::operator =;
    //using Base::value;

    AttributeFrame(Node * host, const QString & name,
                   const Nimble::Frame4f & v = Nimble::Frame4f(), bool transit = false)
      : AttributeVector<Nimble::Frame4f>(host, name, v, transit)
    {
      for(int i = 0; i < Attribute::LAYER_COUNT; ++i)
        for(int j = 0; j < m_factors[i].Elements; ++j)
          m_factors[i][j] = std::numeric_limits<float>::quiet_NaN();
    }

    virtual bool set(float v, Layer layer = MANUAL, ValueUnit unit = VU_UNKNOWN)
    {
      const Nimble::Frame4f f(v, v, v, v);
      if(unit == VU_PERCENTAGE) {
        m_factors[layer] = f;
        setValue(f * m_src, layer);
      } else {
        for(int j = 0; j < m_factors[layer].Elements; ++j)
          m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
        setValue(f, layer);
      }
      return true;
    }

    virtual bool set(int v, Layer layer = MANUAL, ValueUnit unit = VU_UNKNOWN)
    {
      for(int j = 0; j < m_factors[layer].Elements; ++j)
        m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
      setValue(Nimble::Frame4f(v, v, v, v), layer);
      return true;
    }

    virtual bool set(const Nimble::Vector2f & v, Layer layer = MANUAL, QList<ValueUnit> units = QList<ValueUnit>())
    {
      Nimble::Frame4f f(v.x, v.y, v.x, v.y);
      for(int j = 0; j < m_factors[layer].Elements; ++j) {
        if(units[j % 2] == VU_PERCENTAGE) {
          m_factors[layer][j] = f[j];
          f[j] *= m_src;
        } else {
          m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
        }
      }
      setValue(f, layer);
      return true;
    }

    virtual bool set(const Nimble::Vector3f & v, Layer layer = MANUAL, QList<ValueUnit> units = QList<ValueUnit>())
    {
      Nimble::Frame4f f(v.x, v.y, v.z, v.x);
      for(int j = 0; j < m_factors[layer].Elements; ++j) {
        if(units[j % 3] == VU_PERCENTAGE) {
          m_factors[layer][j] = f[j];
          f[j] *= m_src;
        } else {
          m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
        }
      }
      setValue(f, layer);
      return true;
    }

    virtual bool set(const Nimble::Vector4f & v, Layer layer = MANUAL, QList<ValueUnit> units = QList<ValueUnit>())
    {
      Nimble::Frame4f f(v);
      for(int j = 0; j < m_factors[layer].Elements; ++j) {
        if(units[j] == VU_PERCENTAGE) {
          m_factors[layer][j] = f[j];
          f[j] *= m_src;
        } else {
          m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
        }
      }
      setValue(f, layer);
      return true;
    }

    void setSrc(float src)
    {
      m_src = src;
      for(Attribute::Layer l = Attribute::ORIGINAL; l < Attribute::LAYER_COUNT; ++((int&)l)) {
        if(!m_valueSet[l]) continue;
        int count = 0;
        Nimble::Frame4f f = value(l);
        for(int j = 0; j < m_factors[l].Elements; ++j) {
          if(!Nimble::Math::isNAN(m_factors[l][j])) {
            ++count;
            f[j] = m_factors[l][j] * src;
          }
        }
        if(count)
          this->setValue(f, l);
      }
    }

    virtual void clearValue(Attribute::Layer layer)
    {
      for(int j = 0; j < m_factors[layer].Elements; ++j)
        m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
      AttributeVector<Nimble::Frame4f>::clearValue(layer);
    }

  private:
    Nimble::Vector4f m_factors[Attribute::LAYER_COUNT];
    float m_src;
  };

}

#endif // VALUABLE_ATTRIBUTE_FRAME_HPP
