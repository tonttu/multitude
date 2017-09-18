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

#include <Radiant/Mutex.hpp>
#include <Radiant/PenEvent.hpp>
#include <Radiant/TouchEvent.hpp>
#include <Radiant/Trace.hpp>

#include <QOpenGLContext>
#include <QDropEvent>
#include <QTouchEvent>

#include <cassert>

#ifdef RADIANT_WINDOWS
#include <Dwmapi.h>

static int64_t performanceCounterFrequency = 0;
#endif

namespace Luminous
{

  Window::Window(QScreen* screen)
    : QWindow(screen)
    // The parent object for the context must be nullptr so its thread-affinity
    // can be changed later.
    , m_openGLContext(new QOpenGLContext(nullptr))
    , m_eventHook(nullptr)
  {
    // This window should be renderable by OpenGL
    setSurfaceType(QSurface::OpenGLSurface);

    m_openGLContext->setScreen(screen);

#ifdef RADIANT_WINDOWS
    MULTI_ONCE {
      LARGE_INTEGER tmp;
      if (QueryPerformanceFrequency(&tmp))
        performanceCounterFrequency = tmp.QuadPart;
    }
#endif
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
    m_openGLContext->doneCurrent();
    m_openGLContext->moveToThread(thread);
  }

  bool Window::createOpenGLContext()
  {
    m_openGLContext->setFormat(format());
    return m_openGLContext->create();
  }

  QOpenGLContext * Window::context() const
  {
    return m_openGLContext;
  }

  void Window::doneCurrent()
  {
    m_openGLContext->doneCurrent();
  }

  bool Window::makeCurrent()
  {
    bool ok = m_openGLContext->makeCurrent(this);

    if(!ok)
      Radiant::error("Window::makeCurrent # failed to make OpenGL context current (context is valid: %d)", m_openGLContext->isValid());

    return ok;
  }

  void Window::swapBuffers()
  {
    m_openGLContext->swapBuffers(this);

#ifdef RADIANT_WINDOWS
    if (!m_uncloak) {
      m_uncloak = true;
      auto v = FALSE;
      DwmSetWindowAttribute((HWND)winId(), DWMWA_CLOAK, &v, sizeof(v));
    }
#endif
  }

  void Window::setKeyboardFocusOnClick(bool value)
  {
    m_setKeyboardFocusOnClick = value;
  }

  Window::QWindowLock Window::lock()
  {
    Radiant::Guard g(m_windowChangeMutex);
    return QWindowLock(this, std::move(g));
  }

