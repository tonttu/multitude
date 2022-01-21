#include "BezierSplineTessellator.hpp"
#include "CubicBezierCurve.hpp"

namespace Luminous
{
  class BezierSplineTessellator::D
  {
  public:
    D(std::vector<BezierSplineTessellator::Vertex> & vertices);
    /// In an arc of angle radians and radius of strokeRadius, the max error
    /// between a perfect arc and a polyline with roundCapSegments segments
    /// is at most m_maxRoundCapError
    inline int roundCapSegments(float strokeRadius, float angle = Nimble::Math::PI) const;
    inline float capSegmentAngle(float strokeRadius) const;
    /// Optimized version of std::cos(capSegmentAngle(strokeWidth))
    inline float capSegmentAngleCos(float strokeRadius) const;
    void renderRoundCapBegin(CubicBezierCurve::PolylinePoint p, Nimble::Vector2f normal, BezierSplineTessellator::Vertex v);
    void renderRoundCapEnd(CubicBezierCurve::PolylinePoint p, Nimble::Vector2f normal, BezierSplineTessellator::Vertex v);

  public:
    std::vector<BezierSplineTessellator::Vertex> & m_vertices;
    float m_maxCurveError;
    float m_maxRoundCapError;

    // Cached to avoid extra memory allocations
    std::vector<CubicBezierCurve::PolylinePoint> m_polylineBuffer;
  };

  BezierSplineTessellator::D::D(std::vector<BezierSplineTessellator::Vertex> & vertices)
    : m_vertices(vertices)
  {
  }

  int BezierSplineTessellator::D::roundCapSegments(float strokeRadius, float angle) const
  {
    return 1 + angle / capSegmentAngle(strokeRadius);
  }

  float BezierSplineTessellator::D::capSegmentAngle(float strokeRadius) const
  {
    return 2.f * std::acos(1.f - m_maxRoundCapError / strokeRadius);
  }

  float BezierSplineTessellator::D::capSegmentAngleCos(float strokeRadius) const
  {
    float a = 1.f - m_maxRoundCapError / strokeRadius;
    return 2.f * a * a - 1.f;
  }

