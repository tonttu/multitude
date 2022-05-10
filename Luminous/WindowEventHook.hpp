/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_WINDOWEVENTHOOK_HPP
#define LUMINOUS_WINDOWEVENTHOOK_HPP

#include <QtGlobal>

namespace Radiant
{
  class PenEvent;
  class TouchEvent;
}

class QExposeEvent;
class QFocusEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
class QResizeEvent;
class QShowEvent;
class QTabletEvent;
class QWheelEvent;
class QMoveEvent;
class QDropEvent;
class QEvent;

namespace Luminous
{
  class Window;

  /// Class for getting window events
  class WindowEventHook
  {
  public:
    WindowEventHook() : m_window(nullptr) {}
    virtual ~WindowEventHook() {}

    virtual void exposeEvent(QExposeEvent* ev) { Q_UNUSED(ev); }
    virtual void focusInEvent(QFocusEvent* ev) { Q_UNUSED(ev); }
    virtual void focusOutEvent(QFocusEvent* ev) { Q_UNUSED(ev); }
    virtual void hideEvent(QHideEvent* ev) { Q_UNUSED(ev); }
    virtual void keyPressEvent(QKeyEvent* ev) { Q_UNUSED(ev); }
    virtual void keyReleaseEvent(QKeyEvent* ev) { Q_UNUSED(ev); }
    virtual void mouseDoubleClickEvent(QMouseEvent* ev) { Q_UNUSED(ev); }
    virtual void mouseMoveEvent(QMouseEvent* ev) { Q_UNUSED(ev); }
    virtual void mousePressEvent(QMouseEvent* ev) { Q_UNUSED(ev); }
    virtual void mouseReleaseEvent(QMouseEvent* ev) { Q_UNUSED(ev); }
    virtual void moveEvent(QMoveEvent* ev) { Q_UNUSED(ev); }
    virtual bool nativeEvent(const QByteArray& eventType, void* message, long* result)
    {
      Q_UNUSED(eventType);
      Q_UNUSED(message);
      Q_UNUSED(result);

      return false;
    }

    virtual void resizeEvent(QResizeEvent* ev) { Q_UNUSED(ev); }
    virtual void showEvent(QShowEvent* ev) { Q_UNUSED(ev); }
    virtual void penEvent(const Radiant::PenEvent & ev) { Q_UNUSED(ev); }
    virtual void touchEvent(const Radiant::TouchEvent & ev) { Q_UNUSED(ev); }
    virtual void wheelEvent(QWheelEvent* ev) { Q_UNUSED(ev); }
    virtual void dropEvent(QDropEvent* ev) { Q_UNUSED(ev); }
    virtual bool event(QEvent* ev) { Q_UNUSED(ev); return false; }

    /// Time since last keyboard or mouse activity
    virtual double lastActivity() const = 0;

    void setWindow(Window* window) { m_window = window; }
    Window* window() { return m_window; }

  protected:
    Window* m_window;
  };

}

#endif // WINDOWEVENTHOOK_HPP
