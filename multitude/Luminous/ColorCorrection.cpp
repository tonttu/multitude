#include "ColorCorrection.hpp"

#include <algorithm>

#include <Valuable/AttributeSpline.hpp>


namespace
{

  static float applyModifiers(float x, float y, float contrast, float gamma, float brightness)
  {
    y += (x - .5f) * (contrast - 1.f);
    return std::pow(y, gamma) + brightness;
  }

  static float invertModifiers(float x, float y, float contrast, float gamma, float brightness)
  {
    y = std::pow(y - brightness, 1.f / gamma);
    return y - (x - .5f) * (contrast - 1.f);
  }

}

namespace Luminous
{

  class ColorCorrection::D
  {
  public:
    D(ColorCorrection * host)
      : m_gamma(host, "gamma", Nimble::Vector3(1.0f, 1.0f, 1.0f))
      , m_contrast(host, "contrast", Nimble::Vector3(1.0f, 1.0f, 1.0f))
      , m_brightness(host, "brightness", Nimble::Vector3(0.0f, 0.0f, 0.0f))
      , m_identity(true)
      , m_rgbCached(false)
    {
      m_splines.push_back(std::unique_ptr<Valuable::AttributeSpline>(
                            new Valuable::AttributeSpline(host, "spline0")));
      m_splines.push_back(std::unique_ptr<Valuable::AttributeSpline>(
                            new Valuable::AttributeSpline(host, "spline1")));
      m_splines.push_back(std::unique_ptr<Valuable::AttributeSpline>(
                            new Valuable::AttributeSpline(host, "spline2")));
    }

  public:
    std::vector<std::unique_ptr<Valuable::AttributeSpline> > m_splines;
    Valuable::AttributeVector3f m_gamma;
    Valuable::AttributeVector3f m_contrast;
    Valuable::AttributeVector3f m_brightness;

    bool m_identity;
    std::vector<Nimble::Vector2f> m_prev[3];

    bool m_rgbCached;
    Luminous::RGBCube m_rgbCube;
  };


  ColorCorrection::ColorCorrection(Node * parent, const QByteArray & name)
    : Valuable::Node(parent, name),
      m_d(new D(this))
  {
    eventAddOut("changed");

    for (int c = 0; c < 3; ++c)
      m_d->m_splines[c]->fixEdges();

    setIdentity();
    eventSend("changed");
    m_d->m_gamma.addListener(std::bind(&ColorCorrection::changed, this));
    m_d->m_contrast.addListener(std::bind(&ColorCorrection::changed, this));
    m_d->m_brightness.addListener(std::bind(&ColorCorrection::changed, this));

    for(auto & colorSpline : m_d->m_splines) {
      colorSpline->addListener(std::bind(&ColorCorrection::changed, this));
    }
  }

  ColorCorrection::~ColorCorrection()
  {
    delete m_d;
  }

  int ColorCorrection::nearestControlPoint(float x, int channel, bool modifiers, Nimble::Vector2f & controlPointOut) const
  {
    int idx = m_d->m_splines[channel]->nearestControlPoint(x, controlPointOut);

    if (modifiers)
      controlPointOut.y = applyModifiers(x, controlPointOut.y, m_d->m_contrast[channel], m_d->m_gamma[channel], m_d->m_brightness[channel]);

    return idx;
  }

  int ColorCorrection::addControlPoint(float x, float y, int channel, bool modifiers)
  {
    assert(x >= 0.f && x <= 1.f);
    assert(y >= 0.f && y <= 1.f);

    if (modifiers)
      y = invertModifiers(x, y, m_d->m_contrast[channel], m_d->m_gamma[channel], m_d->m_brightness[channel]);
    int index = m_d->m_splines[channel]->insert(x, y);
    return index;
  }

  void ColorCorrection::removeControlPoint(int index, int channel)
  {
    m_d->m_splines[channel]->removeControlPoint(index);
  }

  const std::vector<Nimble::Vector2f> & ColorCorrection::controlPoints(int channel) const
  {
    return m_d->m_splines[channel]->points();
  }

  std::vector<Nimble::Vector2f> ColorCorrection::controlPoints(int channel, bool modifiers) const
  {
    std::vector<Nimble::Vector2f> points = m_d->m_splines[channel]->points();
    if (modifiers) {
      for (size_t i = 0; i < points.size(); ++i) {
        points[i].y = applyModifiers(points[i].x, points[i].y, m_d->m_contrast[channel], m_d->m_gamma[channel], m_d->m_brightness[channel]);
      }
    }
    return points;
  }

