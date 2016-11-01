/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "Window.hpp"

#include <Radiant/Trace.hpp>

#include <QOpenGLContext>
#include <QDropEvent>

#include <cassert>

namespace Luminous
{

  Window::Window(QScreen* screen)
    : QWindow(screen)
    , m_screen(screen)
    , m_openGLContext(nullptr)
    , m_eventHook(nullptr)
  {
    // This window should be renderable by OpenGL
    setSurfaceType(QSurface::OpenGLSurface);

    // The parent object for the context must be nullptr so its thread-affinity
    // can be changed later.
    m_openGLContext = new QOpenGLContext(nullptr);
    m_openGLContext->setScreen(m_screen);
  }

  Window::~Window()
  {
  }

  void Window::setEventHook(WindowEventHook* hook)
  {
    if(m_eventHook)
      m_eventHook->setWindow(nullptr);

    m_eventHook = hook;

    if(m_eventHook)
      m_eventHook->setWindow(this);
  }

  WindowEventHook*Window::eventHook() const
  {
    return m_eventHook;
  }

  void Window::setOpenGLThreadAffinity(QThread* thread)
  {
    assert(m_openGLContext);
    m_openGLContext->doneCurrent();
    m_openGLContext->moveToThread(thread);
  }

  bool Window::createOpenGLContext(const QSurfaceFormat& surfaceFormat)
  {
    m_openGLContext->setFormat(surfaceFormat);

    bool result = m_openGLContext->create();
    if(!result) {
      Radiant::error("Window::createOpenGLContext # failed to create OpenGL context");
    }

    return result;
  }

  QSurfaceFormat Window::format() const
  {
    assert(m_openGLContext);
    return m_openGLContext->format();
  }

  void Window::doneCurrent()
  {
    assert(m_openGLContext);
    m_openGLContext->doneCurrent();
  }

  bool Window::makeCurrent()
  {
    assert(m_openGLContext);
    bool ok = m_openGLContext->makeCurrent(this);

    if(!ok)
      Radiant::error("Window::makeCurrent # failed to make OpenGL context current (context is valid: %d)", m_openGLContext->isValid());

    return ok;
  }

  void Window::swapBuffers()
  {
    assert(m_openGLContext);
    m_openGLContext->swapBuffers(this);
  }

  void Window::exposeEvent(QExposeEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->exposeEvent(ev);
  }

  void Window::focusInEvent(QFocusEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->focusInEvent(ev);
  }

  void Window::focusOutEvent(QFocusEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->focusInEvent(ev);
  }

  void Window::hideEvent(QHideEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->hideEvent(ev);
  }

  void Window::keyPressEvent(QKeyEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->keyPressEvent(ev);
  }

  void Window::keyReleaseEvent(QKeyEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->keyReleaseEvent(ev);
  }

  void Window::mouseDoubleClickEvent(QMouseEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->mouseDoubleClickEvent(ev);
  }

  void Window::mouseMoveEvent(QMouseEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->mouseMoveEvent(ev);
  }

  void Window::mousePressEvent(QMouseEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->mousePressEvent(ev);
  }

  void Window::mouseReleaseEvent(QMouseEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->mouseReleaseEvent(ev);
  }

  void Window::moveEvent(QMoveEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->moveEvent(ev);
  }

  bool Window::nativeEvent(const QByteArray& eventType, void* message, long* result)
  {
    if(m_eventHook)
      return m_eventHook->nativeEvent(eventType, message, result);

    return false;
  }

  void Window::resizeEvent(QResizeEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->resizeEvent(ev);
  }

  void Window::showEvent(QShowEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->showEvent(ev);
  }

  void Window::tabletEvent(QTabletEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->tabletEvent(ev);
  }

  void Window::touchEvent(QTouchEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->touchEvent(ev);
  }

  void Window::wheelEvent(QWheelEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->wheelEvent(ev);
  }

  bool Window::event(QEvent* ev)
  {
    bool processed = false;

    switch(ev->type()) {
      case QEvent::DragEnter:
        // We accept any kind of drag event on our windows
        ev->accept();
        break;
      case QEvent::Drop:
        if(m_eventHook)
          m_eventHook->dropEvent(dynamic_cast<QDropEvent*>(ev));
        break;
      case QEvent::Close:
        emit closed();
        break;
      default:
        break;
    }

    if(m_eventHook)
      processed = m_eventHook->event(ev);

    if(!processed) {
      processed = QWindow::event(ev);
    }

    return processed;
  }

}
