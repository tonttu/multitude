#include "ColorCorrection.hpp"

#include <Nimble/Splines.hpp>

#include <QTextStream>

namespace {
  static bool cmpx(float a, const Nimble::Vector2f & b)
  {
    return a < b.x;
  }

  static float evalBezierDerivate(float t, float p0, float p1, float p2, float p3)
  {
    // http://www.wolframalpha.com/input/?i=derivate+%281-t%29%C2%B3a+%2B+3*%281-t%29%C2%B2*t*b+%2B+3*%281-t%29*t%C2%B2*c+%2B+t%C2%B3*d
    return -3.0f * (p0 * (t-1.0f) * (t-1.0f) - p1 * (3.f*t*t - 4.f*t + 1.f) + t * (3.f*p2*t - 2.f*p2 - p3*t));
  }

  template <typename T>
  static T evalBezier(float t, T p0, T p1, T p2, T p3)
  {
    float n = 1.f - t;
    return n*n*n*p0 + 3.f*n*n*t*p1 + 3.f*n*t*t*p2 + t*t*t*p3;
  }

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

  static float solveT(float x, float p0, float p1, float p2, float p3)
  {
    assert(p0 <= p1); assert(p1 <= p2); assert(p2 <= p3);
    assert(p0 <= x); assert(x <= p3);
    const float diff = p3 - p0;

    if (diff < 0.00001f)
      return 0.5f;

    // do first guess with linear interpolation
    float t = (x - p0) / diff;

    for (int i = 0; i < 10; ++i) {
      const float v = evalBezier(t, p0, p1, p2, p3);
      const float error = x - v;
      if (std::abs(error) < 0.0001f)
        break;
      const float d = evalBezierDerivate(t, p0, p1, p2, p3);
      t = Nimble::Math::Clamp(t + 0.9f * error / d, 0.f, 1.f);
    }

    return t;
  }

  class ColorSpline
  {
  public:
    void clear();
    int insert(float x, float y);
    void changeUniform(float v);

    bool isIdentity() const;
    int nearestControlPoint(float x, Nimble::Vector2f & controlPointOut);
    float value(float x) const;

    const std::vector<Nimble::Vector2f> & points() const { return m_points; }
    const std::vector<Nimble::Vector2f> & intermediatePoints() const { return m_intermediatePoints; }
    void setPoints(std::vector<Nimble::Vector2f> & points);
    void removeControlPoint(int index);

    QByteArray serialize() const;
    bool deserialize(const QByteArray & str);

  private:
    void update();

  private:

    std::vector<Nimble::Vector2f> m_points;
    std::vector<Nimble::Vector2f> m_intermediatePoints;
  };

  void ColorSpline::clear()
  {
    m_points.clear();
    m_intermediatePoints.clear();
  }

  int ColorSpline::insert(float x, float y)
  {
    const std::vector<Nimble::Vector2>::iterator high =
        std::upper_bound(m_points.begin(), m_points.end(), x, &cmpx);
    int idx = high - m_points.begin();
    m_points.insert(high, Nimble::Vector2f(x, y));
    update();
    return idx;
  }

  void ColorSpline::changeUniform(float v)
  {
    for (int i = 0, s = m_points.size(); i < s; ++i)
      m_points[i].y += v;
    for (int i = 0, s = m_intermediatePoints.size(); i < s; ++i)
      m_intermediatePoints[i].y += v;
  }

  bool ColorSpline::isIdentity() const
  {
    for (int i = 0, s = m_points.size(); i < s; ++i)
      if (!qFuzzyCompare(m_points[i].x, m_points[i].y))
        return false;
    return true;
  }

  int ColorSpline::nearestControlPoint(float x, Nimble::Vector2f & controlPointOut)
  {
    if (m_points.empty())
      return -1;

    if (m_points.size() == 1) {
      controlPointOut = m_points[0];
      return 0;
    }

    const std::vector<Nimble::Vector2>::const_iterator high =
        std::upper_bound(m_points.begin(), m_points.end(), x, &cmpx);

    if (high == m_points.end()) {
      controlPointOut = m_points.back();
      return m_points.size() - 1;
    }

    if (high == m_points.begin()) {
      controlPointOut = m_points.front();
      return 0;
    }

    const int lowidx = high - m_points.begin() - 1;
    const Nimble::Vector2f p1 = *(high - 1);
    const Nimble::Vector2f p3 = *high;
    if (x - p1.x < p3.x - x) {
      controlPointOut = p1;
      return lowidx;
    } else {
      controlPointOut = p3;
      return lowidx + 1;
    }
  }

