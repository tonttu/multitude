#pragma once

#include <Nimble/Vector2.hpp>

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
  /// consumes 7N*sizeof(float) bytes memory. The same spline would
  /// consume 10(N-1)*sizeof(float) bytes memory if stored as a vector
  /// of cubic Bezier curves.
  ///
  /// This form already uses less memory when N is at least 4, and with
  /// lots of points it saves 30% memory.
  struct BezierNode
  {
    Nimble::Vector2f point;
    Nimble::Vector2f ctrlIn;
    Nimble::Vector2f ctrlOut;
    float strokeWidth;
  };

  /// A cubic Bezier curve
  /// @todo this will replace Luminous::BezierCurve once we get rid of SplineManager
  class BezierCurve2
  {
  public:
    struct PolylinePoint
    {
      Nimble::Vector2f point;
      Nimble::Vector2f tangent;
      float width;
    };

  public:
    BezierCurve2()
    {}

    BezierCurve2(Nimble::Vector2f p0, Nimble::Vector2f ctrlOutP0,
                 Nimble::Vector2f ctrlInP1, Nimble::Vector2f p1)
      : m_data{{p0, ctrlOutP0, ctrlInP1, p1}}
    {}

    BezierCurve2(const BezierNode & begin, const BezierNode & end)
      : m_data{{begin.point, begin.ctrlOut, end.ctrlIn, end.point}}
    {}

    inline Nimble::Vector2f & operator[](int i) { return m_data[i]; }
    inline Nimble::Vector2f operator[](int i) const { return m_data[i]; }

    inline const Nimble::Vector2f * data() const { return m_data.data(); }

    /// Makes polyline approximation of the curve. Does not include the start point
    /// @param[out] points Result is stored into this vector
    /// @param tolerance max error
    /// @param angleToleranceCos cosine of the maximum angle between two
    ///        consecutive tangents written to the output vector
    /// @param widthBegin stroke width at the beginning of the curve
    /// @param widthEnd stroke width at the end of the curve
    /// @param prevUnitTangent unit tangent of the previous PolylinePoint
    ///        added to 'points'. Needed for implementing angleToleranceCos check.
    inline void evaluate(std::vector<PolylinePoint> & points, float tolerance,
                         float angleToleranceCos, float widthBegin, float widthEnd,
                         Nimble::Vector2f prevUnitTangent) const;

    /// Splits curve into two curves at the given parameter
    /// @param curve Curve to split
    /// @param left First half of the curve (before t)
    /// @param right Second half of the curve (after t)
    /// @param t Where to split the curve
    inline void subdivideCurve(BezierCurve2 & left,
                               BezierCurve2 & right, float t) const;

    /// Checks whether the curve is flat given the tolerance.
    inline bool isFlat(float tolerance) const;

    /// Calculates the derivative of the bezier curve in the given point
    inline Nimble::Vector2f tangent(float t) const;

  private:
    std::array<Nimble::Vector2f, 4> m_data;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  void BezierCurve2::evaluate(std::vector<PolylinePoint> & points, float tolerance, float angleToleranceCos,
                              float widthBegin, float widthEnd, Nimble::Vector2f prevUnitTangent) const
  {
    if (isFlat(tolerance)) {
      Nimble::Vector2f t = tangent(1.f);
      const float len = t.length();
      /// If the length is less than 0.001, we start getting serious
      /// floating point accuracy issues
      if (len < 0.001f || dot(t /= len, prevUnitTangent) > angleToleranceCos) {
        points.push_back({m_data[3], t, widthEnd});
        return;
      }
    }

    BezierCurve2 left, right;
    float mid = 0.5f;
    subdivideCurve(left, right, mid);
    float widthMiddle = mid * (widthBegin + widthEnd);
    left.evaluate(points, tolerance, angleToleranceCos, widthBegin, widthMiddle, prevUnitTangent);
    right.evaluate(points, tolerance, angleToleranceCos, widthMiddle, widthEnd, tangent(mid).normalized());
  }

  void BezierCurve2::subdivideCurve(BezierCurve2 & left,
                                    BezierCurve2 & right, float t) const
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

  bool BezierCurve2::isFlat(float tolerance) const
  {
    // calculate the maximum difference between the middle control points and a straight
    // line between the end points.
    Nimble::Vector2f a = m_data[3] - m_data[0];
    Nimble::Vector2f an = a.perpendicular().normalized();
    Nimble::Vector2f b = m_data[1] - m_data[0];
    Nimble::Vector2f c = m_data[2] - m_data[0];
    float projB = std::abs(dot(b, an));
    float projC = std::abs(dot(c, an));
    float diff = std::max(projB, projC);

    return diff <= tolerance;
  }

  Nimble::Vector2f BezierCurve2::tangent(float t) const
  {
    float tm = 1.f - t;
    auto p0 = m_data[0];
    auto p1 = m_data[1];
    auto p2 = m_data[2];
    auto p3 = m_data[3];

    return 3*tm*tm*(p1 - p0) + 6*tm*t*(p2 - p1) + 3*t*t*(p3 - p2);
  }
}
