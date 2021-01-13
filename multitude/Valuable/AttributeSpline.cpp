#include "AttributeSpline.hpp"

#include <QTextStream>


namespace
{
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

}


namespace Valuable
{

  AttributeSpline::AttributeSpline(Valuable::Node *host, const QByteArray &name)
    : Attribute(host, name)
    , m_isChanged(false)
  {
  }

  void AttributeSpline::copyValueFromLayer(Layer from, Layer to)
  {
    if(from == USER && to == DEFAULT) {
      setAsDefaults();
      return;
    } else if( !(from == DEFAULT && to == USER) ) {
      Radiant::error("ColorSpline::copyValuesFromLayer can only copy values from default "
                     "layer to user layer.");
      return;
    }
    m_points = m_defaultPoints;
    m_prevPoints = m_defaultPoints;
    update(false); /// !avoid comparison
    changed();
    m_isChanged = false;
  }

  bool AttributeSpline::deserialize(const Valuable::ArchiveElement& element)
  {
    clear();
    for(ArchiveElement::Iterator it = element.children(); it; ++it) {
      m_points.push_back(Serializer::deserialize<Nimble::Vector2f>(*it));
    }
    update(true);
    return true;
  }

  Valuable::ArchiveElement AttributeSpline::serialize(Valuable::Archive& doc) const
  {
    QString elementName = name().isEmpty() ? "AttributeSpline" : name();

    ArchiveElement elem = doc.createElement(elementName);
    for(auto it = m_points.begin(); it != m_points.end(); ++it) {
      elem.add(Serializer::serialize(doc, *it));
    }
    return elem;
  }

  QByteArray AttributeSpline::type() const
  {
    return "spline";
  }

  void AttributeSpline::setAsDefaults()
  {
    m_defaultPoints = m_points;
    m_prevPoints = m_points;
    update(false); /// !avoid comparison
    m_isChanged = false;
  }

  bool AttributeSpline::isChanged() const
  {
    return m_isChanged;
  }

  void AttributeSpline::clear()
  {
    m_points.clear();
    m_intermediatePoints.clear();
    update(true);
  }

  int AttributeSpline::insert(float x, float y)
  {
    const std::vector<Nimble::Vector2>::iterator high =
        std::upper_bound(m_points.begin(), m_points.end(), x, &cmpx);
    int idx = static_cast<int>(high - m_points.begin());
    m_points.insert(high, Nimble::Vector2f(x, y));
    update(true);
    return idx;
  }

  void AttributeSpline::changeUniform(float v)
  {
    for (size_t i = 0, s = m_points.size(); i < s; ++i)
      m_points[i].y += v;
    for (size_t i = 0, s = m_intermediatePoints.size(); i < s; ++i)
      m_intermediatePoints[i].y += v;

    update(v == 0.f);
  }

  bool AttributeSpline::isIdentity() const
  {
    for (size_t i = 0, s = m_points.size(); i < s; ++i)
      if (!qFuzzyCompare(m_points[i].x, m_points[i].y))
        return false;
    return true;
  }

  int AttributeSpline::nearestControlPoint(float x, Nimble::Vector2f & controlPointOut) const
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
      return static_cast<int>(m_points.size()) - 1;
    }

    if (high == m_points.begin()) {
      controlPointOut = m_points.front();
      return 0;
    }

    const int lowidx = static_cast<int>(high - m_points.begin() - 1);
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

  float AttributeSpline::value(float x) const
  {
    if (m_points.size() < 2)
      return x;

    const std::vector<Nimble::Vector2>::const_iterator high =
        std::upper_bound(m_points.begin(), m_points.end(), x, &cmpx);

    if (high == m_points.end())
      return m_points.back().y;

    if (high == m_points.begin())
      return m_points.front().y;

    const int lowidx = static_cast<int>(high - m_points.begin() - 1);
    const Nimble::Vector2f p0 = *(high - 1);
    const Nimble::Vector2f p1 = m_intermediatePoints[lowidx*2+1];
    const Nimble::Vector2f p2 = m_intermediatePoints[lowidx*2+2];
    const Nimble::Vector2f p3 = *high;

    const float t = solveT(x, p0.x, p1.x, p2.x, p3.x);
    return evalBezier(t, p0, p1, p2, p3).y;
  }

  void AttributeSpline::setPoints(std::vector<Nimble::Vector2f> & points)
  {
    m_points = points;
    update();
  }

  void AttributeSpline::removeControlPoint(int index)
  {
    m_points.erase(m_points.begin() + index);
    update(true);
  }

  QByteArray AttributeSpline::serialize() const
  {
    QByteArray out;
    QTextStream stream(&out);
    stream.setRealNumberPrecision(3);
    for (size_t i = 0; i < m_points.size(); ++i)
      stream << m_points[i].x << " " << m_points[i].y << " ";
    stream.flush();
    return out;
  }

  bool AttributeSpline::deserialize(const QByteArray & str)
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

    fixEdges();
    update();
    return true;
  }

  void AttributeSpline::fixEdges()
  {
    if (m_points.size() == 0) {
      insert(0, 0);
      insert(1, 1);
    } else {
      Nimble::Vector2f p;
      nearestControlPoint(0.0f, p);
      if (p.x != 0.0f) {
        if (std::abs(p.x) < 1.0/256.0f)
          p.x = 0;
        else
          insert(0, 0);
      }
      nearestControlPoint(1.0f, p);
      if (p.x != 1.0f) {
        if (std::abs(p.x-1.0f) < 1.0/256.0f)
          p.x = 1;
        else
          insert(1, 1);
      }
    }
  }

  bool AttributeSpline::areDifferent(const std::vector<Nimble::Vector2f> &v1,
                                 const std::vector<Nimble::Vector2f> &v2) const
  {
    if(v1.size() != v2.size()) {
      return true;
    } else {
      bool changed = false;
      for(size_t i = 0; i < v1.size() && !changed; ++i) {
        Nimble::Vector2f p = v1[i];
        Nimble::Vector2f q = v2[i];
        changed = !qFuzzyCompare(p.x, q.x) || !qFuzzyCompare(p.y, q.y);
      }
      return changed;
    }
  }

  void AttributeSpline::update(bool hasChanged)
  {
    m_intermediatePoints.resize(m_points.size()*2);
    // Set the tangent size to 0 to begin and end
    if (m_points.size() > 0) {
      m_intermediatePoints[m_intermediatePoints.size()-2] = m_points.back();
      m_intermediatePoints[m_intermediatePoints.size()-1] = m_points.back();
      m_intermediatePoints[0] = m_points.front();
      m_intermediatePoints[1] = m_points.front();
    }
    for (std::size_t i = 1, s = m_points.size(); i < s; ++i) {
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

    m_isChanged = areDifferent(m_defaultPoints, m_points);

    bool change = hasChanged || areDifferent(m_prevPoints, m_points);
    m_prevPoints = m_points;

    if(change) {
      changed();
    }

  }


}
