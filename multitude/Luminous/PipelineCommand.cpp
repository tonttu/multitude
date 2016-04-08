/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "PipelineCommand.hpp"

namespace Luminous
{

  CommandClearGL::CommandClearGL(Luminous::ClearMask clearMask, const Radiant::ColorPMA & clearColor, float clearDepth, int clearStencil)
    : m_clearMask(clearMask)
    , m_clearColor(clearColor)
    , m_clearDepth(clearDepth)
    , m_clearStencil(clearStencil)
  {
  }

  void CommandClearGL::execute()
  {
    ClearBufferMask glMask = GL_NONE_BIT;

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

  CommandChangeFrameBufferGL::CommandChangeFrameBufferGL(FrameBufferGL & rt)
    : m_frameBuffer(rt)
  {
  }

  void CommandChangeFrameBufferGL::execute()
  {
    m_frameBuffer.bind();
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
    glBlendEquation(static_cast<GLenum>(m_mode.equation()));
    GLERROR("CommandSetBlendMode::execute # glBlendEquation");
    glBlendFunc(static_cast<GLenum>(m_mode.sourceFunction()), static_cast<GLenum>(m_mode.destFunction()));
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
    glDepthFunc(static_cast<GLenum>(m_mode.function()));
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
    glStencilFuncSeparate(GL_FRONT, static_cast<GLenum>(m_mode.frontFunction()), m_mode.frontRefValue(), m_mode.frontMaskValue());
    GLERROR("RenderDriverGL::setStencilMode # glStencilFuncSeparate");
    glStencilOpSeparate(GL_FRONT, static_cast<GLenum>(m_mode.frontStencilFailOp()), static_cast<GLenum>(m_mode.frontDepthFailOp()), static_cast<GLenum>(m_mode.frontPassOp()));
    GLERROR("RenderDriverGL::setStencilMode # glStencilOpSeparate");

    glStencilFuncSeparate(GL_BACK, static_cast<GLenum>(m_mode.backFunction()), static_cast<GLint>(m_mode.backRefValue()), static_cast<GLint>(m_mode.backMaskValue()));
    GLERROR("RenderDriverGL::setStencilMode # glStencilFuncSeparate");
    glStencilOpSeparate(GL_BACK, static_cast<GLenum>(m_mode.backStencilFailOp()), static_cast<GLenum>(m_mode.backDepthFailOp()), static_cast<GLenum>(m_mode.backPassOp()));
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
    GLERROR("CommandChangeRenderBuffersGL::execute # glColorMask");

    // Depth buffer
    GLboolean depth = (m_depthBuffer ? GL_TRUE : GL_FALSE);
    glDepthMask(depth);
    GLERROR("CommandChangeRenderBuffersGL::execute # glDepthMask");

    // Stencil buffer
    GLuint stencil = (m_stencilBuffer ? 0xff : 0x00);
    glStencilMask(stencil);
    GLERROR("CommandChangeRenderBuffersGL::execute # glStencilMask");
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
    ClearBufferMask glMask = GL_NONE_BIT;

    if (m_mask & CLEARMASK_COLOR)
      glMask |= GL_COLOR_BUFFER_BIT;
    if (m_mask & CLEARMASK_DEPTH)
      glMask |= GL_DEPTH_BUFFER_BIT;
    if (m_mask & CLEARMASK_STENCIL)
      glMask |= GL_STENCIL_BUFFER_BIT;

    glBlitFramebuffer(m_src.low().x, m_src.low().y, m_src.high().x, m_src.high().y,
                      m_dst.low().x, m_dst.low().y, m_dst.high().x, m_dst.high().y,
                      glMask, static_cast<GLenum>(m_filter));
    GLERROR("CommandBlitGL::execute glBlitFramebuffer");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandCullMode::CommandCullMode(CullMode mode)
    : m_mode(mode)
  {}

  void CommandCullMode::execute()
  {
    if(m_mode.enabled()) {
      glEnable(GL_CULL_FACE);
      GLERROR("CommandCullMode::execute # glEnable");
    } else {
      glDisable(GL_CULL_FACE);
      GLERROR("CommandCullMode::execute # glDisable");
    }

    glCullFace(static_cast<GLenum>(m_mode.face()));
    GLERROR("CommandCullMode::execute # glCullFace");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandFrontFace::CommandFrontFace(FaceWinding winding)
    : m_winding(winding)
  {}

  void CommandFrontFace::execute()
  {
    glFrontFace(static_cast<GLenum>(m_winding));
    GLERROR("CommandFrontFace::execute # glFrontFace");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandClipDistance::CommandClipDistance(const QList<int> & planes, bool enable)
    : m_planes(planes)
    , m_enable(enable)
  {
  }

  void CommandClipDistance::execute()
  {
    if (m_enable) {
      for (int i = 0; i < m_planes.size(); ++i) {
        glEnable(GL_CLIP_DISTANCE0 + m_planes.at(i));
        GLERROR("CommandClipDistance::execute # glEnable");
      }
    }
    else {
      for (int i = 0; i < m_planes.size(); ++i) {
        glDisable(GL_CLIP_DISTANCE0 + m_planes.at(i));
        GLERROR("CommandClipDistance::execute # glDisable");
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  CommandDrawBuffers::CommandDrawBuffers(const std::vector<GLenum>& buffers)
    : m_buffers(buffers)
  {
  }

  void CommandDrawBuffers::execute()
  {
    glDrawBuffers( m_buffers.size(), reinterpret_cast<GLenum *>(m_buffers.data()));
    GLERROR("CommandDrawBuffers::execute # glDrawBuffers");
  }
}
