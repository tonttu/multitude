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

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandScissorGL::CommandScissorGL(const Nimble::Recti & rect)
    : m_rect(rect)
  {
  }

  void CommandScissorGL::execute()
  {
    glScissor(m_rect.low().x, m_rect.low().y, m_rect.width(), m_rect.height());
    GLERROR("CommandScissorGL::execute glScissor");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandViewportGL::CommandViewportGL(const Nimble::Recti & rect)
    : m_rect(rect)
  {
  }

  void CommandViewportGL::execute()
  {
    glViewport(m_rect.low().x, m_rect.low().y, m_rect.width(), m_rect.height());
    GLERROR("CommandViewportGL::execute glViewport");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandBlitGL::CommandBlitGL(const Nimble::Recti & src, const Nimble::Recti & dst,
                               Luminous::ClearMask mask, Luminous::Texture::Filter filter)
    : m_src(src)
    , m_dst(dst)
    , m_mask(mask)
    , m_filter(filter)
  {
  }

  void CommandBlitGL::execute()
  {
    GLbitfield glMask = 0;
    if (m_mask & ClearMask_Color)
      glMask |= GL_COLOR_BUFFER_BIT;
    if (m_mask & ClearMask_Depth)
      glMask |= GL_DEPTH_BUFFER_BIT;
    if (m_mask & ClearMask_Stencil)
      glMask |= GL_DEPTH_BUFFER_BIT;

    glBlitFramebuffer(m_src.low().x, m_src.low().y, m_src.high().x, m_src.high().y,
                      m_dst.low().x, m_dst.low().y, m_dst.high().x, m_dst.high().y,
                      glMask, m_filter);
    GLERROR("CommandBlitGL::execute glBlitFramebuffer");
  }
}
