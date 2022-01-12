#pragma once

#include <Nimble/Circle.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Range.hpp>

#include <array>
#include <vector>

namespace Luminous
{
  /// Single node in a longer continuous Bezier spline. You can generate
  /// a cubic Bezier curve from any two consecutive Bezier nodes by
  /// using these control points:
  /// {first.point, first.ctrlOut, second.ctrlIn, second.point}.
  ///
  /// Bezier spline with N nodes (with float width in every node)
  /// consumes 9N*sizeof(float) bytes memory. The same spline would
  /// consume 12(N-1)*sizeof(float) bytes memory if stored as a vector
  /// of cubic Bezier curves.
  ///
  /// This form already uses less memory when N is at least 5, and with
  /// lots of points it saves 25% memory.
  /// @sa Luminous::BezierSpline
  struct BezierNode
  {
    Nimble::Vector3f ctrlIn;
    Nimble::Vector3f point;
    Nimble::Vector3f ctrlOut;

    inline bool operator==(const BezierNode & b) const;
  };

  inline std::ostream & operator<<(std::ostream & os, const BezierNode & node);
  inline std::istream & operator>>(std::istream & is, BezierNode & node);

  /// A cubic Bezier curve. It has 2+1 dimensions, where the last value is
  /// interpreted by all *2D member functions as the curve stroke radius or
  /// half of the stroke width.
  /// @todo this will replace Luminous::BezierCurve once we get rid of SplineManager
  class CubicBezierCurve
  {
  public:
    struct PolylinePoint
    {
      Nimble::Vector3f point;
      Nimble::Vector2f tangent2D;
    };

  public:
    CubicBezierCurve()
    {}

    CubicBezierCurve(Nimble::Vector3f p0, Nimble::Vector3f ctrlOutP0,
                     Nimble::Vector3f ctrlInP1, Nimble::Vector3f p1)
      : m_data{{p0, ctrlOutP0, ctrlInP1, p1}}
    {}

    CubicBezierCurve(const BezierNode & begin, const BezierNode & end)
      : m_data{{begin.point, begin.ctrlOut, end.ctrlIn, end.point}}
    {}

    inline Nimble::Vector3f & operator[](int i) { return m_data[i]; }
    inline Nimble::Vector3f operator[](int i) const { return m_data[i]; }

    inline const Nimble::Vector3f * data() const { return m_data.data(); }

    /// Makes polyline approximation of the curve. Does not include the start point.
    /// Interpretes the curve as 2D with .z component as half of the stroke width.
    /// @param[out] points Result is stored into this vector
    /// @param toleranceSqr square of max error
    /// @param angleToleranceCos cosine of the maximum angle between two
    ///        consecutive tangents written to the output vector
    /// @param prevUnitTangent unit tangent of the previous PolylinePoint
    ///        added to 'points'. Needed for implementing angleToleranceCos check.
    inline void evaluate2D(std::vector<PolylinePoint> & points, float toleranceSqr,
                           float angleToleranceCos,
                           Nimble::Vector2f prevUnitTangent) const;

    /// Splits curve into two curves at the given parameter
    /// @param curve Curve to split
    /// @param left First half of the curve (before t)
    /// @param right Second half of the curve (after t)
    /// @param t Where to split the curve
    inline void subdivide(CubicBezierCurve & left,
                          CubicBezierCurve & right, float t) const;

    /// Checks whether the curve is flat given the squared tolerance.
    inline bool isFlat(float toleranceSqr) const;

    /// Calculates intersections of the curve with a shape and return
    /// intersecting curve parts as t parameter ranges.
    /// Interpretes the curve as 2D with .z component as half of the stroke width.
    ///
    /// For instance, if the curve and the shape don't intersect at all,
    /// intersections vector will not be touched.
    /// If the curve is fully inside the shape, one range (0..1) will be
    /// returned.
    /// @param[out] intersections Intersecting parts of the curve are appended
    ///             to this vector as t ranges
    /// @param shape Arbitrary shape for the intersection testing. Needs to have
    ///        shape->intersects(Nimble::Rect) and shape->contains(Nimble::Rect).
    /// @param sizeToleranceSqr Square of the maximum error the function can do
    /// @sa Luminous::splineIntersections
    template <typename Shape>
    void intersections2D(std::vector<Nimble::Rangef> & intersections,
                         const Shape & shape,
                         float sizeToleranceSqr,
                         float leftT = 0.f, float rightT = 1.f) const;

