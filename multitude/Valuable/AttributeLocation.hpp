#ifndef VALUABLE_ATTRIBUTE_LOCATION_HPP
#define VALUABLE_ATTRIBUTE_LOCATION_HPP

#include "AttributeVector.hpp"

namespace Valuable
{

  class AttributeLocation2f : public AttributeVector<Nimble::Vector2f>
  {
  public:
    using AttributeVector<Nimble::Vector2f>::operator =;

    AttributeLocation2f(Node * host, const QString & name,
                        const Nimble::Vector2f & v = Nimble::Vector2f(0, 0),
                        bool transit = false)
      : AttributeVector<Nimble::Vector2f>(host, name, v, transit)
    {
      for(int i = 0; i < Attribute::LAYER_COUNT; ++i)
        for(int j = 0; j < m_factors[i].Elements; ++j)
          m_factors[i][j] = std::numeric_limits<float>::quiet_NaN();
    }

    virtual bool set(float v, Layer layer = MANUAL, ValueUnit unit = VU_UNKNOWN) OVERRIDE
    {
      const Nimble::Vector2f f(v, v);
      if(unit == VU_PERCENTAGE) {
        m_factors[layer] = f;
        setValue(Nimble::Vector2(f.x * m_src.x, f.y * m_src.y), layer);
      } else {
        for(int j = 0; j < m_factors[layer].Elements; ++j)
          m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
        setValue(f, layer);
      }
      return true;
    }

    virtual bool set(int v, Layer layer = MANUAL, ValueUnit = VU_UNKNOWN) OVERRIDE
    {
      for(int j = 0; j < m_factors[layer].Elements; ++j)
        m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
      setValue(Nimble::Vector2f(v, v), layer);
      return true;
    }

    virtual bool set(const Nimble::Vector2f & v, Layer layer = MANUAL, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      Nimble::Vector2f f(v);
      for(int j = 0; j < m_factors[layer].Elements; ++j) {
        if(j < units.size() && units[j] == VU_PERCENTAGE) {
          m_factors[layer][j] = f[j];
          f[j] *= m_src[j];
        } else {
          m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
        }
      }
      setValue(f, layer);
      return true;
    }

    void setSrcx(float src)
    {
      m_src.x = src;
      for(Attribute::Layer l = Attribute::ORIGINAL; l < Attribute::LAYER_COUNT;
          l = Attribute::Layer(l + 1)) {
        if(!m_valueSet[l]) continue;
        if(!Nimble::Math::isNAN(m_factors[l].x))
          this->setValue(Nimble::Vector2f(src * m_factors[l].x, value().y), l);
      }
    }

    void setSrcy(float src)
    {
      m_src.y = src;
      for(Attribute::Layer l = Attribute::ORIGINAL; l < Attribute::LAYER_COUNT;
          l = Attribute::Layer(l + 1)) {
        if(!m_valueSet[l]) continue;
        if(!Nimble::Math::isNAN(m_factors[l].y))
          this->setValue(Nimble::Vector2f(value().x, src * m_factors[l].y), l);
      }
    }

    virtual void clearValue(Attribute::Layer layer = MANUAL) OVERRIDE
    {
      for(int j = 0; j < m_factors[layer].Elements; ++j)
        m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
      AttributeVector<Nimble::Vector2f>::clearValue(layer);
    }

  private:
    Nimble::Vector2f m_factors[Attribute::LAYER_COUNT];
    Nimble::Vector2f m_src;
  };

}

#endif // VALUABLE_ATTRIBUTE_LOCATION_HPP
