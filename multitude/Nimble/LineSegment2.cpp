/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "LineSegment2.hpp"
#include "Matrix3.hpp"

#include <vector>

namespace Nimble {
  template <typename T>
  bool LineSegment2T<T>::intersectsBezier(Vector2f cp[4])
  {
    // make sure end.x will be >= 0
    Vector2 start = m_points[0].x < m_points[1].x ? m_points[0] : m_points[1];
    Vector2 end = m_points[0].x < m_points[1].x ? m_points[1] : m_points[0];
    end -= start;

    float angle = atan2(end.y, end.x);

    // translate and rotate control points so that we can check intersection against { y = 0 }
    Matrix3 m = Matrix3::rotate2D(-angle) * Matrix3::translate2D(-start);
    Vector2 cps[] = {
      m.project(cp[0]),
      m.project(cp[1]),
      m.project(cp[2]),
      m.project(cp[3])
    };

    if ((cps[0].y <= 0 && cps[1].y <= 0 && cps[2].y <= 0 && cps[3].y <= 0) ||
        (cps[0].y >= 0 && cps[1].y >= 0 && cps[2].y >= 0 && cps[3].y >= 0)) {
      return false;
    }

    struct Subdivider {
      Nimble::LineSegment2f check;

      std::vector<Vector2> & points;
      int counts;

      bool subdivide(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, int level=0) {
        ++counts;
        Vector2 p12 = 0.5f*(p1+p2);
        Vector2 p23 = 0.5f*(p2+p3);
        Vector2 p34 = 0.5f*(p3+p4);
        Vector2 p123 = 0.5f*(p12+p23);
        Vector2 p234 = 0.5f*(p23+p34);
        Vector2 p1234 = 0.5f*(p123 + p234);

        const float y0 = p1.y;
        const float y1 = p2.y;
        const float y2 = p3.y;
        const float y3 = p4.y;

        if((y0 <= 0 && y1 <= 0 && y2 <= 0 && y3 <= 0) ||
           (y0 >= 0 && y1 >= 0 && y2 >= 0 && y3 >= 0)) {
          return false;
        } else if(level > 20 || fabs( (p1234 - 0.5f*(p1+p4)).lengthSqr() ) < 1e-1f) {
          Vector2 iPoint;
          if(check.intersects(Nimble::LineSegment2f(p1, p4), &iPoint)) {
            points.push_back(iPoint);
            return true;
          }
        }
        return subdivide(p1, p12, p123, p1234, level+1) || subdivide(p1234, p234, p34, p4, level+1);
      }
    };

    std::vector<Vector2> p;
    Subdivider sub = { Nimble::LineSegment2f(Vector2(0, 0), Vector2(end.length(), 0)), p, 0 };


    bool ret = sub.subdivide( cps[0], cps[1], cps[2], cps[3] );
    return ret;
  }

  /// Template classes must be instantiated to be exported because of intersectsBezier.
  template class LineSegment2T<float>;
  template class LineSegment2T<double>;
}

