/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Export.hpp"
#include "WindowEventHook.hpp"

#include <Nimble/Vector2.hpp>

namespace Luminous
{

  /// Virtual base classes for OpenGL windows
  class LUMINOUS_API Window
  {
  public:
    /// Construct an empty window with zero size
    Window();
    /// Destructor
    virtual ~Window();

    /// Queries if the window is closed.
    /// This would happen if the user closes the window.
    /// @return true if the window has been closed
    bool isFinished() const;

    /// Sets the full-screen mode of the window
    /// @param fullscreen true to enable fullscreen mode; false to disable it
    void setFullscreen(bool fullscreen);

    /// Update window system (mouse & keyboard) events
    virtual void poll() = 0;
    /// Swap OpenGL buffers
    virtual void swapBuffers() = 0;

    /// Sets the OpenGL context for the calling thread
    virtual void makeCurrent() = 0;

    /// Clears the OpenGL context for the calling thread
    virtual void doneCurrent() = 0;

    /// This function can be used to perform any initialization that must be
    /// performed in the main-thread.
    virtual bool mainThreadInit() = 0;

    /// Returns the width of the window
    /// @return width of the window in pixels
    int width() const;
    /// Returns the height of the window
    /// @return height of the window in pixels
    int height() const;

    /// Set the width of the window
    /// @param w window width in pixels
    void setWidth(int w) { m_width = w; }
    /// Set the height of the window
    /// @param h window height in pixels
    void setHeight(int h) { m_height = h; }

    /// Set the event handler for window events. The event handler must remain
    /// valid for the lifetime of the window.
    /// @param hook event handler
    virtual void setEventHook(WindowEventHook * hook);
    /// Get the event handler for the window
    /// @return pointer to the event handler
    WindowEventHook * eventHook() const;

    /// This function can be used to perform any initialization that must be
    /// executed in the render-thread associated with the window.
    virtual void init() {}

    /// Cleanup any window resources. Default implementation does nothing.
    virtual void deinit() {}

    /// Minimize the window
    virtual void minimize() = 0;
    /// Maximize the window
    virtual void maximize() = 0;
    /// Restore the window from minimized state
    virtual void restore() = 0;

    /// Get the window position
    /// @return window position
    Nimble::Vector2i position() const { return m_pos; }
    /// Set the window position
    /// @param pos window position
    void setPosition(Nimble::Vector2i pos) { m_pos = pos; }

    /// Control mouse cursor visibility
    /// @param visible true to show cursor; false to hide it
    virtual void showCursor(bool visible) = 0;

  private:
    bool m_finished;
    bool m_fullscreen;
    int m_width;
    int m_height;
    Nimble::Vector2i m_pos;

    WindowEventHook * m_eventHook;
  };

}

#endif
