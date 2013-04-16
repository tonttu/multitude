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

      debugLuminous("OpenGL Context Debug:");
      debugLuminous("\tValid OpenGL Context: %d", isValid());
      debugLuminous("\tAccum Buffer Size: %d", f.accumBufferSize());
      debugLuminous("\tDepth Buffer Size: %d", f.depthBufferSize());
      debugLuminous("\tStencil Buffer Size: %d", f.stencilBufferSize());

      debugLuminous("\tRed Buffer Size: %d", f.redBufferSize());
      debugLuminous("\tGreen Buffer Size: %d", f.greenBufferSize());
      debugLuminous("\tBlue Buffer Size: %d", f.blueBufferSize());
      debugLuminous("\tAlpha Buffer Size: %d", f.alphaBufferSize());

      debugLuminous("\tDouble Buffering: %d", f.doubleBuffer());
      debugLuminous("\tDirect Rendering: %d", f.directRendering());

      debugLuminous("\tProfile: %d", f.profile());

      debugLuminous("\tVersion Major: %d", f.majorVersion());
      debugLuminous("\tVersion Minor: %d", f.minorVersion());

      debugLuminous("\tMultisampling: %d", f.sampleBuffers());
      debugLuminous("\tSamples: %d", f.samples());
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
    D(const Luminous::MultiHead::Window & win, const QString & title)
      : m_mainWindow(0)
      , m_deferredActivateWindow(false)
      , m_mhWindow(win)
      , m_title(title)
      , m_raiseCount(0)
    {}

    ~D()
    {
    }

    GLThreadWidget * m_mainWindow;
    bool m_deferredActivateWindow;
    const Luminous::MultiHead::Window & m_mhWindow;
    QString m_title;
    int m_raiseCount;
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  QtWindow::QtWindow(const MultiHead::Window & window, const QString & windowTitle)
    : Window()
    , m_d(new D(window, windowTitle))
  {
    /* The code below opens a new OpenGL window at desired location. Extra
       steps are taken to ensure that the window creation happens so that:

       1) A dummy window is created, and moved to the right location, with
       right size etc.

       2) An OpenGL widget is opened at this correct location.

       The purpose of this exercise is that that when one is using ATI
       GPUs, then ATI driver selects the GPU for the OpenGL context
       based on window location, when the context is created. Choosing
       wrong GPU can cause massive performance penalty. Similar behavior has been
       witnessed on OSX.

    */

    QWidget * parent = 0;

#ifdef RADIANT_LINUX
    // Handle multiple XScreens
    QDesktopWidget * desktop = QApplication::desktop();

    /// @todo this should be more graceful, what if the requested screen number doesn't exist?
    const int xScreenNumber = window.screennumber();
    assert(xScreenNumber <= desktop->screenCount());

    parent = desktop->screen(xScreenNumber);
#endif

    QGLFormat format = QGLFormat::defaultFormat();

    // Disable multi-sampling in the default framebuffer if we do our own
    // multi-sampling
    if(m_d->m_mhWindow.directRendering())
      format.setSamples(m_d->m_mhWindow.antiAliasingSamples());

    format.setVersion(3, 2);
    format.setProfile(QGLFormat::CompatibilityProfile);

    Qt::WindowFlags flags = 0;
    if(m_d->m_mhWindow.frameless()) {
      flags |= Qt::FramelessWindowHint;
      flags |= Qt::X11BypassWindowManagerHint;
    }

    m_d->m_mainWindow = new GLThreadWidget(format, nullptr, *this, flags, m_d->m_mhWindow);

    if(!m_d->m_title.isEmpty())
      m_d->m_mainWindow->setWindowTitle(m_d->m_title);

    if(m_d->m_mhWindow.screen()->iconify())
       m_d->m_mainWindow->setWindowState(Qt::WindowMinimized);

    m_d->m_mainWindow->move(m_d->m_mhWindow.location().x, m_d->m_mhWindow.location().y);
    m_d->m_mainWindow->raise();
    m_d->m_mainWindow->show();
    m_d->m_mainWindow->raise();
    m_d->m_mainWindow->resize(m_d->m_mhWindow.width(), m_d->m_mhWindow.height());

    if(m_d->m_mhWindow.fullscreen())
      m_d->m_mainWindow->showFullScreen();

    m_d->m_mainWindow->setFocus(Qt::ActiveWindowFocusReason);

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
    // Raising windows is a tough job...
    if(m_d->m_raiseCount < 100) {
      ++m_d->m_raiseCount;
      m_d->m_mainWindow->raise();
    }

    // Execute any deferred activateWindow() calls
    if(m_d->m_deferredActivateWindow) {
      m_d->m_mainWindow->activateWindow();
      m_d->m_deferredActivateWindow = false;
    }
  }

  void QtWindow::makeCurrent()
  {
    for(int i = 0; i < 100; ++i) {
      m_d->m_mainWindow->makeCurrent();
      if(glGetError() == GL_NO_ERROR) break;
      Radiant::Sleep::sleepMs(10);
    }
  }

  void QtWindow::swapBuffers()
  {
    m_d->m_mainWindow->swapBuffers();

    // Timeout in seconds after which the cursor is hidden
    const float hideCursorLowerLimit = 5.f;
    float since = m_d->m_mainWindow->m_lastMouseAction.sinceSecondsD();

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
    m_d->m_mainWindow->showMinimized();
  }

  void QtWindow::maximize()
  {
    m_d->m_mainWindow->showMaximized();
  }

  void QtWindow::restore()
  {
    m_d->m_mainWindow->showNormal();
  }

  void QtWindow::showCursor(bool visible)
  {
    m_d->m_mainWindow->showCursor(visible);
  }

}
