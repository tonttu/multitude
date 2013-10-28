/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_XINERAMA_HPP
#define LUMINOUS_XINERAMA_HPP

/// @cond

#include "Export.hpp"
#include "ScreenDetector.hpp"

#include <QX11Info>

class QString;
namespace Luminous
{
  class LUMINOUS_API Xinerama
  {
  public:
    Xinerama();

    std::vector<ScreenInfo> screens(Display * display, int screen);
  };
}

/// @endcond

#endif // LUMINOUS_XINERAMA_HPP
