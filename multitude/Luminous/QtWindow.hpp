/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

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
