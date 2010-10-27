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

#ifndef NIMBLE_LINESEGMENT2T_HPP
#define NIMBLE_LINESEGMENT2T_HPP

#include <Nimble/Vector2.hpp>

namespace Nimble {

  /// 2D line segment
  /** Where lines are of infinite length, line segments have a finite
      length. */

    /// @todo does not really work for floating point values
template <typename T>
  class NIMBLE_API LineSegment2T
  {
  public:
    inline LineSegment2T() {}
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

    inline T length() const
    {
      return (m_points[1] - m_points[0]).length();
    }

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
    /// @param point optional intersection point
    inline bool intersects(const LineSegment2T & that,
                           Vector2T<T> * point = 0) const
    {
      Vector2f tmp;

      bool r = linesIntersect(m_points[0], m_points[1],
                              that.m_points[0], that.m_points[1], & tmp);

      if(point)
        * point = tmp;

      return r;
    }

    /** Intersection between two lines, with the lines being treated as infinite lines. */
    inline bool intersectsInfinite(const LineSegment2T & that,
                                   Vector2T<T> * point) const
    {
      /*
      Vector2T<T> v1 = begin();
      Vector2T<T> v2 = v1 + directionNormalized();

      Vector2T<T> v3 = that.begin();
      Vector2T<T> v4 = v3 + that.directionNormalized();

      float x1 = v1.x;
      float y1 = v1.y;
      float x2 = v2.x;
      float y2 = v2.y;

      float x3 = v3.x;
      float y3 = v3.y;
      float x4 = v4.x;
      float y4 = v4.y;

      float bx = x2 - x1;
      float by = y2 - y1;
      float dx = x4 - x3;
      float dy = y4 - y3;
      float b_dot_d_perp = bx*dy - by*dx;

      if(b_dot_d_perp == 0) {

        if(point)
          point->clear();
        return false;
      }
      float cx = x3-x1;
      float cy = y3-y1;
      float t = (cx*dy - cy*dx) / b_dot_d_perp;

      if(point)
        point->make(x1+t*bx, y1+t*by);
      return true;
      */

      float a1 = this->end().y - this->begin().y;
      float b1 = this->begin().x - this->end().x;
      float c1 = this->end().x * this->begin().y - this->begin().x * this->end().y;

      float a2 = that.end().y - that.begin().y;
      float b2 = that.begin().x - that.end().x;
      float c2 = that.end().x * that.begin().y - that.begin().x * that.end().y;//   { a2*x + b2*y + c2 = 0 is line 2 }

      float denom = a1*b2 - a2*b1;

      if(Math::Abs(denom) > 1.0e-6) {
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

    /// Returns true if the line segment intersects with the given bezier curve
    bool intersectsBezier(Vector2f cp[4]);

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

}

#endif
