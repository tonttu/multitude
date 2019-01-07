#include "BezierSplineTesselator.hpp"
#include "CubicBezierCurve.hpp"

namespace Luminous
{
  class BezierSplineTesselator::D
  {
  public:
    D(std::vector<BezierSplineTesselator::Vertex> & vertices);
    /// In an arc of angle radians and radius of strokeWidth/2, the max error
    /// between a perfect arc and a polyline with roundCapSegments segments
    /// is at most m_maxRoundCapError
    inline int roundCapSegments(float strokeWidth, float angle = Nimble::Math::PI) const;
    inline float capSegmentAngle(float strokeWidth) const;
    /// Optimized version of std::cos(capSegmentAngle(strokeWidth))
    inline float capSegmentAngleCos(float strokeWidth) const;
    void renderCapBegin(CubicBezierCurve::PolylinePoint p, Nimble::Vector2f normal, BezierSplineTesselator::Vertex v);
    void renderCapEnd(CubicBezierCurve::PolylinePoint p, Nimble::Vector2f normal, BezierSplineTesselator::Vertex v);

  public:
    std::vector<BezierSplineTesselator::Vertex> & m_vertices;
    float m_maxCurveError;
    float m_maxRoundCapError;

    // Cached to avoid extra memory allocations
    std::vector<CubicBezierCurve::PolylinePoint> m_polylineBuffer;
  };

  BezierSplineTesselator::D::D(std::vector<BezierSplineTesselator::Vertex> & vertices)
    : m_vertices(vertices)
  {
  }

  int BezierSplineTesselator::D::roundCapSegments(float strokeWidth, float angle) const
  {
    return 1 + angle / capSegmentAngle(strokeWidth);
  }

  float BezierSplineTesselator::D::capSegmentAngle(float strokeWidth) const
  {
    return 2.f * std::acos(1.f - m_maxRoundCapError / (strokeWidth * 0.5f));
  }

  float BezierSplineTesselator::D::capSegmentAngleCos(float strokeWidth) const
  {
    float a = 1.f - m_maxRoundCapError / (strokeWidth * 0.5f);
    return 2.f * a * a - 1.f;
  }

  void BezierSplineTesselator::D::renderCapBegin(
      CubicBezierCurve::PolylinePoint p, Nimble::Vector2f normal, Vertex v)
  {
    int segments = roundCapSegments(p.width);

    if (segments <= 1)
      return;

    float angle = (float)Nimble::Math::PI / segments;
    if (segments % 2 == 0)
      angle = -angle;
    float s = std::sin(angle), c = std::cos(angle);

    Nimble::Vector2f dir0 = normal;
    dir0.rotate(float((segments + 1) / 2) / segments * (float)Nimble::Math::PI);
    Nimble::Vector2f dir1 = dir0;

    for (int segment = 1;;) {
      v.location = p.point + dir0;
      m_vertices.push_back(v);

      if (++segment == segments)
        break;

      dir1.rotate(-s, c);

      v.location = p.point + dir1;
      m_vertices.push_back(v);

      if (++segment == segments)
        break;

      dir0.rotate(s, c);
    }
  }

