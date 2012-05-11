#include <Radiant/Platform.hpp>

//#ifndef RADIANT_LINUX

#include "QtWindow.hpp"
#include "WindowEventHook.hpp"

#include <Radiant/Sleep.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/KeyEvent.hpp>

#include <QApplication>
#include <QGLWidget>
#include <QMouseEvent>

namespace Radiant
{

  class GLThreadWidget : public QGLWidget
  {
  public:
    GLThreadWidget(QWidget * host, QtWindow & window, Qt::WindowFlags flags)
      : QGLWidget(host, 0, flags),
      m_window(window)
    {
      // Needed for key events on Windows
      setFocusPolicy(Qt::StrongFocus);

      // Make the widget receive mouse move events even if no buttons are pressed
      setMouseTracking(true);

      m_lastAction = Radiant::TimeStamp::getTime();
    }

    virtual void swapBuffers()
    {
      QGLWidget::swapBuffers();

      float since = m_lastAction.sinceSecondsD();

      /// @todo implement
//      if(since < 6.0) {
//        if(since > 5.0)
//          ThreadedRendering::SimpleThreadedApplication::instance()->notifyHideCursor();
//      }
    }

  private:
    // Empty overrides for thread-safety
    virtual void paintEvent(QPaintEvent *) {}

    void resizeEvent ( QResizeEvent * e )
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleWindowMove( pos().x(), pos().y(), e->size().width(), e->size().height() );
    }

    void moveEvent ( QMoveEvent * e )
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleWindowMove( e->pos().x(), e->pos().y(), size().width(), size().height() );
    }

    WindowEventHook::MouseButtonMask convertQtMouseButton(Qt::MouseButtons b)
    {
      WindowEventHook::MouseButtonMask mask = WindowEventHook::NoButton;

      if(b & Qt::LeftButton)
        mask = WindowEventHook::LeftButton;

      if(b & Qt::MidButton)
        mask = WindowEventHook::MiddleButton;

      if(b & Qt::RightButton)
        mask = WindowEventHook::RightButton;

      return mask;
    }

    virtual void mouseMoveEvent(QMouseEvent * e)
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleMouseEvent(*e);
      showCursor();
    }

    virtual void mousePressEvent(QMouseEvent * e)
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleMouseEvent(*e);
      showCursor();
    }

    virtual void mouseReleaseEvent(QMouseEvent * e)
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleMouseEvent(*e);
      showCursor();
    }

    virtual void keyPressEvent(QKeyEvent * e)
    {
      // All done if there's no hooks installed
      if(m_window.eventHook())
        m_window.eventHook()->handleKeyboardEvent(*e);
    }

    virtual void keyReleaseEvent(QKeyEvent * e)
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleKeyboardEvent(*e);
    }

    virtual void showCursor()
    {
      m_lastAction = Radiant::TimeStamp::getTime();

      if(QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
      }
    }

    QtWindow & m_window;

    Radiant::TimeStamp m_lastAction;
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  QtWindow::QtWindow(const WindowConfig & hint, const char * caption)
    : Window()
  {
    /* The code below opens a new OpenGL window at desired loation. Extra steps
       are taken to ensure that the window creation happens so that:

       1) A dummy window is created, and moved to the right location,
       with right size etc.

       2) An OpenGL widget is opened at this correct location.

       The purpose of this exercise is that that when one is using ATI
       GPUs, then ATI driver selects the GPU for the OpenGL context
       based on window location, when the context is created. Choosing
       wrong GPU can cause massive performance penalty. Similar behavior has been
       witnessed on OSX.

    */



    Qt::WindowFlags flags = 0;
    if(hint.frameless)
      flags = Qt::FramelessWindowHint;

    QWidget * host = new QWidget(0,  flags);

    if(caption)
      host->setWindowTitle(caption);
    if(hint.iconify)
       host->setWindowState(Qt::WindowMinimized);

    host->move(hint.x, hint.y);
    host->raise();
    host->show();
    host->resize(hint.width, hint.height);

    if(hint.fullscreen)
      host->showFullScreen();

    m_mainWindow = new GLThreadWidget(host, *this, flags);

    m_mainWindow->raise();
    m_mainWindow->show();
    //    m_mainWindow->move(hint.x, hint.y);
    m_mainWindow->resize(hint.width, hint.height);
    m_mainWindow->setFocus();
  }

  QtWindow::~QtWindow()
  {
    m_mainWindow->deleteLater();
  }

  void QtWindow::poll()
  {
    // Qt events are processed in SimpleThreadedApplication::update()
  }

  void QtWindow::makeCurrent()
  {
    for(int i = 0; i < 100; ++i) {
      m_mainWindow->makeCurrent();
      if(glGetError() == GL_NO_ERROR) break;
      Radiant::Sleep::sleepMs(10);
    }
  }

  void QtWindow::swapBuffers()
  {
    m_mainWindow->swapBuffers();
  }

  void QtWindow::minimize()
  {
    m_mainWindow->showMinimized();
  }

  void QtWindow::maximize()
  {
    m_mainWindow->showMaximized();
  }

  void QtWindow::restore()
  {
    m_mainWindow->showNormal();
  }

}

//#endif
