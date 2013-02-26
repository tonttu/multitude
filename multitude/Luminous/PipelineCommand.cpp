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
    if (m_clearMask & CLEARMASK_COLOR) {
      glClearColor(m_clearColor.red(), m_clearColor.green(), m_clearColor.blue(), m_clearColor.alpha());
      glMask |= GL_COLOR_BUFFER_BIT;
    }

    // Clear depth buffer
    if (m_clearMask & CLEARMASK_DEPTH) {
      glClearDepth(m_clearDepth);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }

    // Clear stencil buffer
    if (m_clearMask & CLEARMASK_STENCIL) {
      glClearStencil(m_clearStencil);
      glMask |= GL_STENCIL_BUFFER_BIT;
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

  CommandSetBlendMode::CommandSetBlendMode(const BlendMode & mode)
    : m_mode(mode)
  {

  }

  void CommandSetBlendMode::execute()
  {
    glEnable(GL_BLEND);
    glBlendColor(m_mode.constantColor().red(), m_mode.constantColor().green(), m_mode.constantColor().blue(), m_mode.constantColor().alpha() );
    GLERROR("CommandSetBlendMode::execute # glBlendColor");
    glBlendEquation(m_mode.equation());
    GLERROR("CommandSetBlendMode::execute # glBlendEquation");
    glBlendFunc(m_mode.sourceFunction(), m_mode.destFunction());
    GLERROR("CommandSetBlendMode::execute # glBlendFunc");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  CommandSetDepthMode::CommandSetDepthMode(const DepthMode & mode)
    : m_mode(mode)
  {

  }

  void CommandSetDepthMode::execute()
  {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(m_mode.function());
    GLERROR("RenderDriverGL::setDepthMode # glDepthFunc");
    glDepthRange(m_mode.range().low(), m_mode.range().high());
    GLERROR("RenderDriverGL::setDepthMode # glDepthRange");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  CommandSetStencilMode::CommandSetStencilMode(const StencilMode & mode)
    : m_mode(mode)
  {

  }

  void CommandSetStencilMode::execute()
  {
    glEnable(GL_STENCIL_TEST);
    glStencilFuncSeparate(GL_FRONT, m_mode.frontFunction(), m_mode.frontRefValue(), m_mode.frontMaskValue());
    GLERROR("RenderDriverGL::setStencilMode # glStencilFuncSeparate");
    glStencilOpSeparate(GL_FRONT, m_mode.frontStencilFailOp(), m_mode.frontDepthFailOp(), m_mode.frontPassOp());
    GLERROR("RenderDriverGL::setStencilMode # glStencilOpSeparate");

    glStencilFuncSeparate(GL_BACK, m_mode.backFunction(), m_mode.backRefValue(), m_mode.backMaskValue());
    GLERROR("RenderDriverGL::setStencilMode # glStencilFuncSeparate");
    glStencilOpSeparate(GL_BACK, m_mode.backStencilFailOp(), m_mode.backDepthFailOp(), m_mode.backPassOp());
    GLERROR("RenderDriverGL::setStencilMode # glStencilOpSeparate");
  }
  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandChangeRenderBuffersGL::CommandChangeRenderBuffersGL(bool colorBuffer, bool depthBuffer, bool stencilBuffer)
    : m_colorBuffer(colorBuffer)
    , m_stencilBuffer(stencilBuffer)
    , m_depthBuffer(depthBuffer)
  {
  }

  void CommandChangeRenderBuffersGL::execute()
  {
    // Color buffers
    GLboolean color = (m_colorBuffer ? GL_TRUE : GL_FALSE);
    glColorMask( color, color, color, color);

    // Depth buffer
    GLboolean depth = (m_depthBuffer ? GL_TRUE : GL_FALSE);
    glDepthMask(depth);

    // Stencil buffer
    GLuint stencil = (m_stencilBuffer ? 0xff : 0x00);
    glStencilMask(stencil);
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
    if (m_mask & CLEARMASK_COLOR)
      glMask |= GL_COLOR_BUFFER_BIT;
    if (m_mask & CLEARMASK_DEPTH)
      glMask |= GL_DEPTH_BUFFER_BIT;
    if (m_mask & CLEARMASK_STENCIL)
      glMask |= GL_STENCIL_BUFFER_BIT;

    glBlitFramebuffer(m_src.low().x, m_src.low().y, m_src.high().x, m_src.high().y,
                      m_dst.low().x, m_dst.low().y, m_dst.high().x, m_dst.high().y,
                      glMask, m_filter);
    GLERROR("CommandBlitGL::execute glBlitFramebuffer");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandCullMode::CommandCullMode(CullMode mode)
    : m_mode(mode)
  {}

  void CommandCullMode::execute()
  {
    if(m_mode.enabled())
      glEnable(GL_CULL_FACE);
    else
      glDisable(GL_CULL_FACE);

    glCullFace(m_mode.face());
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandFrontFace::CommandFrontFace(FaceWinding winding)
    : m_winding(winding)
  {}

  void CommandFrontFace::execute()
  {
    glFrontFace(m_winding);
  }

}
