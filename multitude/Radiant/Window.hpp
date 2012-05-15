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

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Export.hpp"
#include "WindowEventHook.hpp"

#include <Nimble/Vector2.hpp>

namespace Radiant
{

  /// Virtual base classes for OpenGL windows
  class RADIANT_API Window
  {
  public:
    /// Creates the base definitions for windows
    Window();
    virtual ~Window();

    /// Queries if the window is closed.
    /// This would happen if the user closes the window.
    /// @return true if the window has been closed
    bool isFinished() const;

    /// Sets the full-screen mode of the window
    void setFullscreen(bool fullscreen);

    /// Update window system (mouse & keyboard) events
    virtual void poll() = 0;
    /// Swap OpenGL buffers
    virtual void swapBuffers() = 0;

    /// Sets the OpenGL context for the current thread
    virtual void makeCurrent() = 0;

    /// Returns the width of the window
    int width() const;
    /// Returns the height of the window
    int height() const;

    void setWidth(int w) { m_width = w; }
    void setHeight(int h) { m_height = h; }

    /// Sets the object for sending window events
    virtual void setEventHook(WindowEventHook * hook);
    /// A pointer to the window event callback listener
    WindowEventHook * eventHook() const;

    /// Virtual function for cleaning up window resources
    virtual void deinit() {}

    virtual void minimize() = 0;
    virtual void restore() = 0;

    Nimble::Vector2i position() const { return m_pos; }
    void setPosition(Nimble::Vector2i pos) { m_pos = pos; }

  private:
    ///@cond
    bool m_active;
    bool m_finished;
    bool m_fullscreen;
    int m_width;
    int m_height;
    Nimble::Vector2i m_pos;

    WindowEventHook * m_eventHook;
    ///@endcond
  };

}

#endif