    /// Like intersections2D, but only returns true if the shape intersects
    /// with the curve without calculating the intersection points.
    template <typename Shape>
    bool intersects(const Shape & shape,
                    float sizeToleranceSqr,
                    float leftT = 0.f, float rightT = 1.f) const;

    /// Calculates the bezier value
    inline Nimble::Vector3f value(float t) const;

    /// Calculates the derivative of the bezier curve in the given point
    inline Nimble::Vector3f tangent(float t) const;

    /// Same as tangent(), but just for the X and Y components
    inline Nimble::Vector2f tangent2D(float t) const;

    /// Calculates the second derivative, see tangent() for the first derivative
    inline Nimble::Vector3f derivative2(float t) const;

  private:
    std::array<Nimble::Vector3f, 4> m_data;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  bool BezierNode::operator==(const BezierNode & b) const
  {
    return point == b.point &&
        ctrlIn == b.ctrlIn &&
        ctrlOut == b.ctrlOut;
  }

  std::ostream & operator<<(std::ostream & os, const BezierNode & node)
  {
    return os << node.ctrlIn << ' ' << node.point << ' ' << node.ctrlOut;
  }

  std::istream & operator>>(std::istream & is, BezierNode & node)
  {
    return is >> node.ctrlIn >> node.point >> node.ctrlOut;
  }

  void CubicBezierCurve::evaluate2D(std::vector<PolylinePoint> & points, float toleranceSqr, float angleToleranceCos,
                                    Nimble::Vector2f prevUnitTangent) const
  {
    if (isFlat(toleranceSqr)) {
      Nimble::Vector2f t = tangent2D(1.f);
      const float lenSqr = t.lengthSqr();
      if (lenSqr < toleranceSqr || dot(t /= std::sqrt(lenSqr), prevUnitTangent) > angleToleranceCos) {
        points.push_back({m_data[3], t});
        return;
      }
    }

    float lenSqr = (m_data[3] - m_data[0]).lengthSqr();
    if (lenSqr < toleranceSqr) {
      points.push_back({m_data[3], tangent2D(1.f).normalized()});
      return;
    }

    CubicBezierCurve left, right;
    float mid = 0.5f;
    subdivide(left, right, mid);
    left.evaluate2D(points, toleranceSqr, angleToleranceCos, prevUnitTangent);
    right.evaluate2D(points, toleranceSqr, angleToleranceCos, tangent2D(mid).normalized());
  }

  void CubicBezierCurve::subdivide(CubicBezierCurve & left,
                                   CubicBezierCurve & right, float t) const
  {
    // De Casteljau's algorithm
    auto p0 = m_data[0];
    auto p1 = m_data[1];
    auto p2 = m_data[2];
    auto p3 = m_data[3];

    auto p11 = (1.f-t)*p0 + t*p1;
    auto p21 = (1.f-t)*p1 + t*p2;
    auto p31 = (1.f-t)*p2 + t*p3;
    auto p12 = (1.f-t)*p11 + t*p21;
    auto p22 = (1.f-t)*p21 + t*p31;
    auto p13 = (1.f-t)*p12 + t*p22;

    left = { p0, p11, p12, p13 };
    right = { p13, p22, p31, p3 };
  }

  bool CubicBezierCurve::isFlat(float toleranceSqr) const
  {
    // calculate the maximum difference between the middle control points and a straight
    // line between the end points.
    // See http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
    float invSqr = 1.0f / (m_data[3] - m_data[0]).lengthSqr();
    float aSqr = cross(m_data[1] - m_data[0], m_data[1] - m_data[3]).lengthSqr() * invSqr;
    float bSqr = cross(m_data[2] - m_data[0], m_data[2] - m_data[3]).lengthSqr() * invSqr;

    return std::max(aSqr, bSqr) <= toleranceSqr;
  }

