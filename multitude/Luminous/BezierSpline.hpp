#pragma once

#include "CubicBezierCurve.hpp"
#include "Export.hpp"

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
  /// too large, but it can be calculated quickly.
  LUMINOUS_API Nimble::Rectf splineBoundsApproximation(const BezierSpline & path);

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
      curve.intersections(curveIntersections, shape, sizeToleranceSqr);

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
}
