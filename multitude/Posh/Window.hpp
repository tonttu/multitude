#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "WindowEventHook.hpp"

namespace Posh
{

  /// Virtual base classes for OpenGL windows
  class Window
  {
  public:
    /// Creates the base definitions for windows
    Window();
    ~Window();

    /// Queries if the window is closed.
    /** This would happen if the user closes the window. */
    bool isFinished() const;

    /// Update window system (mouse & keyboard) events
    void poll();
    /// Swap OpenGL buffers
    void swapBuffers();

    /// Sets the OpenGL context for the current thread
    void makeCurrent();

    /// Returns the width of the window
    int width() const;
    /// Returns the height of the window
    int height() const;

    /// Sets the object for sending window events
    void setEventHook(WindowEventHook * hook);
    /// A pointer to the window event callback listener
    WindowEventHook * eventHook() const;

    /// Virtual function for cleaning up window resources
    void deinit();

  private:
    class D;
    D * m_d;
  };

}

#endif
