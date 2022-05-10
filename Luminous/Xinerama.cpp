/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Xinerama.hpp"

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

#include <Radiant/Trace.hpp>

#include <QString>

namespace Luminous
{
  Xinerama::Xinerama()
  {
  }

  std::vector<ScreenInfo> Xinerama::screens(Display * display, int screen)
  {
    assert(display);
    std::vector<ScreenInfo> res;

    int event_base = 0, error_base = 0;
    if (!XineramaQueryExtension(display, &event_base, &error_base)) {
      Radiant::warning("Xinerama::screens # XineramaQueryExtension failed");
      return res;
    }

    int major_version, minor_version;
    if (!XineramaQueryVersion(display, &major_version, &minor_version)) {
      Radiant::warning("Xinerama::screens # XineramaQueryVersion failed");
      return res;
    }

    if (!XineramaIsActive(display))
      return res;

    int n;
    XineramaScreenInfo * si = XineramaQueryScreens(display, &n);
    if (!si) {
      Radiant::warning("Xinerama::screens # XineramaQueryScreens failed");
      return res;
    }

    for (int i = 0; i < n; ++i) {
      ScreenInfo info;
      info.setConnection(QString("Xinerama %1").arg(i));
      info.setGeometry(Nimble::Recti(Nimble::Vector2i(si[i].x_org, si[i].y_org),
                                     Nimble::Size(si[i].width, si[i].height)));
      info.setGpu("Xinerama");
      info.setGpuName("Xinerama");
      info.setLogicalScreen(screen);
      info.setName(QString("Xinerama %1").arg(i));
      info.setNumId(i);
      res.push_back(info);
    }

    XFree(si);
    return res;
  }
}
