#ifndef RADIANT_COCOAWINDOW_HPP
#define RADIANT_COCOAWINDOW_HPP

#include "Window.hpp"
#include "MultiHead.hpp"

#include <Radiant/Trace.hpp>

#include <Radiant/Platform.hpp>

#ifndef RADIANT_OSX
# error "CocoaWindow only works on OS X"
#endif

namespace Luminous
{

  /// A Cocoa OpenGL Window class
  class CocoaWindow : public Window
  {
  public:
    /// Constructs a new window
    /// @param hint window configuration
    /// @param caption window caption
    CocoaWindow(const MultiHead::Window & window);
    ~CocoaWindow();

    virtual void poll();
    virtual void swapBuffers();
    virtual void makeCurrent();

    virtual void minimize();
    virtual void restore();

    virtual void showCursor(bool visible);

  private:
    class D;
    D * m_d;

  };

}

#endif
