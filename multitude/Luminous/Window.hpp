/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Export.hpp"
#include "WindowEventHook.hpp"

#include <QWindow>

// Only forward declaration to keep OpenGL includes to a minimum to avoid
// conflicts
class QOpenGLContext;

namespace Luminous
{

  /// Virtual base classes for OpenGL windows
  class LUMINOUS_API Window : public QWindow
  {
    Q_OBJECT

  public:
    /// Construct an empty window with zero size
    Window(QScreen* screen);
    /// Destructor
    virtual ~Window();

    /// Swap the back and front buffers of this window
    virtual void swapBuffers();

    /// Sets the OpenGL context for the calling thread
    virtual bool makeCurrent();

    /// Clears the OpenGL context for the calling thread
    virtual void doneCurrent();

    /// Set the event handler for window events. The event handler must remain
    /// valid for the lifetime of the window.
    /// @param hook event handler
    virtual void setEventHook(WindowEventHook * hook);
    /// Get the event handler for the window
    /// @return pointer to the event handler
    WindowEventHook * eventHook() const;

    /// Set the thread-affinity of the OpenGL context in this window to given
    /// thread. This function must be called from the thread that current owns the
    /// context.
    /// @param thread thread to move the OpenGL context to
    void setOpenGLThreadAffinity(QThread* thread);

    /// Attempt to create an OpenGL context for this window with the given
    /// surface format.
    /// @param surfaceFormat surface format for the context
    /// @return true if context
    bool createOpenGLContext(const QSurfaceFormat& surfaceFormat);

    /// Returns the format of the OpenGL context, if the context has been
    /// created.
    /// @return OpenGL context format
    QSurfaceFormat format() const;

  signals:
    void closed();

  protected:
    void exposeEvent(QExposeEvent* ev) override;
    void focusInEvent(QFocusEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;
    void hideEvent(QHideEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;
    void keyReleaseEvent(QKeyEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void moveEvent(QMoveEvent* ev) override;
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void resizeEvent(QResizeEvent* ev) override;
    void showEvent(QShowEvent* ev) override;
    void tabletEvent(QTabletEvent* ev) override;
    void touchEvent(QTouchEvent* ev) override;
    void wheelEvent(QWheelEvent* ev) override;
    bool event(QEvent* ev) override;

  private:
    QScreen* m_screen;
    QOpenGLContext* m_openGLContext;
    WindowEventHook* m_eventHook;
  };

}

#endif
