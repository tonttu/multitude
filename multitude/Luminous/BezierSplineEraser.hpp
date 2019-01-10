#pragma once

#include "BezierSpline.hpp"

namespace Luminous
{
  /// Helper class to perform erasing on a set of bezier splines
  template <typename Shape>
  class BezierSplineEraser
  {
  public:
    /// @param shape See Luminous::CubicBezierCurve::intersections
    /// @param sizeToleranceSqr See Luminous::CubicBezierCurve::intersections
    BezierSplineEraser(const Shape & shape, float sizeToleranceSqr);
    /// @param path The input spline that we are erasing with m_shape. This
    ///        vector will not be modified
    /// @param newPaths List of new splines that replace the original one
    /// @param pathBounds Optional bounding box for path. Used to optimize
    ///        erasing if this is not empty.
    /// @returns true if something was erased, false if the shape didn't
    ///          intersect with the path
    bool erase(const BezierSpline & path, std::vector<BezierSpline> & newPaths,
               const Nimble::Rectf & pathBounds = Nimble::Rectf());

  private:
    const Shape m_shape;
    const float m_sizeToleranceSqr;
    std::vector<SplineRange> m_splineIntersections;
    std::vector<Nimble::Rangef> m_curveIntersections;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename Shape>
  BezierSplineEraser<Shape>::BezierSplineEraser(const Shape & shape, float sizeToleranceSqr)
    : m_shape(shape)
    , m_sizeToleranceSqr(sizeToleranceSqr)
  {
  }

  template <typename Shape>
  bool BezierSplineEraser<Shape>::erase(const BezierSpline & path, std::vector<BezierSpline> & newPaths,
                                        const Nimble::Rectf & pathBounds)
  {
    newPaths.clear();
    m_splineIntersections.clear();

    if (!pathBounds.isEmpty()) {
      if (m_shape.contains(pathBounds))
        return true;

      if (!m_shape.intersects(pathBounds))
        return false;
    }

    splineIntersections(path, m_splineIntersections, m_shape, m_sizeToleranceSqr, m_curveIntersections);

    if (m_splineIntersections.empty())
      return false;

    if (m_splineIntersections.size() == 1) {
      SplineRange & range = m_splineIntersections[0];
      if (range.leftIdx == 0 && range.leftT == 0.f &&
          range.rightIdx + 2 == path.size() && range.rightT == 1.0f)
        return true;
    }

    size_t prevIdx = 0;
    float prevT = 0;

    for (SplineRange & range: m_splineIntersections) {
      if (range.leftIdx == 0 && range.leftT == 0) {
        prevIdx = range.rightIdx;
        prevT = range.rightT;
        continue;
      }

      newPaths.push_back(splineExtractRange(path, {prevIdx, prevT, range.leftIdx, range.leftT}));
      prevIdx = range.rightIdx;
      prevT = range.rightT;
    }

    if (prevIdx + 2 != path.size() || prevT != 1.0f)
      newPaths.push_back(splineExtractRange(path, {prevIdx, prevT, path.size() - 2, 1.0f}));

    return true;
  }
}
