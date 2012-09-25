/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */


#ifndef LUMINOUS_UTILS_HPP
#define LUMINOUS_UTILS_HPP

#include <Luminous/Luminous.hpp>
#include <Luminous/DummyOpenGL.hpp>

#include <Nimble/Rect.hpp>
#include <Nimble/Matrix3.hpp>

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector4.hpp>

namespace Luminous {

  /// OpenGL utility functions
  /** This class has functions for drawing various simple primitives -
      circles, lines, textured rectangles, etc.*/
  /// @deprecated use RenderContext instead
  class LUMINOUS_API Utils
  {
  public:
/// @cond
    enum Edge {
      LEFT,
      RIGHT,
      TOP,
      BOTTOM
    };

    /*static void fadeEdge(float w, float h, float seam,
             float gamma, Edge e, bool withGrid);*/


//    /** Check that there are no OpenGL errors. If there has been an
//    error, then the error is printed along with msg. */
//    static bool glCheck(const char * msg);

    static inline Nimble::Vector4 project(const Nimble::Matrix3 & m,
                      const Nimble::Vector2 & xy)
    {
      Nimble::Vector3 xyw = m * xy;

      // return Nimble::Vector4(xyw.x / xyw[2], xyw.y / xyw[2], 0, xyw[2]);
      return Nimble::Vector4(xyw.x, xyw.y, 0, xyw[2]);
    }
  };

  /// @endcond
}

#endif