  Nimble::Vector3 ColorCorrection::controlPoint(size_t index) const
  {
    Nimble::Vector3 result;

    for(int i = 0; i < 3; i++)
      result[i] = m_d->m_splines[i]->points().at(index)[1];

    return result;
  }

  void ColorCorrection::setControlPoint(size_t index, const Nimble::Vector3 &rgbvalue)
  {
    for(int c = 0; c < 3; c++) {
      std::vector<Nimble::Vector2f> points = m_d->m_splines[c]->points();
      points.at(index).y = rgbvalue[c];
      m_d->m_splines[c]->setPoints(points);
    }
  }

  void ColorCorrection::multiplyRGBValues(float mul, bool clamp)
  {
    for(int c = 0; c < 3; c++) {
      std::vector<Nimble::Vector2f> points = m_d->m_splines[c]->points();
      for(size_t i = 0; i < points.size(); i++) {
        Nimble::Vector2f & p = points[i];
        p.y *= mul;
        if(clamp)
          p.y = Nimble::Math::Clamp(p.y, 0.0f, 1.0f);
      }
      m_d->m_splines[c]->setPoints(points);
    }
  }

  float ColorCorrection::value(float x, int channel, bool clamp, bool modifiers) const
  {
    float y;
    if (modifiers) {
      y = value(x, channel, false, false);
      y = applyModifiers(x, y, m_d->m_contrast[channel], m_d->m_gamma[channel], m_d->m_brightness[channel]);
    } else {
      y = m_d->m_splines[channel]->value(x);
    }
    return clamp ? Nimble::Math::Clamp(y, 0.f, 1.f) : y;
  }

  Nimble::Vector3f ColorCorrection::value(float x) const
  {
    return Nimble::Vector3f(value(x, 0, true, true),
                            value(x, 1, true, true),
                            value(x, 2, true, true));
  }

  Nimble::Vector3f ColorCorrection::valueRGB(float x, bool clamp, bool modifiers) const
  {
    return Nimble::Vector3f(value(x, 0, clamp, modifiers),
                            value(x, 1, clamp, modifiers),
                            value(x, 2, clamp, modifiers));
  }

  bool ColorCorrection::isIdentity() const
  {
    return m_d->m_identity;
  }

  void ColorCorrection::setIdentity()
  {
    for (int c = 0; c < 3; ++c) {
      m_d->m_splines[c]->clear();
      m_d->m_splines[c]->insert(0, 0);
      m_d->m_splines[c]->insert(1, 1);
    }
    m_d->m_gamma = Nimble::Vector3f(1.f, 1.f, 1.f);
    m_d->m_contrast = Nimble::Vector3(1.f, 1.f, 1.f);
    m_d->m_brightness = Nimble::Vector3(0.f, 0.f, 0.f);
  }

  void Luminous::ColorCorrection::setIdentity(const std::vector<float> & points)
  {
    for (int c = 0; c < 3; ++c) {
      m_d->m_splines[c]->clear();
      for(size_t j = 0; j < points.size(); j++) {
        float v = points[j];
        m_d->m_splines[c]->insert(v, v);
      }

    }
    m_d->m_gamma = Nimble::Vector3f(1.f, 1.f, 1.f);
    m_d->m_contrast = Nimble::Vector3(1.f, 1.f, 1.f);
    m_d->m_brightness = Nimble::Vector3(0.f, 0.f, 0.f);
  }

  // Change every value of given channel by v
  void ColorCorrection::changeUniform(int channel, float v)
  {
    m_d->m_splines[channel]->changeUniform(v);
  }

  void ColorCorrection::encode(Radiant::BinaryData & bd) const
  {
    bd.writeVector3Float32(gamma());
    bd.writeVector3Float32(contrast());
    bd.writeVector3Float32(brightness());
    for (int c = 0; c < 3; ++c) {
      const std::vector<Nimble::Vector2f> & tmp = m_d->m_splines[c]->points();
      bd.writeInt32(tmp.size());
      bd.writeBlob(tmp.empty() ? 0 : &tmp[0], int(tmp.size() * sizeof(tmp[0])));
    }
  }

  bool ColorCorrection::decode(Radiant::BinaryData & bd)
  {
    Nimble::Vector3f gamma = bd.readVector3Float32();
    Nimble::Vector3f contrast = bd.readVector3Float32();
    Nimble::Vector3f brightness = bd.readVector3Float32();
    std::vector<Nimble::Vector2f> points[3];
    for (int c = 0; c < 3; ++c) {
      int pointCount = bd.readInt32();
      points[c].resize(pointCount);
      if (!bd.readBlob(pointCount > 0 ? &points[c][0] : 0, int(pointCount * sizeof(points[c][0])))) {
        Radiant::warning("ColorCorrection::decode # read error");
        return false;
      }
    }

    for (int c = 0; c < 3; ++c)
      m_d->m_splines[c]->setPoints(points[c]);

    setGamma(gamma);
    setContrast(contrast);
    setBrightness(brightness);
    return true;
  }

