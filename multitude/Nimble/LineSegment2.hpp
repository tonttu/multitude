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
    /// Constructs a new line segment between two points
    inline LineSegment2T(const Vector2T<T> & p1,
                         const Vector2T<T> & p2)
    {
      m_points[0] = p1;
      m_points[1] = p2;
    }

    inline bool operator == (const LineSegment2T & that) const
    { 
      return (m_points[0] == that.m_points[0]) &&
        (m_points[1] == that.m_points[1]);
    }

    inline bool pointMatch(const LineSegment2T & that) const
    {
      for(int i = 0; i < 2; i++)
        for(int j = 0; j < 2; j++)
          if(m_points[i] == that.m_points[j])
            return true;

      return false;
    }

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

    Vector2T<T> & begin() { return m_points[0]; }
    const Vector2T<T> & begin() const { return m_points[0]; }

    Vector2T<T> & end() { return m_points[1]; }
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
