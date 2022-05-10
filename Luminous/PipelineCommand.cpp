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

  CommandClearGL::CommandClearGL(OpenGLAPI& opengl, Luminous::ClearMask clearMask, const Radiant::ColorPMA & clearColor, float clearDepth, int clearStencil)
    : PipelineCommand(opengl)
    , m_clearMask(clearMask)
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
      m_opengl.glClearColor(m_clearColor.red(), m_clearColor.green(), m_clearColor.blue(), m_clearColor.alpha());
      glMask |= GL_COLOR_BUFFER_BIT;
    }

    // Clear depth buffer
    if (m_clearMask & CLEARMASK_DEPTH) {
      m_opengl.glClearDepth(m_clearDepth);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }

    // Clear stencil buffer
    if (m_clearMask & CLEARMASK_STENCIL) {
      m_opengl.glClearStencil(m_clearStencil);
      glMask |= GL_STENCIL_BUFFER_BIT;
    }

    m_opengl.glClear(glMask);
    GLERROR("CommandClearGL::execute glClear");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandChangeFrameBufferGL::CommandChangeFrameBufferGL(OpenGLAPI& opengl, FrameBufferGL & rt)
    : PipelineCommand(opengl)
    , m_frameBuffer(rt)
  {
  }

  void CommandChangeFrameBufferGL::execute()
  {
    m_frameBuffer.bind();
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandSetBlendMode::CommandSetBlendMode(OpenGLAPI& opengl, const BlendMode & mode)
    : PipelineCommand(opengl)
    , m_mode(mode)
  {
  }

  void CommandSetBlendMode::execute()
  {
    m_opengl.glEnable(GL_BLEND);
    m_opengl.glBlendColor(m_mode.constantColor().red(), m_mode.constantColor().green(), m_mode.constantColor().blue(), m_mode.constantColor().alpha() );
    GLERROR("CommandSetBlendMode::execute # glBlendColor");
    m_opengl.glBlendEquation(m_mode.equation());
    GLERROR("CommandSetBlendMode::execute # glBlendEquation");
    m_opengl.glBlendFunc(m_mode.sourceFunction(), m_mode.destFunction());
    GLERROR("CommandSetBlendMode::execute # glBlendFunc");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  CommandSetDepthMode::CommandSetDepthMode(OpenGLAPI& opengl, const DepthMode & mode)
    : PipelineCommand(opengl)
    , m_mode(mode)
  {
  }

  void CommandSetDepthMode::execute()
  {
    m_opengl.glEnable(GL_DEPTH_TEST);
    m_opengl.glDepthFunc(m_mode.function());
    GLERROR("RenderDriverGL::setDepthMode # glDepthFunc");
    m_opengl.glDepthRange(m_mode.range().low(), m_mode.range().high());
    GLERROR("RenderDriverGL::setDepthMode # glDepthRange");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  CommandSetStencilMode::CommandSetStencilMode(OpenGLAPI& opengl, const StencilMode & mode)
    : PipelineCommand(opengl)
    , m_mode(mode)
  {
  }

  void CommandSetStencilMode::execute()
  {
    m_opengl.glEnable(GL_STENCIL_TEST);
    m_opengl.glStencilFuncSeparate(GL_FRONT, m_mode.frontFunction(), m_mode.frontRefValue(), m_mode.frontMaskValue());
    GLERROR("RenderDriverGL::setStencilMode # glStencilFuncSeparate");
    m_opengl.glStencilOpSeparate(GL_FRONT, m_mode.frontStencilFailOp(), m_mode.frontDepthFailOp(), m_mode.frontPassOp());
    GLERROR("RenderDriverGL::setStencilMode # glStencilOpSeparate");

    m_opengl.glStencilFuncSeparate(GL_BACK, m_mode.backFunction(), m_mode.backRefValue(), m_mode.backMaskValue());
    GLERROR("RenderDriverGL::setStencilMode # glStencilFuncSeparate");
    m_opengl.glStencilOpSeparate(GL_BACK, m_mode.backStencilFailOp(), m_mode.backDepthFailOp(), m_mode.backPassOp());
    GLERROR("RenderDriverGL::setStencilMode # glStencilOpSeparate");
  }
  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandChangeRenderBuffersGL::CommandChangeRenderBuffersGL(OpenGLAPI& opengl, bool colorBuffer, bool depthBuffer, bool stencilBuffer)
    : PipelineCommand(opengl)
    , m_colorBuffer(colorBuffer)
    , m_stencilBuffer(stencilBuffer)
    , m_depthBuffer(depthBuffer)
  {
  }

  void CommandChangeRenderBuffersGL::execute()
  {
    // Color buffers
    GLboolean color = (m_colorBuffer ? GL_TRUE : GL_FALSE);
    m_opengl.glColorMask( color, color, color, color);
    GLERROR("CommandChangeRenderBuffersGL::execute # glColorMask");

    // Depth buffer
    GLboolean depth = (m_depthBuffer ? GL_TRUE : GL_FALSE);
    m_opengl.glDepthMask(depth);
    GLERROR("CommandChangeRenderBuffersGL::execute # glDepthMask");

    // Stencil buffer
    GLuint stencil = (m_stencilBuffer ? 0xff : 0x00);
    m_opengl.glStencilMask(stencil);
    GLERROR("CommandChangeRenderBuffersGL::execute # glStencilMask");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandScissorGL::CommandScissorGL(OpenGLAPI& opengl, const Nimble::Recti & rect)
    : PipelineCommand(opengl)
    , m_rect(rect)
  {
  }

  void CommandScissorGL::execute()
  {
    m_opengl.glScissor(m_rect.low().x, m_rect.low().y, m_rect.width(), m_rect.height());
    GLERROR("CommandScissorGL::execute glScissor");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandViewportGL::CommandViewportGL(OpenGLAPI& opengl, const Nimble::Recti & rect)
    : PipelineCommand(opengl)
    , m_rect(rect)
  {
  }

  void CommandViewportGL::execute()
  {
    m_opengl.glViewport(m_rect.low().x, m_rect.low().y, m_rect.width(), m_rect.height());
    GLERROR("CommandViewportGL::execute glViewport");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandBlitGL::CommandBlitGL(OpenGLAPI& opengl, const Nimble::Recti & src, const Nimble::Recti & dst,
                               Luminous::ClearMask mask, Luminous::Texture::Filter filter)
    : PipelineCommand(opengl)
    , m_src(src)
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

    m_opengl.glBlitFramebuffer(m_src.low().x, m_src.low().y, m_src.high().x, m_src.high().y,
                               m_dst.low().x, m_dst.low().y, m_dst.high().x, m_dst.high().y,
                               glMask, m_filter);
    GLERROR("CommandBlitGL::execute glBlitFramebuffer");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandCullMode::CommandCullMode(OpenGLAPI& opengl, CullMode mode)
    : PipelineCommand(opengl)
    , m_mode(mode)
  {}

  void CommandCullMode::execute()
  {
    if(m_mode.enabled()) {
      m_opengl.glEnable(GL_CULL_FACE);
      GLERROR("CommandCullMode::execute # glEnable");
    } else {
      m_opengl.glDisable(GL_CULL_FACE);
      GLERROR("CommandCullMode::execute # glDisable");
    }

    m_opengl.glCullFace(m_mode.face());
    GLERROR("CommandCullMode::execute # glCullFace");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandFrontFace::CommandFrontFace(OpenGLAPI& opengl, FaceWinding winding)
    : PipelineCommand(opengl)
    , m_winding(winding)
  {}

  void CommandFrontFace::execute()
  {
    m_opengl.glFrontFace(m_winding);
    GLERROR("CommandFrontFace::execute # glFrontFace");
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CommandClipDistance::CommandClipDistance(OpenGLAPI& opengl, const QList<int> & planes, bool enable)
    : PipelineCommand(opengl)
    , m_planes(planes)
    , m_enable(enable)
  {
  }

  void CommandClipDistance::execute()
  {
    if (m_enable) {
      for (int i = 0; i < m_planes.size(); ++i) {
        m_opengl.glEnable(GL_CLIP_DISTANCE0 + m_planes.at(i));
        GLERROR("CommandClipDistance::execute # glEnable");
      }
    }
    else {
      for (int i = 0; i < m_planes.size(); ++i) {
        m_opengl.glDisable(GL_CLIP_DISTANCE0 + m_planes.at(i));
        GLERROR("CommandClipDistance::execute # glDisable");
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  CommandDrawBuffers::CommandDrawBuffers(OpenGLAPI& opengl, const std::vector<GLenum>& buffers)
    : PipelineCommand(opengl)
    , m_buffers(buffers)
  {
  }

  void CommandDrawBuffers::execute()
  {
    m_opengl.glDrawBuffers(static_cast<GLsizei>(m_buffers.size()),
                           reinterpret_cast<GLenum *>(m_buffers.data()));
    GLERROR("CommandDrawBuffers::execute # glDrawBuffers");
  }
}