  void ColorCorrection::fill(std::vector<Nimble::Vector3ub> & to) const
  {
    if (to.empty())
      return;

    float div = 1.0f / to.size();
    for (int i = 0; i < int(to.size()); ++i) {
      for (int c = 0; c < 3; ++c) {
        const float v = value(i * div, c, true, true);
        to[i][c] = Nimble::Math::Round(v * 255.0f);
      }
    }
  }

  Nimble::Vector3f ColorCorrection::gamma() const
  {
    return m_d->m_gamma;
  }

  void ColorCorrection::setGamma(const Nimble::Vector3f & gamma)
  {
    m_d->m_gamma = gamma;
  }

  Nimble::Vector3f ColorCorrection::contrast() const
  {
    return m_d->m_contrast;
  }

  void ColorCorrection::setContrast(const Nimble::Vector3f & contrast)
  {
    m_d->m_contrast = contrast;
  }

  Nimble::Vector3f ColorCorrection::brightness() const
  {
    return m_d->m_brightness;
  }

  void ColorCorrection::setBrightness(const Nimble::Vector3f & brightness)
  {
    m_d->m_brightness = brightness;
  }

//  Valuable::ArchiveElement ColorCorrection::serialize(Valuable::Archive & archive) const
//  {
//    Valuable::ArchiveElement element = Node::serialize(archive);
//    Valuable::ArchiveElement red = archive.createElement("red");
//    Valuable::ArchiveElement green = archive.createElement("green");
//    Valuable::ArchiveElement blue = archive.createElement("blue");
//    red.set(m_d->m_splines[0].serialize());
//    green.set(m_d->m_splines[1].serialize());
//    blue.set(m_d->m_splines[2].serialize());
//    element.add(red);
//    element.add(green);
//    element.add(blue);
//    return element;
//  }

  bool ColorCorrection::deserialize(const Valuable::ArchiveElement & element)
  {
    bool b = Node::deserialize(element);
    for (int c = 0; c < 3; ++c)
      m_d->m_splines[c]->fixEdges();
    return b;
  }

  bool ColorCorrection::readElement(const Valuable::ArchiveElement &element)
  {
    // No warnings for obsolete attributes
    QByteArray name = element.name().toUtf8();
    if(name == "red" || name == "green" || name == "blue")
      return true;
    return false;
  }

//  bool ColorCorrection::readElement(const Valuable::ArchiveElement & element)
//  {
//    if (element.name() == "offsets") {
//      Valuable::AttributeContainer< std::vector<Nimble::Vector3f> > offsets;
//      if (offsets.deserialize(element)) {
//        if (offsets->size() == 256) {
//          for (int c = 0; c < 3; ++c)
//            m_d->m_splines[c].clear();
//          for (int i = 0; i < 256; i += 15 /* old DIVISOR value */)
//          {
//            for (int c = 0; c < 3; ++c)
//              addControlPoint(i/255.0f, i/255.0f + offsets->at(i)[c], c, false);
//          }
//        }
//        for (int c = 0; c < 3; ++c)
//          m_d->m_splines[c].fixEdges();
//        if (m_d->m_identity) {
//          for (int c = 0; c < 3; ++c) {
//            m_d->m_splines[c].clear();
//            m_d->m_splines[c].insert(0, 0);
//            m_d->m_splines[c].insert(1, 1);
//          }
//        }
//        return true;
//      }
//    } else if (element.name() == "red") {
//      return m_d->m_splines[0].deserialize(element.get().toUtf8());
//    } else if (element.name() == "green") {
//      return m_d->m_splines[1].deserialize(element.get().toUtf8());
//    } else if (element.name() == "blue") {
//      return m_d->m_splines[2].deserialize(element.get().toUtf8());
//    }

//    return false;
//  }

  const RGBCube & ColorCorrection::asRGBCube() const
  {
    if(!m_d->m_rgbCached) {
      m_d->m_rgbCube.fromColorSplines(*this);
      m_d->m_rgbCached = true;
    }

    return m_d->m_rgbCube;
  }

  void ColorCorrection::changed()
  {
    eventSend("changed");
  }

}
