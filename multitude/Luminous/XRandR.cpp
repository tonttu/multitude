#include "XRandR.hpp"

#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <Radiant/Trace.hpp>

#include <QString>

namespace Luminous
{
  XRandR::XRandR()
  {
  }

  bool XRandR::getGeometry(int screen, const QString & display, Nimble::Recti & rect)
  {
    Display * d = QX11Info::display();
    Window root = XRootWindow(d, screen);

    if(!root) {
      Radiant::warning("XRandR::getGeometry # Couldn't find a root window for screen %d", screen);
      return false;
    }

    int event_base = 0, error_base = 0;
    if(!XRRQueryExtension(d, &event_base, &error_base)) {
      Radiant::warning("XRandR::getGeometry # XRRQueryExtension failed");
      return false;
    }

    int major_version, minor_version;
    if(!XRRQueryVersion(d, &major_version, &minor_version)) {
      Radiant::warning("XRandR::getGeometry # XRRQueryExtension failed");
      return false;
    }

    XRRScreenResources * sr = XRRGetScreenResources(d, root);
    if(!sr) {
      Radiant::error("XRandR::getGeometry # XRRGetScreenResources failed");
      return false;
    }

    bool found = false;
    for(int i = 0; !found && i < sr->noutput; ++i) {
      XRROutputInfo * output = XRRGetOutputInfo(d, sr, sr->outputs[i]);
      if(!output) {
        Radiant::error("XRandR::getGeometry # XRRGetOutputInfo failed");
        continue;
      }

      if(output->name == display) {
        XRRCrtcInfo * crtc = XRRGetCrtcInfo(d, sr, output->crtc);

        if(!crtc) {
          Radiant::error("XRandR::getGeometry # XRRGetCrtcInfo failed");
        } else {
          Nimble::Vector2i l(crtc->x, crtc->y);
          Nimble::Vector2i h = l + Nimble::Vector2i(crtc->width, crtc->height);
          rect.setLow(l);
          rect.setHigh(h);
          found = true;
          XRRFreeCrtcInfo(crtc);
        }
      }

      XRRFreeOutputInfo(output);
    }

    XRRFreeScreenResources(sr);
    return found;
  }
}
