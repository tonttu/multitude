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

#ifndef RADIANT_RGBA_HPP
#define RADIANT_RGBA_HPP

#include <Radiant/Export.hpp>

#include <stdint.h>

/// @todo combine with Radiant::Color (template data type)

namespace Radiant {

  /// A minimal RGBA template class.
  /** For more complete color class, use #Radiant::Color. 
      
      This class was deviced mostly for usage with the grid classes to
      hold RGBA image data.
   */
  template <class T>
  class RADIANT_API RGBAT
  {
  public:
    RGBAT(T r = 0, T g = 0, T b = 0, T a = 0) : r(r), g(g), b(b), a(a) { }

    inline bool operator == (const RGBAT & that) const
    {
      return r == that.r && g == that.g && b == that.b && a == that.a;
    }

    inline bool isBlank() const { return a == 0; }

    T r, g, b, a;
  };

  typedef RGBAT<uint8_t> RGBAu8;

#ifdef WIN32
#ifdef RADIANT_EXPORT
  // In WIN32 template classes must be instantiated to be exported
  template class RGBAT<uint8_t>;
#endif
#endif

}

#endif
