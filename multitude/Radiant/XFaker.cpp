/* COPYRIGHT
 *
 * This file is part of MultiWidgets.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitou.ch
 *
 * All rights reserved, 2007-2009
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "MultiWidgets.hpp" for authors and more details.
 *
 */

#include "XFaker.hpp"

#include <Nimble/Rect.hpp>

// #include <X11/extensions/Xinerama.h>
#include <X11/extensions/XTest.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstdio>
#include <vector>

#include <stdlib.h>

namespace Radiant {

  class XFaker::D
  {
  public:

    Display* m_display;

    /* std::vector<Nimble::Recti> m_screens;

    int screenFor(Nimble::Vector2i loc)
    {
      for(unsigned i = 0; i < m_screens.size(); i++) {
        if(m_screens[i].contains(loc))
          return i;
      }
      
      return DefaultScreen(m_display);
    }
    */
  };



  XFaker::XFaker(const char* displayName)
    : m_d(new D)
  {
    if(!displayName)
      displayName = ":0.0";

    // Open display
    m_d->m_display = XOpenDisplay(displayName);
  
    if(!m_d->m_display) {
      printf("Failed to open display %s\n", displayName);
    }

    int eventBase, errorBase, majorVersion, minorVersion;
    if(XTestQueryExtension(m_d->m_display, &eventBase, &errorBase, &majorVersion, &minorVersion) == False) {
      printf("XTest extension not supported!\n");
    }
    /*
    int screencount = 0;
    XineramaScreenInfo * screens = XineramaQueryScreens(m_d->m_display, & screencount);

    if(screens) {
      for(int i = 0; i < screencount; i++) {
        
        Nimble::Recti rect(screens->x_org, screens->y_org, screens->width, screens->height);
        m_d->m_screens.push_back(rect);
        screens++;
      }
      // free(screens);
    }
    */
  }

  XFaker::~XFaker()
  {
    if(m_d->m_display) 
      XCloseDisplay(m_d->m_display);

    delete m_d;
  }

  void XFaker::fakeMouseMove(int x, int y)
  {
    // int screen = m_d->screenFor(Nimble::Vector2i(x, y));

    /* XTestFakeMotionEvent does not work with Xinerama. */
    // XTestFakeMotionEvent(m_d->m_display, screen, x, y, CurrentTime);
    XWarpPointer(m_d->m_display, None, XDefaultRootWindow(m_d->m_display), 0, 0, 0, 0, x, y);
    XFlush(m_d->m_display);

    printf("Faking mouse movement (%d,%d)\n", x, y);
  }

  void XFaker::fakeMouseButton(int button, bool press) 
  {
    XTestFakeButtonEvent(m_d->m_display, button, press ? True : False, CurrentTime);
    XFlush(m_d->m_display);

    printf("Faking mouse button %s\n", press ? "down" : "up");
  }

  void XFaker::fakeMouseWheel(int, int dy)
  {
    // We only support y-axis scrolling
    if(dy == 0) return;

    // Button 4 is +wheel, button 5 is -wheel
    int button = (dy >= 0 ? 4 : 5);

    // Need absolute value
    dy = (dy < 0 ? -dy : dy);

    printf("Faking mouse wheel %s %d click(s)\n", (button == 4 ? "up" : "down"), dy);

    // Send clicks
    for(; dy >= 0; dy -= 1) {
      XTestFakeButtonEvent(m_d->m_display, button, True, CurrentTime);
      XTestFakeButtonEvent(m_d->m_display, button, False, CurrentTime);
    }

    XFlush(m_d->m_display);    
  }

}
