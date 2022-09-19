/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "StateGL.hpp"
#include "RenderDriverGL.hpp"

namespace Luminous
{
  StateGL::StateGL(unsigned int threadIndex, RenderDriverGL & driver)
    : m_threadIndex(threadIndex)
    , m_driver(driver)
  {
  }

  void StateGL::initGl()
  {
    m_opengl = &m_driver.opengl();
    m_opengl45 = m_driver.opengl45();
  }
}
