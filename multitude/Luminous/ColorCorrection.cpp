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

    eventAddOut("changed");
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

  void ColorCorrection::setOffsets(const std::vector<Nimble::Vector3f> & offsets)
  {
    if(offsets.size() == m_offsets->size()) {
      *m_offsets = offsets;
      setChanged();
    }
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
    eventSend("changed");
  }

  // Change every value if given channel by v
  void ColorCorrection::changeUniform(int channel, float v)
  {
    for (int i=0; i < 256; ++i) {
      (*m_offsets)[i][channel] = Nimble::Math::Clamp((*m_offsets)[i][channel] + v, 0.0f, 1.0f);
    }
    setChanged();
  }

  void ColorCorrection::encode(Radiant::BinaryData & bd) const
  {
    const std::vector<Nimble::Vector3> & offsets_tmp = offsets();
    std::vector<float> offsets(offsets_tmp.size() * 3);
    for(int i = 0; i < offsets_tmp.size(); ++i) {
      offsets[i*3] = offsets_tmp[i].x;
      offsets[i*3+1] = offsets_tmp[i].y;
      offsets[i*3+2] = offsets_tmp[i].z;
    }
    bd.writeVector3Float32(gamma());
    bd.writeVector3Float32(contrast());
    bd.writeVector3Float32(brightness());
    bd.writeBlob(&offsets[0], offsets.size() * sizeof(offsets[0]));
  }

  bool ColorCorrection::decode(Radiant::BinaryData & bd)
  {
    Nimble::Vector3f gamma = bd.readVector3Float32();
    Nimble::Vector3f contrast = bd.readVector3Float32();
    Nimble::Vector3f brightness = bd.readVector3Float32();
    std::vector<float> offsets(256 * 3);
    if(bd.readBlob(&offsets[0], offsets.size() * sizeof(offsets[0]))) {
      std::vector<Nimble::Vector3f> offsetsv(256);
      for(int i = 0; i < offsets.size()/3; ++i)
        offsetsv[i].make(offsets[i*3], offsets[i*3+1], offsets[i*3+2]);
      setGamma(gamma);
      setContrast(contrast);
      setBrightness(brightness);
      setOffsets(offsetsv);
      return true;
    }
    return false;
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
    std::vector<Nimble::Vector3T<uint8_t> > tmp(256);
    fillAsBytes(&tmp[0]);
    bool identity = true;
    for(int i = 0; i < 256; ++i) {
      if(tmp[i].x != i || tmp[i].y != i || tmp[i].z != i) {
        identity = false;
        break;
      }
    }
    if(m_identity != identity) {
      m_identity = identity;
      // eventSend(...);
    }
    if(tmp != m_prev) {
      m_prev = tmp;
      eventSend("changed");
    }
  }

}
