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

    virtual bool mainThreadInit() OVERRIDE;

    virtual void minimize() OVERRIDE;
    virtual void maximize() OVERRIDE;
    virtual void restore() OVERRIDE;

    void showCursor(bool visible) OVERRIDE;

    virtual int width() const OVERRIDE;
    virtual void setWidth(int w) OVERRIDE;

    virtual int height() const OVERRIDE;
    virtual void setHeight(int h) OVERRIDE;

    virtual Nimble::Vector2i position() const OVERRIDE;
    virtual void setPosition(Nimble::Vector2i pos) OVERRIDE;

    virtual void doneCurrent() OVERRIDE;

    virtual bool setIcon(const QString & filename) OVERRIDE;
    virtual unsigned gpuId() const OVERRIDE;

  private:
    class D;
    D * m_d;
  };

}

#endif
