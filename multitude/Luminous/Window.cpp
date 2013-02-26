/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Window.hpp"

#include <string.h>

namespace Luminous
{

  Window::Window() : 
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
