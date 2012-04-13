/* COPYRIGHT
 *
 * This file is part of ThreadedRendering.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitouch.fi
 *
 * All rights reserved, 2007-2010
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "ThreadedRendering.hpp" for authors and more details.
 *
 */

#include <Radiant/Platform.hpp>

#ifdef RADIANT_LINUX

#include <QApplication>
#include <QInputContext>

#include <Radiant/KeyEvent.hpp>

#include <Radiant/XWindow.hpp>
#include <Radiant/WindowEventHook.hpp>

#include <Radiant/Trace.hpp>

#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <iostream>

#include <X11/keysym.h>

// Mask for events that the window listens
#define X11_CHECK_EVENT_MASK \
    ( KeyPressMask | KeyReleaseMask | \
    ButtonPressMask | ButtonReleaseMask | \
    PointerMotionMask | StructureNotifyMask \
    )

namespace {
  Radiant::Mutex s_X11mutex;
  Radiant::Mutex s_grabMutex;
  QWidget * s_keyboardGrabber = 0;
  /// XNextEvent with select() so we don't have to resort to polling
  /// @param timeout timeout in ms
  /// @return did we get an event within the timeout
  bool WaitForXNextEvent(Display * display, XEvent * evt, int timeout)
  {
    bool haveEvent = XPending(display);
    if(!haveEvent) {
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = timeout*1000;
      int fd = XConnectionNumber(display);
      fd_set readset;
      FD_ZERO(&readset);
      FD_SET(fd, &readset);
      select(fd+1, &readset, 0, 0, &tv);
      haveEvent = XPending(display);
    }
    if(haveEvent) {
      XNextEvent(display, evt);
      return true;
    }
    return false;
  }

  class InputContextGrabber : public QInputContext
  {
  public:
    InputContextGrabber(QInputContext & origContext,
                        Radiant::WindowEventHook & hook)
      : m_context(origContext)
      , m_hook(hook) {}

    virtual bool filterEvent(const QEvent * event)
    {
      const QKeyEvent* keyEvent = dynamic_cast<const QKeyEvent*>(event);
      if(keyEvent) {
        m_hook.handleKeyboardEvent(Radiant::KeyEvent(*keyEvent));
        return true;
      }

      return m_context.filterEvent(event);
    }

    Radiant::WindowEventHook & hook()
    {
      return m_hook;
    }

    virtual QString identifierName() { return m_context.identifierName(); }
    virtual QString language() { return m_context.language(); }
    virtual void reset() { m_context.reset(); }
    virtual bool isComposing() const { return m_context.isComposing(); }

  private:
    QInputContext & m_context;
    Radiant::WindowEventHook & m_hook;
  };
}

namespace Radiant
{

  static int* generateAttributesFromHint(const WindowConfig & hint)
  {
    (void)hint;

    int* attributes = new int[1024];
    int* cur = attributes;

    *(cur++) = GLX_RGBA;
    *(cur++) = GLX_DOUBLEBUFFER;
    *(cur++) = GLX_RED_SIZE;
    *(cur++) = 8;
    *(cur++) = GLX_GREEN_SIZE;
    *(cur++) = 8;
    *(cur++) = GLX_BLUE_SIZE;
    *(cur++) = 8;
    *(cur++) = GLX_ALPHA_SIZE;
    *(cur++) = 8;
    *(cur++) = None;

    return(attributes);
  }

  static QPair<Qt::MouseButtons, Qt::KeyboardModifiers> x11StateToQt(unsigned int state)
  {
    Qt::MouseButtons buttons = 0;

    if(state & Button1Mask)
      buttons |= Qt::LeftButton;
    if(state & Button2Mask)
      buttons |= Qt::MidButton;
    if(state & Button3Mask)
      buttons |= Qt::RightButton;

    Qt::KeyboardModifiers modifiers = 0;

    if(state & ShiftMask)
      modifiers |= Qt::ShiftModifier;
    if(state & ControlMask)
      modifiers |= Qt::ControlModifier;

    return qMakePair(buttons, modifiers);
  }

  static void dispatchXConfigureEvent(WindowEventHook * hook, const XConfigureEvent & event)
  {
    hook->handleWindowMove(event.x, event.y, event.width, event.height);
  }

