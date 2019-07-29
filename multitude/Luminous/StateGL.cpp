#include "StateGL.hpp"
#include "RenderDriverGL.hpp"

namespace Luminous
{

  OpenGLAPI& StateGL::opengl()
  {
    return m_driver.opengl();
  }

  void StateGL::addTask(std::function<void ()> task)
  {
    m_driver.addTask(std::move(task));
  }
}
