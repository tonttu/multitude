/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "GLResource.hpp"
#include "RenderContext.hpp"
#include <cassert>

namespace Luminous
{

  GLResource::GLResource(RenderContext * context)
    : m_context(context),
      m_deleteOnFrame(0),
      m_generation(0)
  {
    if(!context) {
      m_context = RenderContext::getThreadContext();
      if(!m_context)
        Radiant::fatal("GLResource::GLResource # Thread context not set");
    }
  }

  GLResource::~GLResource()
  {}

  void GLResource::setContext(RenderContext * context)
  {
    if(context == m_context)
      return;

    assert(m_context == 0);

    m_context = context;
    changeByteConsumption(0, consumesBytes());
  }

  long GLResource::consumesBytes()
  {
    return 0;
  }

  void GLResource::setPersistent(bool b)
  {
    if(b)
      m_deleteOnFrame = PERSISTENT;
    else if(context()) {
      // Some random timeout:
      m_deleteOnFrame = context()->frame() + 100;
    }
    else
      m_deleteOnFrame = 10;
  }


  void GLResource::changeByteConsumption(long deallocated, long allocated)
  {
    if(m_context)
      m_context->changeByteConsumption(deallocated, allocated);
  }

}


