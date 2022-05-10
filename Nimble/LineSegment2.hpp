/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_LINESEGMENT2T_HPP
#define NIMBLE_LINESEGMENT2T_HPP

#include "Export.hpp"
#include "Vector2.hpp"
#include "Matrix3.hpp"

#include "LineIntersection.hpp"

#include <vector>

namespace Nimble {

  /// 2D line segment
  /** Where lines are of infinite length, line segments have a finite
      length. */

    /// @todo does not really work for floating point values
template <typename T>
  class LineSegment2T
  {
  public:
    /// Construct line segment object, without initializing its values.
    inline LineSegment2T() {}
    /// Initialize line segment from given coordinate values
    inline LineSegment2T(T x1, T y1, T x2, T y2)
    {
      m_points[0].make(x1, y1);
      m_points[1].make(x2, y2);
    }

    /// Constructs a new line segment between two points
    inline LineSegment2T(const Vector2T<T> & p1,
                         const Vector2T<T> & p2)
    {
      m_points[0] = p1;
      m_points[1] = p2;
    }

    /// Compares the end points of two line segments and returns true if they are equal
    inline bool operator == (const LineSegment2T & that) const
    {
      return (m_points[0] == that.m_points[0]) &&
        (m_points[1] == that.m_points[1]);
    }

    /// Returns the length of the line segment
    inline T length() const
    {
      return (m_points[1] - m_points[0]).length();
    }

    /// Returns normalized direction of this line segment.
    inline Vector2T<T> directionNormalized() const
    {
      Vector2T<T> dir = end() - begin();
      dir.normalize();
      return dir;
    }

    /// Compares the given point against both end points and returns true if the point is
    /// equal to either one
    inline bool pointMatch(const LineSegment2T & that) const
    {
      for(int i = 0; i < 2; i++)
        for(int j = 0; j < 2; j++)
          if(m_points[i] == that.m_points[j])
            return true;

      return false;
    }

    /// Returns true if the two line segments intersect
    /// @param that line segment to compare
    /// @param[out] point optional intersection point
    /// @return True if lines intersect
    inline bool intersects(const LineSegment2T & that,
                           Vector2T<T> * point = 0) const
    {
      Nimble::Vector2T<T> tmp;

      bool r = linesIntersect(m_points[0], m_points[1],
                              that.m_points[0], that.m_points[1], & tmp);

      if(point)
        * point = tmp;

      return r;
    }

    /// Tests for intersection with a given line.
    /// @param that line to test against
    /// @param[out] point (optional) intersection point if found
    /// @return True if lines intersect
    inline bool intersectsInfinite(const LineSegment2T & that,
                                   Vector2T<T> * point) const
    {
      float a1 = this->end().y - this->begin().y;
      float b1 = this->begin().x - this->end().x;
      float c1 = this->end().x * this->begin().y - this->begin().x * this->end().y;

      float a2 = that.end().y - that.begin().y;
      float b2 = that.begin().x - that.end().x;
      float c2 = that.end().x * that.begin().y - that.begin().x * that.end().y;//   { a2*x + b2*y + c2 = 0 is line 2 }

      float denom = a1*b2 - a2*b1;

      if(std::abs(denom) > 1.0e-6) {
        if(point) {
          point->x = (b1*c2 - b2*c1)/denom;
          point->y = (a2*c1 - a1*c2)/denom;
        }
        return true;
      }
      else {
        if(point)
          point->clear();
        return false;
      }

    }

    /// Calculates the distance between this (infinite) line and a point.
    /// @param point 2D point
    /// @return Distance between a line and point
    inline float distanceInfinite(const Vector2T<T> & point)
    {
      Nimble::Vector2T<T> perp = directionNormalized().perpendicular();

      return std::abs(dot(perp, point - m_points[0]));
    }

    /// Returns true if the line segment intersects with the given bezier curve
    bool intersectsBezier(Vector2T<T> cp[4]);

    /// Returns the first end point of the line segment
    Vector2T<T> & begin() { return m_points[0]; }
    /// Returns the first end point of the line segment
    const Vector2T<T> & begin() const { return m_points[0]; }

    /// Returns the second end point of the line segment
    Vector2T<T> & end() { return m_points[1]; }
    /// Returns the second end point of the line segment
    const Vector2T<T> & end() const { return m_points[1]; }

  private:

    Vector2T<T> m_points[2];
  };

/// Line segment of floats
  typedef LineSegment2T<float>  LineSegment2;
  /// Line segment of floats
  typedef LineSegment2T<float>  LineSegment2f;
  /// Line segment of doubles
  typedef LineSegment2T<double> LineSegment2d;


  template <typename T>
  bool LineSegment2T<T>::intersectsBezier(Vector2T<T> cp[4])
  {
    // make sure end.x will be >= 0
    Vector2T<T> start = m_points[0].x < m_points[1].x ? m_points[0] : m_points[1];
    Vector2T<T> end = m_points[0].x < m_points[1].x ? m_points[1] : m_points[0];
    end -= start;

    float angle = std::atan2(end.y, end.x);

    // translate and rotate control points so that we can check intersection against { y = 0 }
    Matrix3T<T> m = Matrix3T<T>::makeRotation(-angle) * Matrix3T<T>::makeTranslation(-start);
    Vector2T<T> cps[] = {
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
      Nimble::LineSegment2T<T> check;

      std::vector< Vector2T<T> > & points;
      int counts;

      bool subdivide(Vector2T<T> p1, Vector2T<T> p2, Vector2T<T> p3, Vector2T<T> p4, int level=0) {
        ++counts;
        Vector2T<T> p12 = 0.5f*(p1+p2);
        Vector2T<T> p23 = 0.5f*(p2+p3);
        Vector2T<T> p34 = 0.5f*(p3+p4);
        Vector2T<T> p123 = 0.5f*(p12+p23);
        Vector2T<T> p234 = 0.5f*(p23+p34);
        Vector2T<T> p1234 = 0.5f*(p123 + p234);

        const float y0 = p1.y;
        const float y1 = p2.y;
        const float y2 = p3.y;
        const float y3 = p4.y;

        if((y0 <= 0 && y1 <= 0 && y2 <= 0 && y3 <= 0) ||
          (y0 >= 0 && y1 >= 0 && y2 >= 0 && y3 >= 0)) {
            return false;
        } else if(level > 20 || fabs( (p1234 - 0.5f*(p1+p4)).lengthSqr() ) < 1e-1f) {
          Vector2T<T> iPoint;
          if(check.intersects(Nimble::LineSegment2T<T>(p1, p4), &iPoint)) {
            points.push_back(iPoint);
            return true;
          }
        }
        return subdivide(p1, p12, p123, p1234, level+1) || subdivide(p1234, p234, p34, p4, level+1);
      }
    };

    std::vector< Vector2T<T> > p;
    Subdivider sub = { Nimble::LineSegment2T<T>(Vector2T<T>(0, 0), Vector2T<T>(end.length(), 0)), p, 0 };

    return sub.subdivide( cps[0], cps[1], cps[2], cps[3] );
  }
}

#endif