  float ColorSpline::value(float x) const
  {
    if (m_points.size() < 2)
      return x;

    const std::vector<Nimble::Vector2>::const_iterator high =
        std::upper_bound(m_points.begin(), m_points.end(), x, &cmpx);

    if (high == m_points.end())
      return m_points.back().y;

    if (high == m_points.begin())
      return m_points.front().y;

    const int lowidx = high - m_points.begin() - 1;
    const Nimble::Vector2f p0 = *(high - 1);
    const Nimble::Vector2f p1 = m_intermediatePoints[lowidx*2+1];
    const Nimble::Vector2f p2 = m_intermediatePoints[lowidx*2+2];
    const Nimble::Vector2f p3 = *high;

    const float t = solveT(x, p0.x, p1.x, p2.x, p3.x);
    return evalBezier(t, p0, p1, p2, p3).y;
  }

  void ColorSpline::setPoints(std::vector<Nimble::Vector2f> & points)
  {
    m_points = points;
    update();
  }

  void ColorSpline::removeControlPoint(int index)
  {
    m_points.erase(m_points.begin() + index);
    update();
  }

  QByteArray ColorSpline::serialize() const
  {
    QByteArray out;
    QTextStream stream(&out);
    stream.setRealNumberPrecision(3);
    for (int i = 0; i < m_points.size(); ++i)
      stream << m_points[i].x << " " << m_points[i].y << " ";
    return out;
  }

  bool ColorSpline::deserialize(const QByteArray & str)
  {
    clear();
    QTextStream stream(str);

    while (true) {
      float x, y;
      stream >> x >> y;
      if (stream.status() != QTextStream::Ok)
        break;
      m_points.push_back(Nimble::Vector2f(x, y));
    }

    update();
    return true;
  }

  void ColorSpline::update()
  {
    m_intermediatePoints.resize(m_points.size()*2);
    // Set the tangent size to 0 to begin and end
    if (m_points.size() > 0) {
      m_intermediatePoints[m_intermediatePoints.size()-2] = m_points.back();
      m_intermediatePoints[m_intermediatePoints.size()-1] = m_points.back();
      m_intermediatePoints[0] = m_points.front();
      m_intermediatePoints[1] = m_points.front();
    }
    for (int i = 1, s = m_points.size(); i < s; ++i) {
      const Nimble::Vector2f prev = m_points[i-1];
      const Nimble::Vector2f p = m_points[i];
      const Nimble::Vector2f next = i == s - 1 ? m_points.back() : m_points[i+1];

      // basic Catmull-Rom tangent..
      const Nimble::Vector2f tangent = 0.25f * (next - prev);

      // .. but limited so that all control points (including generated ones) are in increasing x-order
      const float f1 = 0.5f * (p.x - prev.x) / tangent.x;
      const float f2 = 0.5f * (next.x - p.x) / tangent.x;

      m_intermediatePoints[i*2] = p - Nimble::Math::Min(f1, 1.0f) * tangent;
      m_intermediatePoints[i*2+1] = p + Nimble::Math::Min(f2, 1.0f) * tangent;

      // Fix floating point rounding error issues, needs to be monotonically increasing because of solveT
      if (i != 0 && m_intermediatePoints[i*2-1].x > m_intermediatePoints[i*2].x)
        m_intermediatePoints[i*2].x = m_intermediatePoints[i*2-1].x;
      if (m_intermediatePoints[i*2].x > m_intermediatePoints[i*2+1].x)
        m_intermediatePoints[i*2+1].x = m_intermediatePoints[i*2-1].x;
    }
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
    {
    }

  public:
    ColorSpline m_splines[3];
    Valuable::AttributeVector3f m_gamma;
    Valuable::AttributeVector3f m_contrast;
    Valuable::AttributeVector3f m_brightness;

    bool m_identity;
    std::vector<Nimble::Vector2f> m_prev[3];
  };


