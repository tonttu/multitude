/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_SCREENDETECTORAMD_HPP
#define LUMINOUS_SCREENDETECTORAMD_HPP

#include "ScreenDetector.hpp"

/// @cond

namespace Luminous
{
  class ScreenDetectorAMD
  {
  public:
    static bool detect(int screen, QList<ScreenInfo> & results);
  };
}

/// @endcond

#endif // LUMINOUS_SCREENDETECTORAMD_HPP
