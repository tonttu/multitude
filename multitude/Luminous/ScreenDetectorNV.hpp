/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_SCREENDETECTORNV_HPP
#define LUMINOUS_SCREENDETECTORNV_HPP

#include "ScreenDetector.hpp"

/// @cond

namespace Luminous
{
  class ScreenDetectorNV
  {
  public:
    static bool detect(int screen, QList<ScreenInfo> & results);
  };
}

/// @endcond

#endif // LUMINOUS_SCREENDETECTORNV_HPP
