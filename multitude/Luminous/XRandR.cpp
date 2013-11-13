/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "XRandR.hpp"

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <Radiant/Trace.hpp>

#include <QString>

namespace Luminous
{
  XRandR::XRandR()
  {
  }

  std::vector<ScreenInfo> XRandR::screens(Display * display, int screen)
  {
    assert(display);
    std::vector<ScreenInfo> res;

    Window root = XRootWindow(display, screen);

    if(!root) {
      Radiant::warning("XRandR::screens # Couldn't find a root window for screen %d", screen);
      return res;
    }

    int event_base = 0, error_base = 0;
    if(!XRRQueryExtension(display, &event_base, &error_base)) {
      Radiant::warning("XRandR::screens # XRRQueryExtension failed");
      return res;
    }

    int major_version, minor_version;
    if(!XRRQueryVersion(display, &major_version, &minor_version)) {
      Radiant::warning("XRandR::screens # XRRQueryExtension failed");
      return res;
    }

    XRRScreenResources * sr = XRRGetScreenResources(display, root);
    if(!sr) {
      Radiant::warning("XRandR::screens # XRRGetScreenResources failed");
      return res;
    }

    for(int i = 0; i < sr->noutput; ++i) {
      XRROutputInfo * output = XRRGetOutputInfo(display, sr, sr->outputs[i]);
      if(!output) {
        Radiant::warning("XRandR::screens # XRRGetOutputInfo failed");
        continue;
      }

      if (output->crtc == 0) {
        XRRFreeOutputInfo(output);
        continue;
      }
      XRRCrtcInfo * crtc = XRRGetCrtcInfo(display, sr, output->crtc);

      if (!crtc) {
        Radiant::warning("XRandR::screens # XRRGetCrtcInfo failed");
      } else {
        ScreenInfo info;
        info.setConnection(output->name);
        Nimble::Vector2i l(crtc->x, crtc->y);
        Nimble::Vector2i h = l + Nimble::Vector2i(crtc->width, crtc->height);
        info.setGeometry(Nimble::Recti(l, h));
        if (crtc->rotation == RR_Rotate_0)
          info.setRotation(ScreenInfo::ROTATE_0);
        else if (crtc->rotation == RR_Rotate_90)
          info.setRotation(ScreenInfo::ROTATE_90);
        else if (crtc->rotation == RR_Rotate_180)
          info.setRotation(ScreenInfo::ROTATE_180);
        else if (crtc->rotation == RR_Rotate_270)
          info.setRotation(ScreenInfo::ROTATE_270);
        else {
          Radiant::warning("XRandR::screens # Unknown rotation %d", crtc->rotation);
        }
        res.push_back(info);
        XRRFreeCrtcInfo(crtc);
      }

      XRRFreeOutputInfo(output);
    }

    XRRFreeScreenResources(sr);
    return res;
  }
}
