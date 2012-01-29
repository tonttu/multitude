#ifndef RADIANT_QTWINDOW_HPP
#define RADIANT_QTWINDOW_HPP

#include "Window.hpp"
#include "WindowConfig.hpp"

#include <QGLWidget>

namespace Radiant
{
  class GLThreadWidget;
  /// A window class built on top of Qt
  class QtWindow : public Window
  {
  public:
    /// Constructs a new window
    /// @param hint window configuration
    /// @param caption window caption
    QtWindow(const WindowConfig & hint = WindowConfig(0, 0, 640, 480, false, true, true), const char * caption = 0);
    ~QtWindow();

    virtual void poll();
    virtual void swapBuffers();
    virtual void makeCurrent();

    virtual void minimize();
    virtual void maximize();
    virtual void restore();

    virtual Luminous::GLContext * glContext() { return 0; }

  private:
    GLThreadWidget * m_mainWindow;

  };

}

#endif
