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

#include "Window.hpp"

#include <string.h>

namespace Radiant
{

  Window::Window() : 
    m_active(false),
    m_finished(false),
    m_fullscreen(false),
    m_width(0),
    m_height(0),
    m_pos(0, 0),
    m_eventHook(0)
  {
  }

  Window::~Window()
  {
  }

  bool Window::isFinished() const
  {
    return m_finished;
  }

  void Window::setFullscreen(bool fullscreen)
  {
    m_fullscreen = fullscreen;
  }

  int Window::width() const
  {
    return m_width;
  }

  int Window::height() const
  {
    return m_height;
  }

  void Window::setEventHook(WindowEventHook *hook)
  {
    m_eventHook = hook;
  }

  WindowEventHook * Window::eventHook() const
  {
    return m_eventHook;
  }

}
