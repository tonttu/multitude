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

#include "Vector2.hpp"

namespace Nimble {

  bool linesIntersect(Vector2f line1Start, Vector2f line1End,
                             Vector2f line2Start, Vector2f line2End,
                             Vector2f * interPoint)
  {
    // Check if either line has zero length

    if((line1Start == line1End) || (line2Start == line2End))
    {
      return false;
    }

    // Get slope and deltas of first line

    int   slopeType1 = 0;
    Vector2f  delta1;
    const float   m1 = lineSlope(line1Start, line1End, slopeType1, delta1);

    // Get slope and deltas of second line

    int   slopeType2 = 0;
    Vector2f  delta2;
    const float   m2 = lineSlope(line2Start, line2End, slopeType2, delta2);

    // Determine whether infinite lines cross: if so compute line parameters

    bool  cross = false;
    float   t1 = 0.0f;
    float   t2 = 0.0f;

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
          if(m1 == m2)
          // Lines parallel - no intersection point
          {
          }
          else
          {
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
    {
      return false;
    }

    // Compute point of intersection

    if(interPoint)
    {
      * interPoint = Vector2f(line1Start.x + t1 * delta1.x, line1Start.y + t1 * delta1.y);
    }

    // Return true only if point of intersection is on both lines

    return(t1 >= 0.0f && t1 <= 1.0f && t2 >= 0.0f && t2 <= 1.0f);
  }

}
