#ifndef RADIANT_COCOAWINDOW_HPP
#define RADIANT_COCOAWINDOW_HPP

#include "Window.hpp"
#include "WindowConfig.hpp"

#include <Radiant/Trace.hpp>

namespace Radiant
{

  /// A Cocoa OpenGL Window class
  class CocoaWindow : public Window
  {
  public:
    /// Constructs a new window
    /// @param hint window configuration
    /// @param caption window caption
    CocoaWindow(const WindowConfig & hint = WindowConfig(0, 0, 640, 480, false, true, true));
    ~CocoaWindow();

    virtual void poll();
    virtual void swapBuffers();
    virtual void makeCurrent();

    virtual void minimize();
    virtual void restore();

    virtual Luminous::GLContext * glContext() {return 0;}
  private:
    class D;
    D * m_d;

  };

}

#endif
