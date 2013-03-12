/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ScreenDetector.hpp"

#include <Radiant/Platform.hpp>

#ifndef RADIANT_OSX
# include "ScreenDetectorAMD.hpp"
# include "ScreenDetectorNV.hpp"
#endif

#if defined (RADIANT_LINUX)
#include <QX11Info>
#include <X11/Xlib.h>
#endif

namespace Luminous
{

  void ScreenDetector::scan(bool /*forceRescan*/)
  {
    m_results.clear();

#ifdef RADIANT_LINUX
    int screens = XScreenCount(QX11Info::display());
    for(int screen = 0; screen < screens; ++screen) {
      if(ScreenDetectorNV::detect(screen, m_results)) continue;
      if(ScreenDetectorAMD::detect(screen, m_results)) continue;
    }
#elif RADIANT_WINDOWS
    ScreenDetectorNV::detect(0, m_results);
    ScreenDetectorAMD::detect(0, m_results);
#endif
  }

}
