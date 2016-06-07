#include "StateGL.hpp"
#include "RenderDriverGL.hpp"

namespace Luminous
{

  OpenGLAPI& StateGL::opengl()
  {
    return m_driver.opengl();
  }

}