  template <typename Shape>
  void CubicBezierCurve::intersections2D(std::vector<Nimble::Rangef> & intersections,
                                         const Shape & shape,
                                         float sizeToleranceSqr,
                                         float leftT, float rightT) const
  {
    Nimble::Rectf curveBounds;
    for (auto v: m_data)
      curveBounds.expand(v.vector2(), v.z);

    if (shape.contains(curveBounds)) {
      if (!intersections.empty() && intersections.back().high() == leftT)
        intersections.back().setHigh(rightT);
      else
        intersections.push_back({leftT, rightT});
      return;
    }

    if (!shape.intersects(curveBounds))
      return;

    const float curveLengthSqr = (m_data[0] - m_data[3]).lengthSqr();
    if (curveLengthSqr < sizeToleranceSqr) {
      if (!intersections.empty() && intersections.back().high() == leftT)
        intersections.back().setHigh(rightT);
      else
        intersections.push_back({leftT, rightT});
      return;
    }

    Luminous::CubicBezierCurve left, right;
    subdivide(left, right, 0.5f);

    float mid = 0.5f * (leftT + rightT);
    left.intersections2D(intersections, shape, sizeToleranceSqr, leftT, mid);
    right.intersections2D(intersections, shape, sizeToleranceSqr, mid, rightT);
  }

  template <typename Shape>
  bool CubicBezierCurve::intersects(const Shape & shape,
                                    float sizeToleranceSqr,
                                    float leftT, float rightT) const
  {
    Nimble::Rectf curveBounds;
    for (auto v: m_data)
      curveBounds.expand(v.vector2(), v.z);

    if (shape.contains(curveBounds))
      return true;

    if (!shape.intersects(curveBounds))
      return false;

    const float curveLengthSqr = (m_data[0] - m_data[3]).lengthSqr();
    if (curveLengthSqr < sizeToleranceSqr)
      return true;

    Luminous::CubicBezierCurve left, right;
    subdivide(left, right, 0.5f);

    float mid = 0.5f * (leftT + rightT);
    return left.intersects(shape, sizeToleranceSqr, leftT, mid) ||
        right.intersects(shape, sizeToleranceSqr, mid, rightT);
  }

  Nimble::Vector3f CubicBezierCurve::value(float t) const
  {
    float tm = 1.f - t;
    auto p0 = m_data[0];
    auto p1 = m_data[1];
    auto p2 = m_data[2];
    auto p3 = m_data[3];

    return tm*tm*tm*p0 + 3*tm*tm*t*p1 + 3*tm*t*t*p2 + t*t*t*p3;
  }

  Nimble::Vector3f CubicBezierCurve::tangent(float t) const
  {
    float tm = 1.f - t;
    auto p0 = m_data[0];
    auto p1 = m_data[1];
    auto p2 = m_data[2];
    auto p3 = m_data[3];

    return 3*tm*tm*(p1 - p0) + 6*tm*t*(p2 - p1) + 3*t*t*(p3 - p2);
  }

  Nimble::Vector2f CubicBezierCurve::tangent2D(float t) const
  {
    float tm = 1.f - t;
    auto p0 = m_data[0].vector2();
    auto p1 = m_data[1].vector2();
    auto p2 = m_data[2].vector2();
    auto p3 = m_data[3].vector2();

    return 3*tm*tm*(p1 - p0) + 6*tm*t*(p2 - p1) + 3*t*t*(p3 - p2);
  }

  Nimble::Vector3f CubicBezierCurve::derivative2(float t) const
  {
    float tm = 1.f - t;
    auto p0 = m_data[0];
    auto p1 = m_data[1];
    auto p2 = m_data[2];
    auto p3 = m_data[3];

    return 6*tm*(p2 - 2.f * p1 + p0) + 6*t*(p3 - 2.f * p2 + p1);
  }
}
