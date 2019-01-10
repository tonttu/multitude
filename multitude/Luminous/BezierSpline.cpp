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

  Nimble::Rectf splineBounds(const BezierSpline & path)
  {
    Nimble::Rect bbox;
    if (path.empty())
      return bbox;

    bbox.expand(path[0].point, 0.5f * path[0].strokeWidth);

    for (size_t i = 0, m = path.size() - 1; i < m; ++i) {
      // Solve derivative's roots for x and y, see
      // https://pomax.github.io/bezierinfo/#boundingbox
      const CubicBezierCurve curve(path[i], path[i+1]);
      const Nimble::Vector2f a = 3.f * (-curve[0] + 3.f*curve[1] - 3.f*curve[2] + curve[3]);
      const Nimble::Vector2f b = 6.f * (curve[0] - 2.f*curve[1] + curve[2]);
      const Nimble::Vector2f c = 3.f * (curve[1] - curve[0]);
      const float dx = b.x*b.x - 4.f*a.x*c.x;
      const float dy = b.y*b.y - 4.f*a.y*c.y;

      if (dx >= 0.f) {
        const float tmp = std::sqrt(dx);
        const float t1 = (tmp - b.x) / (2.f * a.x);
        const float t2 = (tmp + b.x) / (-2.f * a.x);
        if (t1 > 0 && t1 < 1.f)
          bbox.expand(curve.value(t1), 0.5f * Nimble::Math::lerp(path[i].strokeWidth, path[i+1].strokeWidth, t1));
        if (t2 > 0 && t2 < 1.f)
          bbox.expand(curve.value(t2), 0.5f * Nimble::Math::lerp(path[i].strokeWidth, path[i+1].strokeWidth, t2));
      }

      if (dy >= 0.f) {
        const float tmp = std::sqrt(dy);
        const float t1 = (tmp - b.y) / (2.f * a.y);
        const float t2 = (tmp + b.y) / (-2.f * a.y);
        if (t1 > 0 && t1 < 1.f)
          bbox.expand(curve.value(t1), 0.5f * Nimble::Math::lerp(path[i].strokeWidth, path[i+1].strokeWidth, t1));
        if (t2 > 0 && t2 < 1.f)
          bbox.expand(curve.value(t2), 0.5f * Nimble::Math::lerp(path[i].strokeWidth, path[i+1].strokeWidth, t2));
      }

      bbox.expand(path[i+1].point, 0.5f * path[i+1].strokeWidth);
    }

    return bbox;
  }
}