  ColorCorrection::ColorCorrection(Node * parent, const QByteArray & name, bool transit)
    : Valuable::Node(parent, name, transit),
      m_d(new D(this))
  {
    eventAddOut("changed");

    setIdentity();
    eventSend("changed");
    m_d->m_gamma.addListener(std::bind(&ColorCorrection::changed, this));
    m_d->m_contrast.addListener(std::bind(&ColorCorrection::changed, this));
    m_d->m_brightness.addListener(std::bind(&ColorCorrection::changed, this));
  }

  ColorCorrection::~ColorCorrection()
  {
    delete m_d;
  }

  int ColorCorrection::nearestControlPoint(float x, int channel, bool modifiers, Nimble::Vector2f & controlPointOut) const
  {
    int idx = m_d->m_splines[channel].nearestControlPoint(x, controlPointOut);

    if (modifiers)
      controlPointOut.y = applyModifiers(x, controlPointOut.y, m_d->m_contrast[channel], m_d->m_gamma[channel], m_d->m_brightness[channel]);

    return idx;
  }

  int ColorCorrection::addControlPoint(float x, float y, int channel, bool modifiers)
  {
    if (modifiers)
      y = invertModifiers(x, y, m_d->m_contrast[channel], m_d->m_gamma[channel], m_d->m_brightness[channel]);
    int index = m_d->m_splines[channel].insert(x, y);
    checkChanged();
    return index;
  }

  void ColorCorrection::removeControlPoint(int index, int channel)
  {
    m_d->m_splines[channel].removeControlPoint(index);
    checkChanged();
  }

  const std::vector<Nimble::Vector2f> & ColorCorrection::controlPoints(int channel) const
  {
    return m_d->m_splines[channel].points();
  }

  std::vector<Nimble::Vector2f> ColorCorrection::controlPoints(int channel, bool modifiers) const
  {
    std::vector<Nimble::Vector2f> points = m_d->m_splines[channel].points();
    if (modifiers) {
      for (int i = 0; i < points.size(); ++i) {
        points[i].y = applyModifiers(points[i].x, points[i].y, m_d->m_contrast[channel], m_d->m_gamma[channel], m_d->m_brightness[channel]);
      }
    }
    return points;
  }

  void ColorCorrection::setControlPoint(size_t index, const Nimble::Vector3 &rgbvalue)
  {
    for(int c = 0; c < 3; c++) {
      std::vector<Nimble::Vector2f> points = m_d->m_splines[c].points();
      points.at(index).y = rgbvalue[c];
      m_d->m_splines[c].setPoints(points);
    }
    checkChanged();
  }

  float ColorCorrection::value(float x, int channel, bool clamp, bool modifiers) const
  {
    float y;
    if (modifiers) {
      y = value(x, channel, false, false);
      y = applyModifiers(x, y, m_d->m_contrast[channel], m_d->m_gamma[channel], m_d->m_brightness[channel]);
    } else {
      y = m_d->m_splines[channel].value(x);
    }
    return clamp ? Nimble::Math::Clamp(y, 0.f, 1.f) : y;
  }

  Nimble::Vector3f ColorCorrection::valueRGB(float x) const
  {
    return Nimble::Vector3f(value(x, 0, true, true),
                            value(x, 1, true, true),
                            value(x, 2, true, true));
  }

  bool ColorCorrection::isIdentity() const
  {
    return m_d->m_identity;
  }

  void ColorCorrection::setIdentity()
  {
    for (int c = 0; c < 3; ++c) {
      m_d->m_splines[c].clear();
      m_d->m_splines[c].insert(0, 0);
      m_d->m_splines[c].insert(1, 1);
    }
    m_d->m_gamma = Nimble::Vector3f(1.f, 1.f, 1.f);
    m_d->m_contrast = Nimble::Vector3(1.f, 1.f, 1.f);
    m_d->m_brightness = Nimble::Vector3(0.f, 0.f, 0.f);
    checkChanged();
  }

  void Luminous::ColorCorrection::setIdentity(const std::vector<float> & points)
  {
    for (int c = 0; c < 3; ++c) {
      m_d->m_splines[c].clear();
      for(size_t j = 0; j < points.size(); j++) {
        float v = points[j];
        m_d->m_splines[c].insert(v, v);
      }

    }
    m_d->m_gamma = Nimble::Vector3f(1.f, 1.f, 1.f);
    m_d->m_contrast = Nimble::Vector3(1.f, 1.f, 1.f);
    m_d->m_brightness = Nimble::Vector3(0.f, 0.f, 0.f);
    checkChanged();
  }