  static void dispatchXMouseEvent(WindowEventHook * hook, XButtonEvent & event)
  {
    //Radiant::info("dispatchXMouseEvent # button: %d", event.button);

    QEvent::Type type = (event.type == ButtonPress) ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease;
    QPoint position(event.x, event.y);

    Qt::MouseButton button = Qt::NoButton;
    if(event.button == Button1)
      button = Qt::LeftButton;
    else if(event.button == Button2)
      button = Qt::MidButton;
    else if(event.button == Button3)
      button = Qt::RightButton;

    QPair<Qt::MouseButtons, Qt::KeyboardModifiers> mods = x11StateToQt(event.state);

    QMouseEvent qtEvent(type, position, button, mods.first, mods.second);

    hook->handleMouseEvent(qtEvent);
  }

  static void dispatchXMouseMoveEvent(WindowEventHook * hook, XMotionEvent & event)
  {
    //Radiant::info("dispatchXMouseMoveEvent # button: %d", event.state);

    QEvent::Type type = QEvent::MouseMove;
    QPoint position(event.x, event.y);
    Qt::MouseButton button = Qt::NoButton;

    QPair<Qt::MouseButtons, Qt::KeyboardModifiers> mods = x11StateToQt(event.state);

    QMouseEvent qtEvent(type, position, button, mods.first, mods.second);

    hook->handleMouseEvent(qtEvent);
  }

  static int errorHandler(Display * display, XErrorEvent * e)
  {
    (void)display;
    Radiant::error("errorHandler # %ld %d %d %d %ld", e->serial, (int)e->error_code,
                   (int)e->request_code, (int)e->minor_code, (long)e->resourceid);
    return 0;
  }

