#ifndef LUMINOUS_QTWINDOW_HPP
#define LUMINOUS_QTWINDOW_HPP

#include "Export.hpp"
#include "Window.hpp"
#include "MultiHead.hpp"

namespace Luminous
{
  class GLThreadWidget;

  /// A window class built on top of Qt
  class LUMINOUS_API QtWindow : public Window
  {
  public:
    /// Constructs a new window
    /// @param window window configuration
    /// @param windowTitle window title
    QtWindow(const MultiHead::Window & window, const QString & windowTitle);
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
