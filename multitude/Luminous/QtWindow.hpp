#ifndef LUMINOUS_QTWINDOW_HPP
#define LUMINOUS_QTWINDOW_HPP

#include "Export.hpp"
#include "Window.hpp"
#include "MultiHead.hpp"

namespace Luminous
{
  /// A window class built on top of Qt
  class LUMINOUS_API QtWindow : public Window
  {
  public:
    /// Constructs a new window
    /// @param window window configuration
    /// @param windowTitle window title
    QtWindow(const MultiHead::Window & window, const QString & windowTitle);
    ~QtWindow();

    virtual void poll() OVERRIDE;
    virtual void swapBuffers() OVERRIDE;
    virtual void makeCurrent() OVERRIDE;

    virtual void minimize() OVERRIDE;
    virtual void maximize() OVERRIDE;
    virtual void restore() OVERRIDE;

    void showCursor(bool visible) OVERRIDE;

  private:
    class D;
    D * m_d;
  };

}

#endif
