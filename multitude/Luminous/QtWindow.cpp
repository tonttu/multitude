/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include <Radiant/Platform.hpp>

#include "QtWindow.hpp"
#include "WindowEventHook.hpp"

#include <Radiant/DropEvent.hpp>
#include <Radiant/KeyEvent.hpp>
#include <Radiant/TabletEvent.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/TouchEvent.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/TimeStamp.hpp>

#include <QApplication>
#include <QGLWidget>
#include <QMouseEvent>
#include <QDesktopWidget>
#include <QGLFormat>

namespace Luminous
{

  class GLThreadWidget : public QGLWidget
  {
  public:
    QtWindow & m_window;
    const MultiHead::Window & m_windowDef;
    Radiant::TimeStamp m_lastMouseAction;

    GLThreadWidget(const QGLFormat & format, QWidget * host, QtWindow & window, Qt::WindowFlags flags,
                   const MultiHead::Window & windowDef)
      : QGLWidget(format, host, 0, flags)
      , m_window(window)
      , m_windowDef(windowDef)
    {
      // Needed for key events on Windows
      setFocusPolicy(Qt::StrongFocus);

      // Make the widget receive mouse move events even if no buttons are pressed
      setMouseTracking(true);
      // Allow drop-events, so that people can drop files on the widget
      setAcceptDrops(true);

      // Display some debug information if requested
      const auto f = this->format();

      // Accept touch events
      setAttribute(Qt::WA_AcceptTouchEvents);
    }

    virtual void showCursor(bool visible)
    {
      if (visible)
        setCursor( QCursor( Qt::ArrowCursor) );
      else
        setCursor( QCursor( Qt::BlankCursor) );
    }

  private:
    // Empty overrides for thread-safety
    virtual void paintEvent(QPaintEvent *) OVERRIDE {}

    virtual void resizeEvent ( QResizeEvent * e ) OVERRIDE
    {
      if(m_window.eventHook()) {
        m_window.eventHook()->handleWindowMove( pos().x(), pos().y(), e->size().width(), e->size().height() );
        e->accept();
      }
    }

    virtual void moveEvent ( QMoveEvent * e ) OVERRIDE
    {
      if(m_window.eventHook()) {
        m_window.eventHook()->handleWindowMove( e->pos().x(), e->pos().y(), size().width(), size().height() );
        e->accept();
      }
    }

    virtual void mouseMoveEvent(QMouseEvent * e) OVERRIDE
    {
      if(m_window.eventHook()) {
        m_window.eventHook()->handleMouseEvent(*e);
        e->accept();
      }

      m_lastMouseAction = Radiant::TimeStamp::currentTime();
    }

    virtual void mousePressEvent(QMouseEvent * e) OVERRIDE
    {
      // If we are running in frameless mode, we must explicitly make the
      // window active so it gets keyboard focus
      if(m_windowDef.frameless())
        activateWindow();

      if(m_window.eventHook()) {
        m_window.eventHook()->handleMouseEvent(*e);
        e->accept();
      }
    }

    virtual void mouseReleaseEvent(QMouseEvent * e) OVERRIDE
    {
      if(m_window.eventHook()) {
        m_window.eventHook()->handleMouseEvent(*e);
        e->accept();
      }
    }

    virtual void wheelEvent(QWheelEvent *e) OVERRIDE
    {
      if(m_window.eventHook()) {
        m_window.eventHook()->handleMouseEvent(Radiant::MouseEvent(*e));
        e->accept();
      }
    }

    virtual void keyPressEvent(QKeyEvent * e) OVERRIDE
    {
      // All done if there's no hooks installed
      if(m_window.eventHook()) {
        m_window.eventHook()->handleKeyboardEvent(Radiant::KeyEvent(*e));
        e->accept();
      }
    }


    virtual void keyReleaseEvent(QKeyEvent * e) OVERRIDE
    {
      if(m_window.eventHook()) {
        m_window.eventHook()->handleKeyboardEvent(Radiant::KeyEvent(*e));
        e->accept();
      }
    }

    virtual void tabletEvent(QTabletEvent * e) OVERRIDE
    {
      // All done if there's no hooks installed
      if(m_window.eventHook()) {
        m_window.eventHook()->handleTabletEvent(Radiant::TabletEvent(*e));
        e->accept();
      }
    }

