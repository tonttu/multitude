/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */
#include "Size2D.hpp"

namespace Radiant
{

  Nimble::Vector2i resize(Nimble::Vector2i oldSize, Nimble::Vector2i newSize, bool keepAspect)
  {
    if(!keepAspect || oldSize.x == 0 || oldSize.y == 0) {
      return newSize;
    } else {
      int rw = newSize.y * oldSize.x / oldSize.y;

      bool useHeight = (rw <= newSize.x);

      if(useHeight) {
        return Nimble::Vector2i(rw, newSize.y);
      } else {
        int y = newSize.x * oldSize.y / oldSize.x;
        int x = newSize.x;
        return Nimble::Vector2i(x, y);
      }
    }
  }

  Nimble::Vector2i fitToSize(float aspect, Nimble::Vector2i constraint)
  {
    int rw = static_cast<int> (constraint.y * aspect);
    bool useHeight = (rw <= constraint.x);

    if(useHeight) 
      return Nimble::Vector2i(rw, constraint.y);
    else 
      return Nimble::Vector2i(constraint.x, static_cast<int> (constraint.x / aspect));
  }

}
