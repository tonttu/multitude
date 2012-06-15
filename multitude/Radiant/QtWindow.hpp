#ifndef RADIANT_QTWINDOW_HPP
#define RADIANT_QTWINDOW_HPP

#include "Export.hpp"
#include "Window.hpp"
#include "WindowConfig.hpp"

namespace Radiant
{
  class GLThreadWidget;
  /// A window class built on top of Qt
  class RADIANT_API QtWindow : public Window
  {
  public:
    /// Constructs a new window
    /// @param hint window configuration
    /// @param caption window caption
    QtWindow(const WindowConfig & hint = WindowConfig(0, 0, 640, 480, false, true, true, 0), const char * caption = 0);
    ~QtWindow();

    virtual void poll();
    virtual void swapBuffers();
    virtual void makeCurrent();

    virtual void minimize();
    virtual void maximize();
    virtual void restore();

    void showCursor(bool visible) OVERRIDE;

  private:
    GLThreadWidget * m_mainWindow;
  };

}

#endif
