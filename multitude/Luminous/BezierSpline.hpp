#pragma once

#include "CubicBezierCurve.hpp"
#include "Export.hpp"

#include <QList>

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
  /// Luminous::CubicBezierCurve::intersections does for a single curve.
  /// @param[out] splineIntersections Intersecting parts of the spline
  /// @param shape See Luminous::CubicBezierCurve::intersections
  /// @param sizeToleranceSqr See Luminous::CubicBezierCurve::intersections
  /// @param curveIntersections This parameter is here just as an optimization
  ///        to reduce the number of memory allocations. This is used internally
  ///        as a temporary buffer for Luminous::CubicBezierCurve::intersections.
  ///        Just create a new instance of the vector on stack and pass the same
  ///        instance (without clearing it) to this function on consecutive calls.
  template <typename Shape>
  void splineIntersections(const BezierSpline & path,
                           std::vector<SplineRange> & splineIntersections,
                           const Shape & shape,
                           float sizeToleranceSqr,
                           std::vector<Nimble::Rangef> & curveIntersections);

  /// Extracts a range of a spline as a new spline
  LUMINOUS_API BezierSpline splineExtractRange(const BezierSpline & src,
                                               SplineRange range);

  /// Bounding box of all spline control points. The resulting bbox might be
  /// too large, but the implementation is faster than splineBounds
  LUMINOUS_API Nimble::Rectf splineBoundsApproximation(const BezierSpline & path);

  /// Bezier spline bounding box. This function is accurate but slower than
  /// splineBoundsApproximation.
  LUMINOUS_API Nimble::Rectf splineBounds(const BezierSpline & path);
  LUMINOUS_API Nimble::Rectf splineBounds(const Luminous::BezierNode * begin,
                                          const Luminous::BezierNode * end);

  /// Converts Luminous::SplineManager::Points to BezierSpline.
  /// This is inlined so that mt-canvus-server can use this without linking to Luminous.
  /// @param fixControlPoints if true, attemps to fix control points from
  ///        the original curve to be more smooth. This is needed since
  ///        SplineManager doesn't generate control points properly for a
  ///        cubic bezier spline.
  template <typename PointVector>
  inline BezierSpline convertSplineManagerPath(const PointVector & points,
                                               float strokeWidth, bool fixControlPoints = true);

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename Shape>
  void splineIntersections(const BezierSpline & path,
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
      curve.intersections(curveIntersections, shape, sizeToleranceSqr, path[idx].strokeWidth, path[idx+1].strokeWidth);

      for (const Nimble::Rangef r: curveIntersections) {
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
  BezierSpline convertSplineManagerPath(const PointVector & points,
                                        float strokeWidth, bool fixControlPoints)
  {
    BezierSpline path;
    if ((points.size() % 3) != 1 || points.size() == 1) {
      Radiant::error("Invalid SplineData (%d points)", (int)points.size());
      return path;
    }

    path.resize(1 + points.size() / 3);
    path[0].ctrlIn = path[0].point = points[0];
    path[0].ctrlOut = points[1];
    path[0].strokeWidth = strokeWidth;

    for (size_t i = 1; i < path.size() - 1; ++i) {
      Luminous::BezierNode & node = path[i];
      node.ctrlIn = points[i*3 - 1];
      node.point = points[i*3];
      node.ctrlOut = points[i*3 + 1];
      node.strokeWidth = strokeWidth;

      if (fixControlPoints) {
        Nimble::Vector2f in = node.point - node.ctrlIn;
        float inLen = in.length();
        Nimble::Vector2f out = node.ctrlOut - node.point;
        float outLen = out.length();
        float angleCos = dot(in / inLen, out / outLen);
        // Fix broken splines generated by SplineManager
        if (angleCos < 0.999f) {
          Nimble::Vector2f tg = (points[i*3+3] - points[i*3-3]).normalized();
          node.ctrlIn = node.point - tg * inLen;
          node.ctrlOut = node.point + tg * outLen;
        }
      }
    }

    path.back().ctrlIn = points[points.size() - 2];
    path.back().ctrlOut = path.back().point = points.back();
    path.back().strokeWidth = strokeWidth;

    return path;
  }
}
