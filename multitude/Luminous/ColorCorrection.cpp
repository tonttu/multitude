#include "ColorCorrection.hpp"

namespace Luminous
{

  ColorCorrection::ColorCorrection(HasValues * parent, const std::string & name, bool transit)
    : Valuable::HasValues(parent, name, transit),
      m_lut(this, "levels")
  {
    m_lut->resize(256);
    setIdentity();
  }

  Nimble::Vector3T<uint8_t>& ColorCorrection::getValue(int idx)
  {
    return m_lut->at(idx);
  }

  const Nimble::Vector3T<uint8_t>& ColorCorrection::getValue(int idx) const
  {
    return m_lut->at(idx);
  }

  const Nimble::Vector3T<uint8_t>* ColorCorrection::getLUT() const
  {
    return &((*m_lut)[0]);
  }

  void ColorCorrection::setIdentity()
  {
    for (int i=0; i < 256; ++i) {
      m_lut->at(i).make(i);
    }
  }

  // Change every value if given channel by v
  void ColorCorrection::changeUniform(int channel, int v)
  {
    for (int i=0; i < 256; ++i) {
      (*m_lut)[i][channel] = Nimble::Math::Clamp((*m_lut)[i][channel] + v, 0, 255);
    }
  }

}
