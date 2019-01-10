#include "BezierSpline.hpp"

namespace Luminous
{
  BezierSpline splineExtractRange(const BezierSpline & src, SplineRange range)
  {
    if (range.leftT == 1.f) {
      ++range.leftIdx;
      range.leftT = 0;
    }
    if (range.rightT == 0.f) {
      --range.rightIdx;
      range.rightT = 1.f;
    }
    if (range.leftIdx > range.rightIdx || (range.leftIdx == range.rightIdx && range.leftT >= range.rightT))
      return BezierSpline();

    BezierSpline path(src.begin() + range.leftIdx, src.begin() + range.rightIdx + 2);
    if (range.leftT != 0.f) {
      Luminous::CubicBezierCurve curve(path[0], path[1]);
      Luminous::CubicBezierCurve left, right;
      curve.subdivide(left, right, range.leftT);
      path[0].ctrlIn = path[0].point = right[0];
      path[0].ctrlOut = right[1];
      path[1].ctrlIn = right[2];
      path[0].strokeWidth = Nimble::Math::lerp(path[0].strokeWidth, path[1].strokeWidth, range.leftT);
    }

    if (range.rightT != 0.f) {
      size_t prev = path.size() - 2, idx = path.size() - 1;
      Luminous::CubicBezierCurve curve(path[prev], path[idx]);
      Luminous::CubicBezierCurve left, right;
      curve.subdivide(left, right, range.rightT);
      path[prev].ctrlOut = left[1];
      path[idx].ctrlIn = left[2];
      path[idx].point = path[idx].ctrlOut = left[3];
      path[idx].strokeWidth = Nimble::Math::lerp(path[prev].strokeWidth, path[idx].strokeWidth, range.rightT);
    }

    return path;
  }

  Nimble::Rectf splineBoundsApproximation(const BezierSpline & path)
  {
    Nimble::Rect bbox;
    for (auto & p: path) {
      float r = 0.5f * p.strokeWidth;
      bbox.expand(p.ctrlIn, r);
      bbox.expand(p.ctrlOut, r);
      bbox.expand(p.point, r);
    }
    return bbox;
  }
}
