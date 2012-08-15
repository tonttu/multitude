#include <Radiant/Platform.hpp>

#include "QtWindow.hpp"
#include "WindowEventHook.hpp"
#include "AdaptiveVSync.hpp"

#include <Radiant/Sleep.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/KeyEvent.hpp>

#include <QApplication>
#include <QGLWidget>
#include <QMouseEvent>
#include <QDesktopWidget>

namespace Luminous
{

  class GLThreadWidget : public QGLWidget
  {
  public:
    GLThreadWidget(const QGLFormat & format, QWidget * host, QtWindow & window, Qt::WindowFlags flags)
      : QGLWidget(format, host, 0, flags),
      m_window(window)
    {
      // Needed for key events on Windows
      setFocusPolicy(Qt::StrongFocus);

      // Make the widget receive mouse move events even if no buttons are pressed
      setMouseTracking(true);
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

    virtual void mouseMoveEvent(QMouseEvent * e)
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleMouseEvent(*e);
    }

    virtual void mousePressEvent(QMouseEvent * e)
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleMouseEvent(*e);
    }

    virtual void mouseReleaseEvent(QMouseEvent * e)
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleMouseEvent(*e);
    }

    virtual void keyPressEvent(QKeyEvent * e)
    {
      // All done if there's no hooks installed
      if(m_window.eventHook())
        m_window.eventHook()->handleKeyboardEvent(Radiant::KeyEvent(*e));
    }

    virtual void keyReleaseEvent(QKeyEvent * e)
    {
      if(m_window.eventHook())
        m_window.eventHook()->handleKeyboardEvent(Radiant::KeyEvent(*e));
    }

    QtWindow & m_window;
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  class QtWindow::D
  {
  public:
    D()
      : m_hostWidget(0)
      , m_mainWindow(0)
    {}

    ~D()
    {
      delete m_hostWidget;
    }

    // Host widget is a container for our actual window (m_mainWindow)
    QWidget * m_hostWidget;
    GLThreadWidget * m_mainWindow;
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  QtWindow::QtWindow(const MultiHead::Window & window, const QString & windowTitle)
    : Window()
    , m_d(new D())
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
    if(window.frameless())
      flags = Qt::FramelessWindowHint;

    QWidget * parent = 0;


#ifdef RADIANT_LINUX
    // Handle multiple XScreens
    QDesktopWidget * desktop = QApplication::desktop();

    /// @todo this should be more graceful, what if the requested screen number doesn't exist?
    const int xScreenNumber = window.screennumber();
    assert(xScreenNumber <= desktop->screenCount());

    parent = desktop->screen(xScreenNumber);
#endif

    m_d->m_hostWidget = new QWidget(parent,  flags);

    if(!windowTitle.isEmpty())
      m_d->m_hostWidget->setWindowTitle(windowTitle);

    if(window.screen()->iconify())
       m_d->m_hostWidget->setWindowState(Qt::WindowMinimized);

    m_d->m_hostWidget->move(window.location().x, window.location().y);
    m_d->m_hostWidget->raise();
    m_d->m_hostWidget->show();
    m_d->m_hostWidget->resize(window.width(), window.height());

    if(window.fullscreen())
      m_d->m_hostWidget->showFullScreen();

    QGLFormat format = QGLFormat::defaultFormat();
    format.setSamples(window.antiAliasingSamples());

    m_d->m_mainWindow = new GLThreadWidget(format, m_d->m_hostWidget, *this, flags);

    m_d->m_mainWindow->raise();
    m_d->m_mainWindow->show();
    m_d->m_mainWindow->resize(window.width(), window.height());
    m_d->m_mainWindow->setFocus();
  }

  QtWindow::~QtWindow()
  {
    delete m_d;
  }

  void QtWindow::poll()
  {
    // Qt events are processed in SimpleThreadedApplication::update()
  }

  void QtWindow::makeCurrent()
  {
    for(int i = 0; i < 100; ++i) {
      m_d->m_mainWindow->makeCurrent();
      if(glGetError() == GL_NO_ERROR) break;
      Radiant::Sleep::sleepMs(10);
    }

    AdaptiveVSync::enable();
  }

  void QtWindow::swapBuffers()
  {
    m_d->m_mainWindow->swapBuffers();
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
