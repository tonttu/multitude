#include "ColorCorrection.hpp"

namespace Luminous
{

  ColorCorrection::ColorCorrection(Node * parent, const QString & name, bool transit)
    : Valuable::Node(parent, name, transit),
      m_offsets(this, "offsets"),
      m_gamma(this, "gamma", Nimble::Vector3(1.0f, 1.0f, 1.0f)),
      m_contrast(this, "contrast", Nimble::Vector3(1.0f, 1.0f, 1.0f)),
      m_brightness(this, "brightness", Nimble::Vector3(0.0f, 0.0f, 0.0f)),
      m_identity(true)
  {
    m_offsets->resize(256);
    setIdentity();
    m_offsets.addListener(std::bind(&ColorCorrection::setChanged, this));
    m_gamma.addListener(std::bind(&ColorCorrection::setChanged, this));
    m_contrast.addListener(std::bind(&ColorCorrection::setChanged, this));
    m_brightness.addListener(std::bind(&ColorCorrection::setChanged, this));
  }

  void ColorCorrection::setOffset(int idx, const Nimble::Vector3 & offset)
  {
    m_offsets->at(idx) = offset;
    setChanged();
  }

  void ColorCorrection::setOffset(int idx, int channel, float value)
  {
    m_offsets->at(idx).set(channel, value);
    setChanged();
  }


  const Nimble::Vector3& ColorCorrection::getOffset(int idx) const
  {
    return m_offsets->at(idx);
  }

  Nimble::Vector3 ColorCorrection::getValue(int idx) const
  {
    Nimble::Vector3 v = getOffset(idx);

    float x = idx/255.0f;


    v += x*m_contrast.asVector() + 0.5f*(Nimble::Vector3(1) - m_contrast);

    return Nimble::Vector3(
          Nimble::Math::Pow(v[0], m_gamma[0]) + m_brightness[0],
          Nimble::Math::Pow(v[1], m_gamma[1]) + m_brightness[1],
          Nimble::Math::Pow(v[2], m_gamma[2]) + m_brightness[2]).clampUnit();
  }

  bool ColorCorrection::isIdentity() const
  {
    return m_identity;
  }

  void ColorCorrection::setIdentity()
  {
    for (int i=0; i < 256; ++i) {
      m_offsets->at(i).make( 0 * i/255.0f);
    }
    m_gamma = Nimble::Vector3(1);
    m_contrast = Nimble::Vector3(1);
    m_brightness = Nimble::Vector3(0);
    m_identity = true;
  }

  // Change every value if given channel by v
  void ColorCorrection::changeUniform(int channel, float v)
  {
    for (int i=0; i < 256; ++i) {
      (*m_offsets)[i][channel] = Nimble::Math::Clamp((*m_offsets)[i][channel] + v, 0.0f, 1.0f);
    }
    setChanged();
  }

  void ColorCorrection::fillAsBytes(Nimble::Vector3T<uint8_t> * to) const
  {
    for (int i=0; i < 256; ++i) {
      Nimble::Vector3 v = getValue(i);
      for (int c=0; c < 3; ++c) {
        (*to)[c] = Nimble::Math::Clamp(Nimble::Math::Round(v[c] * 255.0f), 0, 255);
      }
      ++to;
    }
  }

  void ColorCorrection::setChanged()
  {
    Nimble::Vector3T<uint8_t> tmp[256];
    fillAsBytes(tmp);
    m_identity = true;
    for(int i = 0; i < 256; ++i) {
      if(tmp[i].x != i || tmp[i].y != i || tmp[i].z != i) {
        m_identity = false;
        break;
      }
    }
  }

}
