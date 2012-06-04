/* COPYRIGHT
 *
 * This file is part of ThreadedRendering.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitouch.fi
 *
 * All rights reserved, 2007-2010
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "ThreadedRendering.hpp" for authors and more details.
 *
 */

#ifndef RADIANT_XWINDOW_HPP
#define RADIANT_XWINDOW_HPP

#include "Export.hpp"
#include "Window.hpp"
#include "WindowConfig.hpp"

namespace Radiant
{
  /** X11 OpenGL window class. */
  class RADIANT_API XWindow : public Radiant::Window
  {
  public:
    /// Creates a new XWindow
    XWindow(const WindowConfig& hint = WindowConfig(0, 0, 640, 480, false, false, true, 0),
            const char* caption = 0);
    ~XWindow();

    virtual void poll();
    virtual void swapBuffers();
    virtual void makeCurrent();

    virtual void deinit();

    virtual void minimize();
    virtual void restore();

    virtual void setEventHook(WindowEventHook * hook);

  private:
    class D;
    D * m_d;
  };

}

#endif