    virtual void dropEvent(QDropEvent *de) OVERRIDE
    {
      if(m_window.eventHook()) {
        bool ok = true;

        QPoint onWindow = de->pos();
        Nimble::Vector2 ongraphics = m_windowDef.windowToGraphics
            (Nimble::Vector2(onWindow.x(), onWindow.y()), ok);
        if(ok) {
          m_window.eventHook()->handleDropEvent(Radiant::DropEvent(*de, ongraphics));
          de->accept();
        }
      }
    }

    virtual void dragEnterEvent(QDragEnterEvent *e) OVERRIDE
    {
      // We accept all kinds of drops
      e->acceptProposedAction();
    }

    virtual bool event(QEvent * e) OVERRIDE
    {
      QTouchEvent * te = dynamic_cast<QTouchEvent *>(e);

      if(!te) {
        return QGLWidget::event(e);
      }

      /*const QList<QTouchEvent::TouchPoint> & points = te->touchPoints();

      for(auto it = points.begin(); it != points.end(); it++) {

      }
      */

      m_window.eventHook()->handleTouchEvent(Radiant::TouchEvent(*te));
      te->accept();
      return true;
    }
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  class QtWindow::D
  {
  public:
    D()
      : m_mainWindow(0)
      , m_glWidget(0)
      , m_deferredActivateWindow(false)
      , m_raiseCount(0)
  #ifdef RADIANT_WINDOWS
      , m_dc(nullptr)
      , m_rc(nullptr)
  #endif
    {}

    ~D()
    {
#ifdef RADIANT_WINDOWS
      if(m_glWidget)
        m_glWidget->releaseDC(m_dc);
#endif
      delete m_mainWindow;
    }

    // Get the host widget for our OpenGL context
    static QWidget * getHostWidget(int screenNumber, Qt::WindowFlags flags)
    {
      QDesktopWidget * desktop = QApplication::desktop();

      // Make sure the screen number is valid. Fall back to default screen if not.
      if(screenNumber >= desktop->screenCount()) {
        Radiant::error("Request to create window on screen %d, but only %d screens detected. Using default screen instead.", screenNumber, desktop->screenCount());
        screenNumber = -1;
      }

      QWidget * parent = desktop->screen(screenNumber);

      return new QWidget(parent, flags);
    }

    QWidget * m_mainWindow;
    GLThreadWidget * m_glWidget;
    bool m_deferredActivateWindow;
    int m_raiseCount;
#ifdef RADIANT_WINDOWS
    HDC m_dc;
    HGLRC m_rc;
#endif
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  QtWindow::QtWindow(const MultiHead::Window & window, const QString & windowTitle)
    : Window()
    , m_d(new D())
  {

    // The code below opens a new OpenGL window at desired location. Extra
    // steps are taken to ensure that the window creation happens so that:
    //
    // 1) A dummy window is created, and moved to the right location, with
    // right size etc.
    //
    // 2) An OpenGL widget is opened at this correct location.
    //
    // The purpose of this exercise is that that when using AMD GPUs, then AMD
    // driver selects the GPU for the OpenGL context based on window location,
    // when the context is created. Choosing the wrong GPU can cause massive
    // performance penalty. Similar behavior has been witnessed on OS X.

    Qt::WindowFlags flags = 0;

    if(window.frameless()) {
      flags |= Qt::FramelessWindowHint;
      flags |= Qt::X11BypassWindowManagerHint;
    }

    m_d->m_mainWindow = m_d->getHostWidget(window.screennumber(), flags);

    if(!windowTitle.isEmpty())
      m_d->m_mainWindow->setWindowTitle(windowTitle);

    if(window.screen()->iconify())
       m_d->m_mainWindow->setWindowState(Qt::WindowMinimized);

    m_d->m_mainWindow->move(window.location().x, window.location().y);
    m_d->m_mainWindow->resize(window.width(), window.height());
    m_d->m_mainWindow->raise();
    m_d->m_mainWindow->show();

    if(window.fullscreen())
      m_d->m_mainWindow->showFullScreen();

    QGLFormat format = QGLFormat::defaultFormat();

    // Disable multi-sampling in the default framebuffer if we do our own
    // multi-sampling
    if(window.directRendering())
      format.setSamples(window.antiAliasingSamples());

    format.setVersion(3, 2);
    format.setProfile(QGLFormat::CompatibilityProfile);

    m_d->m_glWidget = new GLThreadWidget(format, m_d->m_mainWindow, *this, flags, window);

    m_d->m_glWidget->resize(window.width(), window.height());
    m_d->m_glWidget->raise();
    m_d->m_glWidget->show();

    m_d->m_glWidget->setFocus(Qt::ActiveWindowFocusReason);

    // If we bypass the window manager, we must explicitly make the window
    // active to get keyboard focus. We must defer this call so that the event
    // loop has time to process the events generated by window creation.
    if(flags & Qt::X11BypassWindowManagerHint)
      m_d->m_deferredActivateWindow = true;
  }

  QtWindow::~QtWindow()
  {
    delete m_d;
  }

  void QtWindow::poll()
  {
#ifdef RADIANT_LINUX
    // This hack is to get around Unity window manager issues in Ubuntu
    if(m_d->m_raiseCount < 40) {
      ++m_d->m_raiseCount;
      m_d->m_mainWindow->raise();
    }
#endif

    // Execute any deferred activateWindow() calls
    if(m_d->m_deferredActivateWindow) {
      m_d->m_glWidget->activateWindow();
      m_d->m_deferredActivateWindow = false;
    }
  }

  void QtWindow::makeCurrent()
  {
    for(int i = 0; i < 10; ++i) {
#ifdef RADIANT_WINDOWS
      wglMakeCurrent(m_d->m_dc, m_d->m_rc);
#else
      m_d->m_glWidget->makeCurrent();
#endif

      if(glGetError() == GL_NO_ERROR)
        break;

      Radiant::Sleep::sleepMs(10);
      Radiant::warning("QtWindow::makeCurrent # makeCurrent() failed, retrying... (%d)", i);
    }
  }

  bool QtWindow::mainThreadInit()
  {
    // We must use Qt's version of makeCurrent() to be able to grab a
    // handle to the OpenGL context.
    bool currentContextOk = false;

    for(int i = 0; i < 10; ++i) {
      m_d->m_glWidget->makeCurrent();

      if(glGetError() == GL_NO_ERROR) {
        currentContextOk = true;
        break;
      }

      Radiant::Sleep::sleepMs(10);
      Radiant::warning("QtWindow::mainThreadInit # makeCurrent() failed, retrying... (%d)", i);
    }

    if(!currentContextOk)
      return false;

#ifdef RADIANT_WINDOWS
    // Grab the OpenGL context handle
    m_d->m_rc = wglGetCurrentContext();
    assert(m_d->m_rc);

    if(!m_d->m_rc)
      return false;

    // Grab the device context handle
    m_d->m_dc = m_d->m_glWidget->getDC();
    assert(m_d->m_dc);

    return m_d->m_dc != nullptr;
#else
    return currentContextOk;
#endif
  }

  void QtWindow::swapBuffers()
  {
#ifdef RADIANT_WINDOWS
    HDC dc = wglGetCurrentDC();

    SwapBuffers(dc);
#else
    m_d->m_glWidget->swapBuffers();
#endif

    // Timeout in seconds after which the cursor is hidden
    const float hideCursorLowerLimit = 5.f;
    float since = m_d->m_glWidget->m_lastMouseAction.sinceSecondsD();

    if(since > hideCursorLowerLimit) {

      auto cursor = QApplication::overrideCursor();

      if(cursor && cursor->shape() != Qt::BlankCursor)
        QApplication::setOverrideCursor(Qt::BlankCursor);
      else if(!cursor || cursor->shape() == Qt::BlankCursor)
        QApplication::restoreOverrideCursor();
    }
  }

  void QtWindow::minimize()
  {
    m_d->m_glWidget->showMinimized();
  }

  void QtWindow::maximize()
  {
    m_d->m_glWidget->showMaximized();
  }

  void QtWindow::restore()
  {
    m_d->m_glWidget->showNormal();
  }

  void QtWindow::showCursor(bool visible)
  {
    m_d->m_glWidget->showCursor(visible);
  }

  void QtWindow::doneCurrent()
  {
    m_d->m_glWidget->doneCurrent();
  }

}
