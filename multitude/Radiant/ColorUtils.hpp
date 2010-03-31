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

#ifndef RADIANT_COLOR_UTILS_HPP
#define RADIANT_COLOR_UTILS_HPP

#include <Nimble/Math.hpp>
#include <Nimble/Vector3.hpp>

#include <Radiant/Export.hpp>
#include <Radiant/Trace.hpp>

#include <assert.h>

namespace Radiant
{
    /// @todo move to Color class

  /// ColorUtils contains color conversion utilities
  namespace ColorUtils
  {

    // Conversion between RGB and HSV using code published in Hearn, D. and Baker, M.
    // (1997) "Computer Graphics", New Jersey: Prentice Hall Inc. (pp. 578-579.).
    RADIANT_API void rgbTohsv(float r, float g, float b, float & h, float & s, float & v);
    RADIANT_API void rgbTohsv(Nimble::Vector3f & rgb, Nimble::Vector3f & hsv);
    RADIANT_API void hsvTorgb(float h, float s, float v, float & r, float & g, float & b);
    RADIANT_API void hsvTorgb(Nimble::Vector3f & hsv, Nimble::Vector3f & rgb);

  }

}

#endif