  void Window::exposeEvent(QExposeEvent* ev)
  {
    if(m_eventHook)
      m_eventHook->exposeEvent(ev);
#ifdef RADIANT_WINDOWS
    // Disable native touch feedback
    BOOL value = false;
    for (int i = FEEDBACK_TOUCH_CONTACTVISUALIZATION; i <= FEEDBACK_GESTURE_PRESSANDTAP; ++i)
      SetWindowFeedbackSetting((HWND)winId(), FEEDBACK_TYPE(i), 0, sizeof(BOOL), &value);

    if (!m_uncloak) {
      auto v = TRUE;
      DwmSetWindowAttribute((HWND)winId(), DWMWA_CLOAK, &v, sizeof(v));
    }
#endif
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
    // Request keyboard focus to this window if the window manager is not handling it
    if (m_setKeyboardFocusOnClick && !isActive()) {
      requestActivate();
    }

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
#ifdef RADIANT_WINDOWS
    if (eventType == "windows_generic_MSG") {
      MSG * msg = static_cast<MSG*>(message);
      if (msg->message == WM_POINTERDOWN ||
          msg->message == WM_POINTERUPDATE ||
          msg->message == WM_POINTERUP) {
        auto id = GET_POINTERID_WPARAM(msg->wParam);
        POINTER_INPUT_TYPE type{PT_POINTER};
        POINTER_PEN_INFO info;
        POINTER_TOUCH_INFO touchInfo;
        GetPointerType(id, &type);
        if (type==PT_PEN && GetPointerPenInfo(id, &info)) {
          m_pointerInfo.resize(info.pointerInfo.historyCount);
          if (info.pointerInfo.historyCount > 1) {
            uint32_t count = info.pointerInfo.historyCount;
            if (GetPointerInfoHistory(id, &count, m_pointerInfo.data())) {
              m_pointerInfo.resize(count);
            } else {
              m_pointerInfo.resize(1);
              m_pointerInfo[0] = info.pointerInfo;
            }
          } else {
            m_pointerInfo[0] = info.pointerInfo;
          }

          for (int i = (int)m_pointerInfo.size() - 1; i >= 0; --i) {
            POINTER_INFO & ptr = m_pointerInfo[i];

            // We don't care about hover events
            if (msg->message == WM_POINTERUPDATE &&
                (ptr.pointerFlags & POINTER_FLAG_INCONTACT) == 0)
              return true;

            Radiant::PenEvent te;
            te.setId(id);
            Nimble::Vector2f loc(ptr.ptPixelLocation.x,
                                 ptr.ptPixelLocation.y);
            Nimble::Vector2f himetric(ptr.ptHimetricLocationRaw.x,
                                      ptr.ptHimetricLocationRaw.y);

            te.setLocation(loc);

            Radiant::PenEvent::Flags flags = Radiant::PenEvent::FLAG_NONE;
            if (info.penMask & PEN_MASK_PRESSURE) {
              flags |= Radiant::PenEvent::FLAG_PRESSURE;
              te.setPressure(info.pressure / 1024.f);
            }
            if (info.penMask & PEN_MASK_ROTATION) {
              flags |= Radiant::PenEvent::FLAG_ROTATION;
              te.setRotation(Nimble::Math::degToRad(info.rotation));
            }
            Nimble::Vector2f tilt{0, 0};
            if (info.penMask & PEN_MASK_TILT_X) {
              flags |= Radiant::PenEvent::FLAG_TILT_X;
              tilt.x = Nimble::Math::degToRad(info.tiltX);
            }
            if (info.penMask & PEN_MASK_TILT_Y) {
              flags |= Radiant::PenEvent::FLAG_TILT_Y;
              tilt.y = Nimble::Math::degToRad(info.tiltY);
            }
            if (info.penFlags & PEN_FLAG_BARREL) {
              flags |= Radiant::PenEvent::FLAG_BARREL;
            }
            if (info.penFlags & PEN_FLAG_INVERTED) {
              flags |= Radiant::PenEvent::FLAG_INVERTED;
            }
            if (info.penFlags & PEN_FLAG_ERASER) {
              flags |= Radiant::PenEvent::FLAG_ERASER;
            }
            te.setTilt(tilt);
            te.setFlags(flags);

            if (msg->message == WM_POINTERDOWN) {
              te.setType(Radiant::PenEvent::TYPE_DOWN);
            } else if (msg->message == WM_POINTERUP) {
              te.setType(Radiant::PenEvent::TYPE_UP);
            } else {
              te.setType(Radiant::PenEvent::TYPE_UPDATE);
            }

            te.setSourceDevice(uint64_t(ptr.sourceDevice));
            te.setRawLocation(himetric);
            te.setTime(double(ptr.PerformanceCount) / performanceCounterFrequency);

            if (m_eventHook) {
              m_eventHook->penEvent(te);
            }
          }
          return true;
        }
        else if (type==PT_TOUCH && GetPointerTouchInfo(id, &touchInfo)) {
          m_pointerInfo.resize(touchInfo.pointerInfo.historyCount);
          if (touchInfo.pointerInfo.historyCount > 1) {
            uint32_t count = touchInfo.pointerInfo.historyCount;
            if (GetPointerInfoHistory(id, &count, m_pointerInfo.data())) {
              m_pointerInfo.resize(count);
            } else {
              m_pointerInfo.resize(1);
              m_pointerInfo[0] = touchInfo.pointerInfo;
            }
          } else {
            m_pointerInfo[0] = touchInfo.pointerInfo;
          }

          for (int i = (int)m_pointerInfo.size() - 1; i >= 0; --i) {
            POINTER_INFO & ptr = m_pointerInfo[i];
            if (msg->message == WM_POINTERUPDATE &&
                (ptr.pointerFlags & POINTER_FLAG_INCONTACT) == 0)
              continue;

            Nimble::Vector2f loc(ptr.ptPixelLocation.x,
                                 ptr.ptPixelLocation.y);
            Nimble::Vector2f himetric(ptr.ptHimetricLocationRaw.x,
                                      ptr.ptHimetricLocationRaw.y);

            Radiant::TouchEvent::Type type = Radiant::TouchEvent::TOUCH_UPDATE;

            if (msg->message == WM_POINTERDOWN) {
              type = Radiant::TouchEvent::TOUCH_BEGIN;
            } else if (msg->message == WM_POINTERUP) {
              type = Radiant::TouchEvent::TOUCH_END;
            }

            if (m_eventHook) {
              Radiant::TouchEvent event(id, type, loc);
              event.setRawLocation(himetric);
              event.setSourceDevice(uint64_t(ptr.sourceDevice));
              event.setTime(double(ptr.PerformanceCount) / performanceCounterFrequency);
              m_eventHook->touchEvent(event);
            }
          }
          return true;
        }
      }
    }

#endif

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
    if (m_eventHook)
      m_eventHook->penEvent(*ev);
  }

  void Window::touchEvent(QTouchEvent* ev)
  {
    if(m_eventHook) {
      for (auto & p: ev->touchPoints()) {
        Radiant::TouchEvent::Type type = Radiant::TouchEvent::TOUCH_UPDATE;
        if (p.state() == Qt::TouchPointPressed)
          type = Radiant::TouchEvent::TOUCH_BEGIN;
        else if (p.state() == Qt::TouchPointReleased)
          type = Radiant::TouchEvent::TOUCH_END;
        Radiant::TouchEvent touch(p.id(), type, Nimble::Vector2f(p.screenPos().x(), p.screenPos().y()));
        m_eventHook->touchEvent(touch);
      }
    }
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