  // Change every value if given channel by v
  void ColorCorrection::changeUniform(int channel, float v)
  {
    m_d->m_splines[channel].changeUniform(v);
    checkChanged();
  }

  void ColorCorrection::encode(Radiant::BinaryData & bd) const
  {
    bd.writeVector3Float32(gamma());
    bd.writeVector3Float32(contrast());
    bd.writeVector3Float32(brightness());
    for (int c = 0; c < 3; ++c) {
      const std::vector<Nimble::Vector2f> & tmp = m_d->m_splines[c].points();
      bd.writeInt32(tmp.size());
      bd.writeBlob(&tmp[0], int(tmp.size() * sizeof(tmp[0])));
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
      if (!bd.readBlob(&points[c][0], int(pointCount * sizeof(points[c][0]))))
        return false;
    }

    for (int c = 0; c < 3; ++c)
      m_d->m_splines[c].setPoints(points[c]);

    setGamma(gamma);
    setContrast(contrast);
    setBrightness(brightness);
    checkChanged();
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

  Valuable::ArchiveElement ColorCorrection::serialize(Valuable::Archive & archive) const
  {
    Valuable::ArchiveElement element = Node::serialize(archive);
    Valuable::ArchiveElement red = archive.createElement("red");
    Valuable::ArchiveElement green = archive.createElement("green");
    Valuable::ArchiveElement blue = archive.createElement("blue");
    red.set(m_d->m_splines[0].serialize());
    green.set(m_d->m_splines[1].serialize());
    blue.set(m_d->m_splines[2].serialize());
    element.add(red);
    element.add(green);
    element.add(blue);
    return element;
  }

  bool ColorCorrection::readElement(const Valuable::ArchiveElement & element)
  {
    if (element.name() == "offsets") {
      Valuable::AttributeContainer< std::vector<Nimble::Vector3f> > offsets;
      if (offsets.deserialize(element)) {
        if (offsets->size() == 256) {
          for (int c = 0; c < 3; ++c)
            m_d->m_splines[c].clear();
          for (int i = 0; i < 256; i += 15 /* old DIVISOR value */)
          {
            for (int c = 0; c < 3; ++c)
              addControlPoint(i/255.0f, i/255.0f + offsets->at(i)[c], c, false);
          }
        }
        checkChanged();
        if (m_d->m_identity) {
          for (int c = 0; c < 3; ++c) {
            m_d->m_splines[c].clear();
            m_d->m_splines[c].insert(0, 0);
            m_d->m_splines[c].insert(1, 1);
          }
        }
        return true;
      }
    } else if (element.name() == "red") {
      return m_d->m_splines[0].deserialize(element.get().toUtf8());
    } else if (element.name() == "green") {
      return m_d->m_splines[1].deserialize(element.get().toUtf8());
    } else if (element.name() == "blue") {
      return m_d->m_splines[2].deserialize(element.get().toUtf8());
    }

    return false;
  }

  void ColorCorrection::changed()
  {
    eventSend("changed");
  }

  void ColorCorrection::checkChanged()
  {
    bool identity = m_d->m_splines[0].isIdentity() && m_d->m_splines[1].isIdentity() && m_d->m_splines[2].isIdentity();
    if (m_d->m_identity != identity) {
      m_d->m_identity = identity;
      // eventSend(...);
    }

    const std::vector<Nimble::Vector2f> * tmp[3];
    bool changed = false;
    for (int c = 0; c < 3; ++c) {
      tmp[c] = &m_d->m_splines[c].points();
      if (tmp[c]->size() != m_d->m_prev[c].size())
        changed = true;
    }

    for (int c = 0; !changed && c < 3; ++c) {
      const std::vector<Nimble::Vector2f> & prev = m_d->m_prev[c];
      const std::vector<Nimble::Vector2f> & now = *tmp[c];
      for (int i = 0; !changed && i < int(now.size()); ++i) {
        if (!qFuzzyCompare(now[i].x, prev[i].x) || !qFuzzyCompare(now[i].y, prev[i].y)) {
          changed = true;
          break;
        }
      }
    }

    if (changed) {
      for (int c = 0; c < 3; ++c)
        m_d->m_prev[c] = *tmp[c];
      this->changed();
    }
  }
}
