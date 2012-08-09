#include "PipelineCommand.hpp"

namespace Luminous
{

  CommandClearGL::CommandClearGL(Luminous::ClearMask clearMask, const Radiant::Color & clearColor, float clearDepth, int clearStencil)
    : m_clearMask(clearMask)
    , m_clearColor(clearColor)
    , m_clearDepth(clearDepth)
    , m_clearStencil(clearStencil)
  {
  }

  void CommandClearGL::execute()
  {
    GLbitfield glMask = 0;

    // Clear color buffer
    if (m_clearMask & ClearMask_Color) {
      glClearColor(m_clearColor.red(), m_clearColor.green(), m_clearColor.blue(), m_clearColor.alpha());
      glMask |= GL_COLOR_BUFFER_BIT;
    }

    // Clear depth buffer
    if (m_clearMask & ClearMask_Depth) {
      glClearDepth(m_clearDepth);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }

    // Clear stencil buffer
    if (m_clearMask & ClearMask_Stencil) {
      glClearStencil(m_clearStencil);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }

    glClear(glMask);
    GLERROR("CommandClearGL::execute glClear");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandChangeRenderTargetGL::CommandChangeRenderTargetGL(RenderTargetGL & rt)
    : m_renderTarget(rt)
  {
  }

  void CommandChangeRenderTargetGL::execute()
  {
    m_renderTarget.bind();
  }

}
