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

#include <string.h>

namespace Luminous
{

  Window::Window() : 
    m_finished(false),
    m_fullscreen(false),
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

  void Window::setEventHook(WindowEventHook *hook)
  {
    m_eventHook = hook;
  }

  WindowEventHook * Window::eventHook() const
  {
    return m_eventHook;
  }

}
