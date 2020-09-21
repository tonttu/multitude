#pragma once

#include "CubicBezierCurve.hpp"
#include "Export.hpp"

#include <QList>

#include <optional>

namespace Luminous
{
  /// A cubic bezier spline is simply a vector of bezier nodes
  using BezierSpline = std::vector<Luminous::BezierNode>;

  /// Used to specify a range in BezierSpline. Left side of the range is
  /// CubicBezierCurve(path[leftIdx], path[leftIdx+1]).value(leftT)
  /// and the right side of the range is
  /// CubicBezierCurve(path[rightIdx], path[rightIdx+1]).value(rightT).
  struct SplineRange
  {
    size_t leftIdx;
    float leftT;

    size_t rightIdx;
    float rightT;
  };

  /// Calculates intersections of a cubic bezier spline with a shape and returns
  /// intersecting curve parts. This functions does the same for a spline that
  /// Luminous::CubicBezierCurve::intersections2D does for a single curve.
  /// @param[out] splineIntersections Intersecting parts of the spline
  /// @param shape See Luminous::CubicBezierCurve::intersections
  /// @param sizeToleranceSqr See Luminous::CubicBezierCurve::intersections
  /// @param curveIntersections This parameter is here just as an optimization
  ///        to reduce the number of memory allocations. This is used internally
  ///        as a temporary buffer for Luminous::CubicBezierCurve::intersections.
  ///        Just create a new instance of the vector on stack and pass the same
  ///        instance (without clearing it) to this function on consecutive calls.
  template <typename Shape>
  void splineIntersections2D(const BezierSpline & path,
                             std::vector<SplineRange> & splineIntersections,
                             const Shape & shape,
                             float sizeToleranceSqr,
                             std::vector<Nimble::Rangef> & curveIntersections);

  /// Extracts a range of a spline as a new spline
  LUMINOUS_API BezierSpline splineExtractRange(const BezierSpline & src,
                                               SplineRange range);

  /// Bounding box of all spline control points. The resulting bbox might be
  /// too large, but the implementation is faster than splineBounds
  /// Interpretes the spline as 2D with .z component as half of the stroke width.
  LUMINOUS_API Nimble::Rectf splineBoundsApproximation2D(const BezierSpline & path);

  /// Bezier spline bounding box. This function is accurate but slower than
  /// splineBoundsApproximation.
  /// Interpretes the spline as 2D with .z component as half of the stroke width.
  LUMINOUS_API Nimble::Rectf splineBounds2D(const BezierSpline & path);
  LUMINOUS_API Nimble::Rectf splineBounds2D(const Luminous::BezierNode * begin,
                                            const Luminous::BezierNode * end);

  /// Converts Luminous::SplineManager::Points to BezierSpline.
  /// The resulting spline is 2D with .z component set to half of the stroke width.
  /// This is inlined so that mt-canvus-server can use this without linking to Luminous.
  /// @param fixControlPoints if true, attemps to fix control points from
  ///        the original curve to be more smooth. This is needed since
  ///        SplineManager doesn't generate control points properly for a
  ///        cubic bezier spline.
  /// @returns non-empty converted spline or no value if the given point vector
  /// doesn't have correct number of points (3N+1).
  template <typename PointVector>
  inline std::optional<BezierSpline> convertSplineManagerPath2D(
      const PointVector & points, float strokeWidth, bool fixControlPoints = true);

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename Shape>
  void splineIntersections2D(const BezierSpline & path,
                             std::vector<SplineRange> & splineIntersections,
                             const Shape & shape,
                             float sizeToleranceSqr,
                             std::vector<Nimble::Rangef> & curveIntersections)
  {
    if (path.size() < 2)
      return;

    for (size_t idx = 0, m = path.size() - 1; idx < m; ++idx) {
      curveIntersections.clear();
      Luminous::CubicBezierCurve curve(path[idx], path[idx+1]);
      curve.intersections2D(curveIntersections, shape, sizeToleranceSqr);

      for (const Nimble::Rangef& r: curveIntersections) {
        if (!splineIntersections.empty()) {
          SplineRange & prev = splineIntersections.back();
          if (prev.rightIdx + 1 == idx && prev.rightT == 1.f && r.low() == 0.f) {
            prev.rightIdx = idx;
            prev.rightT = r.high();
          } else {
            SplineRange range;
            range.leftIdx = range.rightIdx = idx;
            range.leftT = r.low();
            range.rightT = r.high();
            splineIntersections.push_back(range);
          }
        } else {
          SplineRange range;
          range.leftIdx = range.rightIdx = idx;
          range.leftT = r.low();
          range.rightT = r.high();
          splineIntersections.push_back(range);
        }
      }
    }
  }

  template <typename PointVector>
  std::optional<BezierSpline> convertSplineManagerPath2D(const PointVector & points,
                                                         float strokeWidth, bool fixControlPoints)
  {
    if ((points.size() % 3) != 1)
      return {};

    const float r = strokeWidth * 0.5f;
    if (points.size() == 1) {
      BezierSpline path(1);
      path[0].ctrlIn = path[0].point = path[0].ctrlOut = Nimble::Vector3f(points[0], r);
      return path;
    }

    BezierSpline path(1 + points.size() / 3);
    path[0].ctrlIn = path[0].point = Nimble::Vector3f(points[0], r);
    path[0].ctrlOut = Nimble::Vector3f(points[1], r);

    for (size_t i = 1; i < path.size() - 1; ++i) {
      Luminous::BezierNode & node = path[i];
      node.ctrlIn = Nimble::Vector3f(points[i*3 - 1], r);
      node.point = Nimble::Vector3f(points[i*3], r);
      node.ctrlOut = Nimble::Vector3f(points[i*3 + 1], r);

      if (fixControlPoints) {
        Nimble::Vector2f in = points[i*3] - points[i*3 - 1];
        float inLen = in.length();
        Nimble::Vector2f out = points[i*3 + 1] - points[i*3];
        float outLen = out.length();
        float angleCos = dot(in / inLen, out / outLen);
        // Fix broken splines generated by SplineManager
        if (angleCos < 0.999f) {
          Nimble::Vector2f tg = (points[i*3+3] - points[i*3-3]).normalized();
          node.ctrlIn = Nimble::Vector3f(points[i*3] - tg * inLen, r);
          node.ctrlOut = Nimble::Vector3f(points[i*3] + tg * outLen, r);
        }
      }
    }

    path.back().ctrlIn = Nimble::Vector3f(points[points.size() - 2], r);
    path.back().ctrlOut = path.back().point = Nimble::Vector3f(points.back(), r);

    return path;
  }
}
