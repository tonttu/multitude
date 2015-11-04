/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_LINEINTERSECTION_HPP
#define NIMBLE_LINEINTERSECTION_HPP

#include "Vector2.hpp"

namespace Nimble
{

  /// @todo this line stuff really doesn't belong in here!
  /// Line slope types.
  enum LineSlopeType
  {
    LS_VERTICAL,
    LS_SLOPING,
    LS_HORIZONTAL
  };

  /// Compute slope of line.
  /// @param lineStart Line starting point
  /// @param lineEnd Line end point
  /// @param slopeType reference to int to receive slope type.
  /// @param delta reference to Nimble::Vector2f to receive delta.
  /// @return Slope value.
  template <typename T>
  typename Decltype<T, float>::mul lineSlope(const Nimble::Vector2T<T> & lineStart, const Nimble::Vector2T<T> & lineEnd,
                                             int & slopeType, Nimble::Vector2T<T> & delta)
  {
    delta = lineEnd - lineStart;

    if(std::abs(delta.x) < std::numeric_limits<T>::epsilon())
    {
      slopeType = LS_VERTICAL;
    }
    else if(std::abs(delta.y) < std::numeric_limits<T>::epsilon())
    {
      slopeType = LS_HORIZONTAL;
    }
    else
    {
      slopeType = LS_SLOPING;
      return delta.y / delta.x;
    }

    return 0;
  }

  /// Test for intersection of line segments.
  /// @param line1Start, line1End first line.
  /// @param line2Start, line2End second line.
  /// @param interPoint optional pointer to vector to receive the intersection point.
  /// @return true if line segments intersect.
  template <typename T>
  bool linesIntersect(Vector2T<T> line1Start, Vector2T<T> line1End,
                      Vector2T<T> line2Start, Vector2T<T> line2End,
                      Vector2T<T> * interPoint = 0)
  {
    // Check if either line has zero length
    if((line1Start == line1End) || (line2Start == line2End))
      return false;

    // Get slope and deltas of first line
    int   slopeType1 = 0;
    Nimble::Vector2T<T>  delta1;
    const auto   m1 = lineSlope(line1Start, line1End, slopeType1, delta1);

    // Get slope and deltas of second line
    int   slopeType2 = 0;
    Nimble::Vector2T<T>  delta2;
    const auto   m2 = lineSlope(line2Start, line2End, slopeType2, delta2);

    // Determine whether infinite lines cross: if so compute line parameters
    bool  cross = false;
    T   t1(0);
    T   t2(0);

    switch(slopeType1)
    {
    case LS_VERTICAL:
      switch(slopeType2)
      {
      case LS_VERTICAL:
        // Lines parallel - no intersection point
        break;

      case LS_SLOPING:
      {
        cross = true;
        t2 = (line1Start.x - line2Start.x) / delta2.x;
        t1 = (line2Start.y + (t2 * delta2.y) - line1Start.y) / delta1.y;
      }
        break;

      case LS_HORIZONTAL:
      {
        cross = true;
        t1 = (line2Start.y - line1Start.y) / delta1.y;
        t2 = (line1Start.x - line2Start.x) / delta2.x;
      }
        break;
      }
      break;

    case LS_SLOPING:
      switch(slopeType2)
      {
      case LS_VERTICAL:
      {
        cross = true;
        t1 = (line2Start.x - line1Start.x) / delta1.x;
        t2 = (line1Start.y + (t1 * delta1.y) - line2Start.y) / delta2.y;
      }
        break;

      case LS_SLOPING:
      {
        if(m1 != m2) {
          cross = true;
          const float   value = delta2.x * delta1.y;
          const float   divisor = 1.0f - (delta1.x * delta2.y) / value;
          t1 = (line2Start.y / delta1.y + (line1Start.x * delta2.y) / value
                - (line2Start.x * delta2.y) / value - line1Start.y / delta1.y) / divisor;
          t2 = (line1Start.x + t1 * delta1.x - line2Start.x) / delta2.x;
        }
      }
        break;

      case LS_HORIZONTAL:
      {
        cross = true;
        t1 = (line2Start.y - line1Start.y) / delta1.y;
        t2 = (line1Start.x + (t1 * delta1.x) - line2Start.x) / delta2.x;
      }
        break;
      };
      break;

    case LS_HORIZONTAL:
      switch(slopeType2)
      {
      case LS_VERTICAL:
      {
        cross = true;
        t1 = (line2Start.x - line1Start.x) / delta1.x;
        t2 = (line1Start.y - line2Start.y) / delta2.y;
      }
        break;

      case LS_SLOPING:
      {
        cross = true;
        t2 = (line1Start.y - line2Start.y) / delta2.y;
        t1 = (line2Start.x + t2 * delta2.x - line1Start.x) / delta1.x;
      }
        break;

      case LS_HORIZONTAL:
        // Lines parallel - no intersection point
        break;
      }
      break;
    }

    if(!cross)
      return false;

    // Compute point of intersection
    if(interPoint)
      * interPoint = Vector2T<T>(line1Start.x + t1 * delta1.x, line1Start.y + t1 * delta1.y);

    // Return true only if point of intersection is on both lines
    return(t1 >= T(0) && t1 <= T(1) && t2 >= T(0) && t2 <= T(1));
  }


}

#endif // LINEINTERSECTION_HPP
