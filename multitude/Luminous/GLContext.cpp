/* COPYRIGHT
 */

#include "GLContext.hpp"

#include <Radiant/Trace.hpp>

namespace Luminous
{
  using namespace Radiant;

  GLContext::GLContext()
  {}

  GLContext::~GLContext()
  {}

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  GLDummyContext::GLDummyContext()
  {}

  GLDummyContext::~GLDummyContext()
  {}

  void GLDummyContext::makeCurrent()
  {
    error("GLDummyContext::makeCurrent # Method not implemented for this platform");
  }

  GLContext * GLDummyContext::createSharedContext()
  {
    error("GLDummyContext::createSharedContext # Method not implemented for this platform");
    return 0;
  }

  Radiant::Mutex * GLDummyContext::mutex()
  {
    return 0;
  }

}