  static int ignoreErrorHandler(Display *, XErrorEvent *)
  {
    return 0;
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////



  XWindow::X11GLContext::X11GLContext(Display     * display,
                                      GLXContext    sharecontext,
                                      XVisualInfo * visualInfo,
                                      GLXWindow     drawable)
                                        : m_display(display),
                                        m_visualInfo(visualInfo),
                                        m_drawable(drawable),
                                        m_mutex(0)
  {
    m_context = glXCreateContext(m_display, visualInfo, sharecontext, GL_TRUE);
  }

  XWindow::X11GLContext::~X11GLContext()
  {
    glXDestroyContext(m_display, m_context);
  }

  void XWindow::X11GLContext::makeCurrent()
  {
    if(glXMakeCurrent(m_display, m_drawable, m_context) == False)
      Radiant::error("XWindow::X11GLContext::makeCurrent # glXMakeCurrent failed");
  }

  Luminous::GLContext * XWindow::X11GLContext::createSharedContext()
  {
    if(!m_mutex)
      m_mutex = new Radiant::Mutex;


    X11GLContext * ctx = new X11GLContext(m_display, m_context, m_visualInfo, m_drawable);
    ctx->m_mutex = m_mutex;

    return ctx;
  }


  Radiant::Mutex * XWindow::X11GLContext::mutex()
  {
    return m_mutex;
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  /// MwmHints in MwmUtil.h seems to be broken on 64-bit computers.
  /// Even if the data is always 32-bit integers, the struct
  /// is defined to have long (64 bit) integers.
  typedef struct
  {
    unsigned long   flags;
    unsigned long   functions;
    unsigned long   decorations;
    long            inputMode;
    unsigned long   status;
  } MwmHintsLong;

  XWindow::XWindow(const WindowConfig & hint, const char* caption)
    : m_display(0),
    m_drawable(0),
    m_context(0),
    m_autoRepeats(256),
    m_ignoreNextMotionEvent(false)
  {
    int (*handler)(Display*, XErrorEvent*) = XSetErrorHandler(errorHandler);

    m_display = XOpenDisplay(hint.display.toUtf8().data());
    if(m_display == 0) {
      Radiant::error("XOpenDisplay failed for %s", hint.display.toUtf8().data());
      XSetErrorHandler(handler);
      return;
    }


    int* attributes = generateAttributesFromHint(hint);
    XVisualInfo* visualInfo = glXChooseVisual(m_display, DefaultScreen(m_display), attributes);
    delete[] attributes;
    if(visualInfo == 0) {
      Radiant::error("failed to get visual info");
      XSetErrorHandler(handler);
      return;
    }


    int redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize;

    glXGetConfig(m_display, visualInfo, GLX_RED_SIZE,   &redSize);
    glXGetConfig(m_display, visualInfo, GLX_GREEN_SIZE, &greenSize);
    glXGetConfig(m_display, visualInfo, GLX_BLUE_SIZE,  &blueSize);
    glXGetConfig(m_display, visualInfo, GLX_ALPHA_SIZE, &alphaSize);
    glXGetConfig(m_display, visualInfo, GLX_DEPTH_SIZE, &depthSize);
    glXGetConfig(m_display, visualInfo, GLX_STENCIL_SIZE, &stencilSize);

    Colormap colormap;

    int screen_number = visualInfo->screen;
    ::Window root = RootWindow(m_display, screen_number);
    colormap = XCreateColormap(m_display, root,
                               visualInfo->visual, AllocNone);

    XSetWindowAttributes wAttributes;
    wAttributes.colormap = colormap;
    wAttributes.border_pixel = 0;
    wAttributes.override_redirect = hint.frameless && !hint.fullscreen;

    m_width = hint.width;
    m_height = hint.height;
    m_pos.make(hint.x, hint.y);

    wAttributes.event_mask = StructureNotifyMask;

    m_drawable = XCreateWindow(m_display, RootWindow(m_display, screen_number),
                               hint.x, hint.y, m_width, m_height, 0,
                               visualInfo->depth, InputOutput, visualInfo->visual,
                               CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
                               &wAttributes);

    // Make window visible
    XMapWindow(m_display, m_drawable);

    // Try to remove the decorations (create frameless window).
    if(hint.frameless) {
      MwmHintsLong mwmhints;
      Atom prop = None;
      prop = XInternAtom(m_display, "_MOTIF_WM_HINTS", True);
      if (prop == None) {
        Radiant::info("Window Manager does not support MWM hints. To get a borderless window I have to bypass your wm.");
        mwmhints.flags = 0;
      } else {
        mwmhints.flags = 2; //MWM_HINTS_DECORATIONS;
        mwmhints.decorations = 0;

        XChangeProperty(m_display, m_drawable,
                        prop, prop, 32, PropModeReplace,
                        (unsigned char *) &mwmhints, sizeof(mwmhints)/sizeof(long));
      }
    }

    if(hint.fullscreen || hint.frameless) {
      const char * statetype = (hint.fullscreen
            ? "_NET_WM_STATE_FULLSCREEN"
            : "_NET_WM_STATE_ABOVE");
      Atom state_above = XInternAtom(m_display, statetype, False);
      Atom state = XInternAtom(m_display, "_NET_WM_STATE", False);
      if (state == None || state_above == None) {
        Radiant::info("Window Manager does not support window state hints.");
      } else {
        XClientMessageEvent ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = ClientMessage;
        ev.message_type = state;
        ev.display = m_display;
        ev.window = m_drawable;
        ev.format = 32;
        ev.data.l[0] = 1;
        ev.data.l[1] = state_above;

        XLockDisplay(m_display);
        XSendEvent(m_display, root, 0, SubstructureRedirectMask |
                   SubstructureNotifyMask, (XEvent*) &ev);
        XUnlockDisplay(m_display);
      }
    }

    if (hint.iconify) {
      XIconifyWindow(m_display, m_drawable, screen_number);
    }

    // Move the window to desired location
    XMoveWindow(m_display, m_drawable, hint.x, hint.y);
    XResizeWindow(m_display, m_drawable, m_width, m_height);

    // Set title
    if(caption)
    {
      XTextProperty textProp;
      memset(&textProp, 0, sizeof(textProp));
      XStringListToTextProperty(const_cast<char **> (&caption), 1, &textProp);
      XSetWMName(m_display, m_drawable, &textProp);
      XFree(textProp.value);
    }

    // Wait for Window to be mapped with a right size
    mapWindow();

    // Set input focus manually, since override_redirect with non-fullscreen
    // windows disables the input focus be default.
    XSetInputFocus(m_display, m_drawable, RevertToPointerRoot, CurrentTime);

    // Set the window to receive KeyPress and KeyRelease events
    XSelectInput(m_display, m_drawable, X11_CHECK_EVENT_MASK);

    /*if (!hint.showCursor)
      showCursor(false);
*/

    m_lastAction = Radiant::TimeStamp::getTime();
    showCursor(false);
    // Allow the window to be deleted by the window manager
    // WM_DELETE_WINDOW = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    // XSetWMProtocols(m_display, m_drawable, &WM_DELETE_WINDOW, 1);

    m_context = new X11GLContext(m_display, 0, visualInfo, m_drawable);

    makeCurrent();

    XSetErrorHandler(handler);
  }

  XWindow::~XWindow()
  {
    //    m_display = XOpenDisplay(0);

    showCursor(true);

    delete m_context;

    // Destroy automatically unmaps the window
    XDestroyWindow(m_display, m_drawable);
    XCloseDisplay(m_display);
  }

  void XWindow::showCursor(bool show)
  {
    // root window, do nothing
    if (m_drawable == 0) return;

    // Changing the cursor will emit motion event, we ignore this
    m_ignoreNextMotionEvent = true;

    if (show)
      XDefineCursor(m_display, m_drawable, 0);
    else
    {
      Cursor no_ptr;
      Pixmap bm_no;
      XColor black, dummy;
      Colormap colormap;
      static const char bm_no_data[] = { 0,0,0,0, 0,0,0,0 };

      /// For example when using 15 render threads on one GPU,
      /// XCreateBitmapFromData almost always hangs, sometimes in ~7 different
      /// threads with identical backtrace. It seems it's not thread-safe.
      Radiant::Guard g(s_X11mutex);

      colormap = DefaultColormap(m_display, DefaultScreen(m_display));
      XAllocNamedColor(m_display, colormap, "black", &black, &dummy);
      bm_no = XCreateBitmapFromData(m_display, m_drawable, bm_no_data, 8, 8);
      no_ptr = XCreatePixmapCursor(m_display, bm_no, bm_no, &black, &black, 0, 0);
      XDefineCursor(m_display, m_drawable, no_ptr);
      XFreeCursor(m_display, no_ptr);
    }
  }

  void XWindow::swapBuffers()
  {
    glXSwapBuffers(m_display, m_drawable);

    float since = m_lastAction.sinceSecondsD();

    // The cursor is hidden repeatedly for several frames in succession to make
    // sure it is hidden
    const float hideCursorUpperLimit = 7.f;
    const float hideCursorLowerLimit = 5.f;

    if(since < hideCursorUpperLimit) {
      if(since > hideCursorLowerLimit)
        showCursor(false);
      else
        showCursor(true);
    }

  }

  void XWindow::mapWindow()
  {
    bool volatile running = true;
    bool ok = false, visible = false;

    const double quiet_time = 0.1;

    XEvent e = {0};
    XSelectInput(m_display, m_drawable, StructureNotifyMask);

    int tries = 100;

    while(running) {
      bool got = WaitForXNextEvent(m_display, &e, quiet_time*1000);
      if(!running) return;
      if(tries <= 0) break;
      if(!got) {
        if(ok && visible) break;
        XWindowAttributes attr;
        memset(&attr, 0, sizeof(XWindowAttributes));
        XGetWindowAttributes(m_display, m_drawable, &attr);
        if(attr.x == m_pos.x && attr.y == m_pos.y &&
          attr.width == m_width && attr.height == m_height) break;
        XWindowChanges changes = { m_pos.x, m_pos.y, m_width, m_height, 0, 0, 0 };
        XUnmapWindow(m_display, m_drawable);
        XSync(m_display, 0);
        XConfigureWindow(m_display, m_drawable, CWX | CWY | CWWidth | CWHeight, &changes);
        XMapWindow(m_display, m_drawable);
        tries -= 20;
        continue;
      }

      if(e.type == UnmapNotify) {
        visible = false;
      } else if(e.type == MapNotify) {
        visible = true;
      } else if(e.type == ConfigureNotify) {
        if(e.xconfigure.x != m_pos.x || e.xconfigure.y != m_pos.y ||
           e.xconfigure.width != m_width || e.xconfigure.height != m_height) {
          XWindowChanges changes = { m_pos.x, m_pos.y, m_width, m_height, 0, 0, 0 };
          XConfigureWindow(m_display, m_drawable, CWX | CWY | CWWidth | CWHeight, &changes);
          --tries;
        } else {
          ok = true;
        }
      } else if(e.type == CreateNotify) {
        if(e.xcreatewindow.x != m_pos.x || e.xcreatewindow.y != m_pos.y ||
           e.xcreatewindow.width != m_width || e.xcreatewindow.height != m_height) {
          XWindowChanges changes = { m_pos.x, m_pos.y, m_width, m_height, 0, 0, 0 };
          XConfigureWindow(m_display, m_drawable, CWX | CWY | CWWidth | CWHeight, &changes);
          --tries;
        } else {
          ok = true;
        }
      } else if(e.type == ReparentNotify) {
        /* ignore */
      } else {
        Radiant::error("XWindow::mapWindow # Unknown event %d", e.type);
      }
    }
    XSelectInput(m_display, m_drawable, 0);
  }

  void XWindow::poll()
  {
    XEvent event;
    KeySym keysym;

    bzero(&event, sizeof(event));
    bzero(&keysym, sizeof(keysym));

    WindowEventHook * hook = eventHook();

    while (XCheckMaskEvent(m_display, X11_CHECK_EVENT_MASK, &event))
    {
      bool autoRepeat = false;

      switch (event.type)
      {
      case KeyRelease:
      case KeyPress:
        if(hook)
          qApp->x11ProcessEvent(&event);

        // Reset cursor hide counter
        m_lastAction = Radiant::TimeStamp::getTime();
        break;
      case MotionNotify:
        {
          if(hook) {
            dispatchXMouseMoveEvent(hook, event.xmotion);
            //hook->handleMouseMove(event.xmotion.x, event.xmotion.y, event.xmotion.state);          
            qApp->x11ProcessEvent(&event);
          }
          // Reset cursor hide counter only if this event is not to be ignored
          if(!m_ignoreNextMotionEvent)
            m_lastAction = Radiant::TimeStamp::getTime();
          m_ignoreNextMotionEvent = false;
          break;
        }
      case ButtonPress:
      case ButtonRelease:
        {
          // override_redirect might disable normal input focus logic in
          // window managers (at least in Metacity). Do it manually here.
          ::Window focus_return = 0;
          int revert_to_return = 0;
          XGetInputFocus(m_display, &focus_return, &revert_to_return);
          if(focus_return != m_drawable)
            XSetInputFocus(m_display, m_drawable, RevertToPointerRoot, CurrentTime);
        }

        if(hook) {
          dispatchXMouseEvent(hook, event.xbutton);
          qApp->x11ProcessEvent(&event);
        }

        // Reset cursor hide counter
        m_lastAction = Radiant::TimeStamp::getTime();
        break;
      case ConfigureNotify:
        /*std::cout << "ConfigureNotify: xy (" << event.xconfigure.x << ", " << event.xconfigure.y
                      << "), wh (" << event.xconfigure.width
                      << ", " << event.xconfigure.height
                      << ")" << std::endl;*/

        if (hook)
          dispatchXConfigureEvent(hook, event.xconfigure);

        glXWaitX();

        break;

      case ReparentNotify:
        // just ignore this
        break;

      default:
        //         cout << "!! Unknown event of type " << event.type << endl;
        break;
      }
    }
  }

  void XWindow::makeCurrent()
  {
    m_context->makeCurrent();
  }

  void XWindow::deinit()
  {
  }

  void XWindow::minimize()
  {

  }

  void XWindow::restore()
  {

  }

  void XWindow::setEventHook(WindowEventHook * hook)
  {
    Window::setEventHook(hook);

    Radiant::Guard g(s_grabMutex);
    if(s_keyboardGrabber) {
      InputContextGrabber * context = dynamic_cast<InputContextGrabber*>(s_keyboardGrabber->inputContext());
      if(context && &context->hook() == hook)
        return;
      delete s_keyboardGrabber;
    }

    s_keyboardGrabber = new QWidget;
    s_keyboardGrabber->setAttribute(Qt::WA_InputMethodEnabled);
    s_keyboardGrabber->setInputContext(new InputContextGrabber(*s_keyboardGrabber->inputContext(), *hook));

    // Will trigger an error, since s_keyboardGrabbers isn't mapped. We don't care.
    int (*handler)(Display*, XErrorEvent*) = XSetErrorHandler(ignoreErrorHandler);
    s_keyboardGrabber->grabKeyboard();
    XSetErrorHandler(handler);
  }
}

#endif
