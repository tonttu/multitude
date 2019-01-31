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
    }

    if (range.rightT != 0.f) {
      size_t prev = path.size() - 2, idx = path.size() - 1;
      Luminous::CubicBezierCurve curve(path[prev], path[idx]);
      Luminous::CubicBezierCurve left, right;
      curve.subdivide(left, right, range.rightT);
      path[prev].ctrlOut = left[1];
      path[idx].ctrlIn = left[2];
      path[idx].point = path[idx].ctrlOut = left[3];
    }

    return path;
  }

  Nimble::Rectf splineBoundsApproximation2D(const BezierSpline & path)
  {
    Nimble::Rect bbox;
    for (auto & p: path) {
      bbox.expand(p.ctrlIn.vector2(), p.ctrlIn.z);
      bbox.expand(p.ctrlOut.vector2(), p.ctrlOut.z);
      bbox.expand(p.point.vector2(), p.point.z);
    }
    return bbox;
  }

  Nimble::Rectf splineBounds2D(const BezierSpline & path)
  {
    if (path.empty())
      return Nimble::Rect();

    return splineBounds2D(path.data(), path.data() + path.size());
  }

  Nimble::Rectf splineBounds2D(const Luminous::BezierNode * begin, const Luminous::BezierNode * end)
  {
    Nimble::Rect bbox;
    if (begin == end)
      return bbox;

    bbox.expand(begin->point.vector2(), begin->point.z);

    for (auto last = end - 1; begin < last; ++begin) {
      // Solve derivative's roots for x and y, see
      // https://pomax.github.io/bezierinfo/#boundingbox
      const CubicBezierCurve curve(begin[0], begin[1]);
      const Nimble::Vector2f a2 = 2.f * 3.f * (-curve[0].vector2() + 3.f*curve[1].vector2() - 3.f*curve[2].vector2() + curve[3].vector2());
      const Nimble::Vector2f b = 6.f * (curve[0].vector2() - 2.f*curve[1].vector2() + curve[2].vector2());
      const Nimble::Vector2f c = 3.f * (curve[1].vector2() - curve[0].vector2());
      const Nimble::Vector2f d = {b.x*b.x - 2.f*a2.x*c.x, b.y*b.y - 2.f*a2.y*c.y};
      for (int i = 0; i < 2; ++i) {

        if (d[i] >= 0.f) {
          const float tmp = std::sqrt(d[i]);
          // Check if the derivative is close to a line and we are about to
          // trying to solve quadratic formula by dividing with a number very
          // close to zero. If so, just solve bt+c=0 instead of at²+bt+c=0.
          if (std::abs(tmp - b[i]) > std::abs(a2[i]) * 1e4) {
            const float t = -c[i] / b[i];
            if (t > 0 && t < 1.f) {
              Nimble::Vector3f p = curve.value(t);
              bbox.expand(p.vector2(), p.z);
            }
          } else {
            const float t1 = (tmp - b[i]) / a2[i];
            const float t2 = (-tmp - b[i]) / a2[i];
            if (t1 > 0 && t1 < 1.f) {
              Nimble::Vector3f p = curve.value(t1);
              bbox.expand(p.vector2(), p.z);
            }
            if (t2 > 0 && t2 < 1.f) {
              Nimble::Vector3f p = curve.value(t2);
              bbox.expand(p.vector2(), p.z);
            }
          }
        }
      }

      bbox.expand(begin[1].point.vector2(), begin[1].point.z);
    }

    return bbox;
  }
}
