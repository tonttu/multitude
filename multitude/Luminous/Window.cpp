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

#include <Radiant/PenEvent.hpp>
#include <Radiant/TouchEvent.hpp>
#include <Radiant/Trace.hpp>

#include <QOpenGLContext>
#include <QDropEvent>
#include <QTouchEvent>

#include <cassert>

namespace Luminous
{

  Window::Window(QScreen* screen)
    : QWindow(screen)
    , m_screen(screen)
    // The parent object for the context must be nullptr so its thread-affinity
    // can be changed later.
    , m_openGLContext(new QOpenGLContext(nullptr))
    , m_eventHook(nullptr)
  {
    // This window should be renderable by OpenGL
    setSurfaceType(QSurface::OpenGLSurface);

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
  }

  void Window::setKeyboardFocusOnClick(bool value)
  {
    m_setKeyboardFocusOnClick = value;
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
          Radiant::PenEvent te;
          te.setId(id);
          Nimble::Vector2f loc(info.pointerInfo.ptPixelLocation.x,
                               info.pointerInfo.ptPixelLocation.y);
          Nimble::Vector2f himetric(info.pointerInfo.ptHimetricLocation.x,
                                    info.pointerInfo.ptHimetricLocation.y);

          // Unfortunately the pixel location in the event is in ints. To
          // get subpixel accuracy, we are using himetric units. Those can
          // be converted to source device pixels if we know the input device
          // DPI, which is different from OS screen dpi. But even when doing
          // that convertion, the native input device resolution might not be
          // the same as the screen resolution.
          //
          // Since it's difficult to do correctly, we cheat a bit by just
          // calculating a conversion factor on-fly. Touching with a pen to
          // the right bottom part of the display will then make more
          // accurate calibration.
          //
          // NOTE: this only works for single screen setups (see Redmine #13181)

          if (loc.x > m_himetricCalibrationMax.x) {
            m_himetricCalibrationMax.x = loc.x;
            m_himetricFactor.x = loc.x / himetric.x;
          }

          if (loc.y > m_himetricCalibrationMax.y) {
            m_himetricCalibrationMax.y = loc.y;
            m_himetricFactor.y = loc.y / himetric.y;
          }

          if (m_himetricFactor.x > 0) {
            loc.x = himetric.x * m_himetricFactor.x;
            // If the error is more than one pixel, we calculated something wrong!
            if (std::abs(loc.x - info.pointerInfo.ptPixelLocation.x) > 1.f) {
              m_himetricFactor.x = 0;
              m_himetricCalibrationMax.x = 0;
              loc.x = info.pointerInfo.ptPixelLocation.x;
            }
          }
          if (m_himetricFactor.y > 0) {
            loc.y = himetric.y * m_himetricFactor.y;
            if (std::abs(loc.y - info.pointerInfo.ptPixelLocation.y) > 1.f) {
              m_himetricFactor.y = 0;
              m_himetricCalibrationMax.y = 0;
              loc.y = info.pointerInfo.ptPixelLocation.y;
            }
          }

          // mapFromGlobal uses only ints, but in our case this is the same thing
          loc.x -= position().x();
          loc.y -= position().y();
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

          te.setSourceDevice(uint64_t(info.pointerInfo.sourceDevice));

          if (m_eventHook) {
            m_eventHook->penEvent(te);
            return true;
          }
        }
        else if (type==PT_TOUCH && GetPointerTouchInfo(id, &touchInfo)) {
          // We don't care about hover events
          if (msg->message == WM_POINTERUPDATE &&
              (touchInfo.pointerInfo.pointerFlags & POINTER_FLAG_INCONTACT) == 0)
            return true;

          Nimble::Vector2f loc(touchInfo.pointerInfo.ptPixelLocation.x,
                               touchInfo.pointerInfo.ptPixelLocation.y);
          Nimble::Vector2f himetric(touchInfo.pointerInfo.ptHimetricLocation.x,
                                    touchInfo.pointerInfo.ptHimetricLocation.y);

          // See above for information on himetric adjustment.
          //
          // NOTE: this only works for single screen setups (see Redmine #13181)

          if (loc.x > m_himetricCalibrationMax.x) {
            m_himetricCalibrationMax.x = loc.x;
            m_himetricFactor.x = loc.x / himetric.x;
          }

          if (loc.y > m_himetricCalibrationMax.y) {
            m_himetricCalibrationMax.y = loc.y;
            m_himetricFactor.y = loc.y / himetric.y;
          }

          if (m_himetricFactor.x > 0) {
            loc.x = himetric.x * m_himetricFactor.x;
            // If the error is more than one pixel, we calculated something wrong!
            if (std::abs(loc.x - touchInfo.pointerInfo.ptPixelLocation.x) > 1.f) {
              m_himetricFactor.x = 0;
              m_himetricCalibrationMax.x = 0;
              loc.x = touchInfo.pointerInfo.ptPixelLocation.x;
            }
          }
          if (m_himetricFactor.y > 0) {
            loc.y = himetric.y * m_himetricFactor.y;
            if (std::abs(loc.y - touchInfo.pointerInfo.ptPixelLocation.y) > 1.f) {
              m_himetricFactor.y = 0;
              m_himetricCalibrationMax.y = 0;
              loc.y = touchInfo.pointerInfo.ptPixelLocation.y;
            }
          }

          // mapFromGlobal uses only ints, but in our case this is the same thing
          loc.x -= position().x();
          loc.y -= position().y();
          Radiant::TouchEvent::Type type = Radiant::TouchEvent::TOUCH_UPDATE;

          if (msg->message == WM_POINTERDOWN) {
            type = Radiant::TouchEvent::TOUCH_BEGIN;
          } else if (msg->message == WM_POINTERUP) {
            type = Radiant::TouchEvent::TOUCH_END;
          }

          if (m_eventHook) {
            Radiant::TouchEvent event(id, type, loc);
            m_eventHook->touchEvent(event);
            return true;
          }
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
        Radiant::TouchEvent touch(p.id(), type, Nimble::Vector2f(p.pos().x(), p.pos().y()));
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
