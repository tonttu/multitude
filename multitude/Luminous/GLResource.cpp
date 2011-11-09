/* COPYRIGHT
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


