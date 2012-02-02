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
#include "Mutex.hpp"
#include "TimeStamp.hpp"
#include "Window.hpp"
#include "WindowConfig.hpp"

#include <QString>

#include <GL/glx.h>

#include <Luminous/GLContext.hpp>

namespace Radiant
{
  /** X11 OpenGL window class. */
  class RADIANT_API XWindow : public Radiant::Window
  {
  public:
    /// Creates a new XWindow
    XWindow(const WindowConfig& hint = WindowConfig(0, 0, 640, 480, false, false, true),
            const char* caption = 0);
    ~XWindow();

    virtual void poll();
    virtual void swapBuffers();
    virtual void makeCurrent();

    virtual Luminous::GLContext * glContext() { return m_context; }
    virtual void deinit();

    virtual void minimize();
    virtual void restore();

    virtual void setEventHook(WindowEventHook * hook);

  private:
    void mapWindow();

    class X11GLContext : public Luminous::GLContext
    {
    public:
      X11GLContext(Display     * display,
                   GLXContext    sharecontext,
                   XVisualInfo * visualInfo,
                   GLXWindow     drawable);
      virtual ~X11GLContext();

      virtual void makeCurrent();
      virtual GLContext * createSharedContext();
      virtual Radiant::Mutex * mutex();


    private:
      Display     * m_display;
      GLXContext    m_context;
      XVisualInfo * m_visualInfo;
      GLXWindow     m_drawable;
      Radiant::Mutex * m_mutex;
    };

    Display* m_display;
    GLXWindow m_drawable;

    Radiant::TimeStamp m_lastAction;

    X11GLContext * m_context;

    void showCursor(bool show);
  };

}

#endif