  void BezierSplineTesselator::D::renderCapEnd(
      CubicBezierCurve::PolylinePoint p, Nimble::Vector2f normal, BezierSplineTesselator::Vertex v)
  {
    int segments = roundCapSegments(p.width);

    if (segments <= 1)
      return;

    float angle = (float)Nimble::Math::PI / segments;
    float s = std::sin(angle), c = std::cos(angle);

    Nimble::Vector2f dir0 = -normal;
    Nimble::Vector2f dir1 = normal;

    for (int segment = 1;;) {
      dir0.rotate(s, c);

      v.location = p.point + dir0;
      m_vertices.push_back(v);

      if (++segment == segments)
        break;

      dir1.rotate(-s, c);

      v.location = p.point + dir1;
      m_vertices.push_back(v);

      if (++segment == segments)
        break;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  BezierSplineTesselator::BezierSplineTesselator(std::vector<Vertex> & vertices, float maxCurveError, float maxRoundCapError)
    : m_d(new D(vertices))
  {
    m_d->m_maxCurveError = maxCurveError;
    m_d->m_maxRoundCapError = maxRoundCapError;
  }

  BezierSplineTesselator::~BezierSplineTesselator()
  {}

  void BezierSplineTesselator::tesselate(const std::vector<BezierNode> & nodes, const Radiant::ColorPMA & color)
  {
    auto & out = m_d->m_vertices;
    /// @todo this could be incremental, only the last two nodes have changed in
    /// the common case
    out.clear();

    /// @todo draw a circle if nodes.size == 1?
    if (nodes.size() <= 1)
      return;

    auto & polylineBuffer = m_d->m_polylineBuffer;

    CubicBezierCurve::PolylinePoint p;
    Nimble::Vector2f normal;

    Vertex v;
    v.color = color.toVector();

    Nimble::Vector2f prevUnitTangent;
    bool first = true;
    // 32-bit float is not accurate enough for smaller values
    float maxCurveError = std::max(m_d->m_maxCurveError, 0.0001f);
    for (const BezierNode * inputIt = nodes.data(), * inputLast = inputIt + nodes.size() - 1; inputIt != inputLast; ++inputIt) {
      polylineBuffer.clear();
      CubicBezierCurve curve(inputIt[0], inputIt[1]);
      if (first)
        polylineBuffer.push_back({inputIt->point, curve.tangent(0.f), inputIt->strokeWidth});

      float cc = m_d->capSegmentAngleCos(inputIt[0].strokeWidth);
      if (cc < -1.f || cc > 1.f)
        cc = -1.f;

      union {
        float f;
        uint32_t i;
      };
      float maxValue = std::max({std::abs(curve[0].x), std::abs(curve[0].y),
                                 std::abs(curve[1].x), std::abs(curve[1].y),
                                 std::abs(curve[2].x), std::abs(curve[2].y),
                                 std::abs(curve[3].x), std::abs(curve[3].y)});
      f = maxValue;
      i += 5;
      float floatDiff = f - maxValue;
      curve.evaluate(polylineBuffer, std::max(floatDiff, maxCurveError), cc,
          inputIt[0].strokeWidth, inputIt[1].strokeWidth,
          curve.tangent(0.f));

      for (size_t idx = 0, s = polylineBuffer.size(); idx < s; ++idx) {
        p = polylineBuffer[idx];
        float len = p.tangent.length();
        Nimble::Vector2f unitTangent;

        if (first) {
          if (len <= std::numeric_limits<float>::epsilon())
            unitTangent = (polylineBuffer[1].point -
                polylineBuffer[0].point).normalized();
          else
            unitTangent = p.tangent / len;
          normal = unitTangent.perpendicular() * (p.width * 0.5f);

          m_d->renderCapBegin(p, normal, v);
        } else if (len > std::numeric_limits<float>::epsilon()) {
          unitTangent = p.tangent / len;
          normal = unitTangent.perpendicular() * (p.width * 0.5f);
        } else {
          unitTangent = prevUnitTangent;
        }

        if (!first) {
          float s = m_d->capSegmentAngleCos(p.width);
          float angleCos = dot(unitTangent, prevUnitTangent);
          /// The spline might not have c1 continuity, so we detect
          /// sharp turns and render a round join here.
          if (angleCos < s && s >= -1.f && s < 1.f) {
            float angle = std::acos(angleCos);
            if (std::isfinite(angle)) {
              int steps = angle / std::acos(s);

              bool left = cross(prevUnitTangent, unitTangent) > 0.f;
              Nimble::Vector2f normal2 = prevUnitTangent.perpendicular() * (p.width * 0.5f);
              angle = angle / (steps + 1) * (left ? 1 : -1);

              float s = std::sin(angle), c = std::cos(angle);
              for (int i = 0; i < steps; ++i) {
                normal2.rotate(s, c);

                v.location = p.point - normal2;
                out.push_back(v);

                v.location = p.point + normal2;
                out.push_back(v);
              }
            }
          }
        }

        first = false;
        prevUnitTangent = unitTangent;

        v.location = p.point - normal;
        out.push_back(v);

        v.location = p.point + normal;
        out.push_back(v);
      }
    }

    m_d->renderCapEnd(p, normal, v);
  }
}
