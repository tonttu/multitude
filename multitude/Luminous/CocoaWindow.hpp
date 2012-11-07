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

  /// This class represents an OpenGL window using Cocoa on OS X.
  class CocoaWindow : public Window
  {
  public:
    /// Constructs a new window
    /// @param window window configuration
    CocoaWindow(const MultiHead::Window & window);
    ~CocoaWindow();

    virtual void poll() OVERRIDE;
    virtual void swapBuffers() OVERRIDE;
    virtual void makeCurrent() OVERRIDE;

    virtual void minimize() OVERRIDE;
    virtual void maximize() OVERRIDE;
    virtual void restore() OVERRIDE;

    virtual void showCursor(bool visible);

  private:
    class D;
    D * m_d;

  };

}

#endif