  void BezierSplineTessellator::D::renderRoundCapBegin(
      CubicBezierCurve::PolylinePoint p, Nimble::Vector2f normal, Vertex v)
  {
    int segments = roundCapSegments(p.point.z);

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
      v.location = p.point.vector2() + dir0;
      m_vertices.push_back(v);

      if (++segment == segments)
        break;

      dir1.rotate(-s, c);

      v.location = p.point.vector2() + dir1;
      m_vertices.push_back(v);

      if (++segment == segments)
        break;

      dir0.rotate(s, c);
    }
  }

  /// @todo we should take the .z tangent into account here and scale
  /// the round cap begin/end accordingly
  void BezierSplineTessellator::D::renderRoundCapEnd(
      CubicBezierCurve::PolylinePoint p, Nimble::Vector2f normal, BezierSplineTessellator::Vertex v)
  {
    int segments = roundCapSegments(p.point.z);

    if (segments <= 1)
      return;

    float angle = (float)Nimble::Math::PI / segments;
    float s = std::sin(angle), c = std::cos(angle);

    Nimble::Vector2f dir0 = -normal;
    Nimble::Vector2f dir1 = normal;

    for (int segment = 1;;) {
      dir0.rotate(s, c);

      v.location = p.point.vector2() + dir0;
      m_vertices.push_back(v);

      if (++segment == segments)
        break;

      dir1.rotate(-s, c);

      v.location = p.point.vector2() + dir1;
      m_vertices.push_back(v);

      if (++segment == segments)
        break;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  BezierSplineTessellator::BezierSplineTessellator(std::vector<Vertex> & vertices, float maxCurveError, float maxRoundCapError)
    : m_d(new D(vertices))
  {
    m_d->m_maxCurveError = maxCurveError;
    m_d->m_maxRoundCapError = maxRoundCapError;
  }

  BezierSplineTessellator::~BezierSplineTessellator()
  {}

  void BezierSplineTessellator::tessellate(const BezierSpline & nodes, const Radiant::ColorPMA & color, SplineStyle style)
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

    // Initialize this to keep gcc happy. -Werror=maybe-uninitialized generates
    // a false positive later in the function.
    Nimble::Vector3f prevPoint{0.f, 0.f, 0.f};
    Nimble::Vector2f prevUnitTangent;
    bool first = true;
    // 32-bit float is not accurate enough for smaller values
    float maxCurveError = std::max(m_d->m_maxCurveError, 0.0001f);
    for (const BezierNode * inputIt = nodes.data(), * inputLast = inputIt + nodes.size() - 1; inputIt != inputLast; ++inputIt) {
      polylineBuffer.clear();
      CubicBezierCurve curve(inputIt[0], inputIt[1]);
      if (first)
        polylineBuffer.push_back({inputIt->point, curve.tangent2D(0.f)});

      float cc = m_d->capSegmentAngleCos(inputIt[0].point.z);
      if (cc < -1.f || cc > 1.f)
        cc = -1.f;

      union {
        float f;
        uint32_t i;
      };
      static_assert(sizeof(f) == sizeof(i));
      float maxValue = std::max({std::abs(curve[0].x), std::abs(curve[0].y),
                                 std::abs(curve[1].x), std::abs(curve[1].y),
                                 std::abs(curve[2].x), std::abs(curve[2].y),
                                 std::abs(curve[3].x), std::abs(curve[3].y)});
      f = maxValue;
      i += 5;
      float floatDiff = f - maxValue;
      float err = std::max(floatDiff, maxCurveError);
      curve.evaluate2D(polylineBuffer, err*err, cc, curve.tangent2D(0.f));

      for (size_t idx = 0, s = polylineBuffer.size(); idx < s; ++idx) {
        p = polylineBuffer[idx];
        float len = p.tangent2D.length();
        Nimble::Vector2f unitTangent;

        if (len <= std::numeric_limits<float>::epsilon()) {
          unitTangent = curve[3].vector2() - curve[0].vector2();
          len = unitTangent.length();
          if (!first && len <= std::numeric_limits<float>::epsilon()) {
            unitTangent = prevUnitTangent;
          } else {
            unitTangent /= len;
          }
        } else {
          unitTangent = p.tangent2D / len;
        }

        normal = unitTangent.perpendicular() * p.point.z;

        if (first) {
          if (style.capBegin == CAP_ROUND)
            m_d->renderRoundCapBegin(p, normal, v);
        } else {
          float s = m_d->capSegmentAngleCos(p.point.z);
          float angleCos = dot(unitTangent, prevUnitTangent);
          /// The spline might not have c1 continuity, so we detect
          /// sharp turns and render a round join at the previous point.
          if (angleCos < s && s >= -1.f && s < 1.f) {
            float angle = std::acos(angleCos);
            if (std::isfinite(angle)) {
              int steps = angle / std::acos(s);

              bool left = cross(prevUnitTangent, unitTangent) > 0.f;
              Nimble::Vector2f normal2 = prevUnitTangent.perpendicular() * prevPoint.z;
              angle = angle / (steps + 1) * (left ? 1 : -1);

              float s = std::sin(angle), c = std::cos(angle);
              for (int i = 0; i < steps; ++i) {
                normal2.rotate(s, c);

                v.location = prevPoint.vector2() - normal2;
                out.push_back(v);

                v.location = prevPoint.vector2() + normal2;
                out.push_back(v);
              }
            }
          }
        }

        first = false;
        prevPoint = p.point;
        prevUnitTangent = unitTangent;

        v.location = p.point.vector2() - normal;
        out.push_back(v);

        v.location = p.point.vector2() + normal;
        out.push_back(v);
      }
    }

    if (style.capEnd == CAP_ROUND)
      m_d->renderRoundCapEnd(p, normal, v);
  }
}
