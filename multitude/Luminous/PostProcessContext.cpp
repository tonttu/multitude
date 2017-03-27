/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "PostProcessContext.hpp"

#include <Radiant/Trace.hpp>

namespace Luminous
{
  class PostProcessContext::D
  {
  public:
    D(const Luminous::PostProcessFilterPtr filter)
      : m_filter(filter)
    {
      assert(filter);

      m_defaultShader.loadShader("cornerstone:Luminous/GLSL150/tex.vs", Luminous::Shader::Vertex);
      m_defaultShader.loadShader("cornerstone:Luminous/GLSL150/post-process.fs", Luminous::Shader::Fragment);

      Luminous::VertexDescription desc;

      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      desc.addAttribute<Nimble::Vector2>("vertex_uv");

      m_defaultShader.setVertexDescription(desc);
    }

    const Luminous::PostProcessFilterPtr m_filter;

    Luminous::FrameBuffer m_frameBuffer;

    Luminous::Texture m_texture;
    Luminous::RenderBuffer m_depthStencilBuffer;

    Luminous::Program m_defaultShader;
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  PostProcessContext::PostProcessContext(const Luminous::PostProcessFilterPtr filter)
    : m_d(new D(filter))
  {
  }

  PostProcessContext::~PostProcessContext()
  {
    delete m_d;
  }

  void PostProcessContext::initialize(RenderContext & rc, Nimble::Size size)
  {
    m_d->m_frameBuffer.attach(GL_COLOR_ATTACHMENT0, m_d->m_texture);
    m_d->m_frameBuffer.attach(GL_DEPTH_STENCIL_ATTACHMENT, m_d->m_depthStencilBuffer);

    m_d->m_frameBuffer.setSize(size);

    m_d->m_filter->initialize(rc, *this);
  }

  void PostProcessContext::doFilter(Luminous::RenderContext & rc, Nimble::Matrix3f textureMatrix)
  {
    Luminous::Style style;

    style.setFillColor(1.0f, 1.0f, 1.0f, 1.0f);
    style.setTexture("tex", texture());
    style.setFillShaderUniform("texMatrix", textureMatrix);
    style.setFillProgram(m_d->m_defaultShader);

    m_d->m_filter->filter(rc, *this, style);
  }

  bool PostProcessContext::enabled() const
  {
    return m_d->m_filter->enabled();
  }

  unsigned int PostProcessContext::order() const
  {
    return m_d->m_filter->order();
  }

  const PostProcessFilterPtr & PostProcessContext::filter() const
  {
    return m_d->m_filter;
  }

  Luminous::FrameBuffer & PostProcessContext::frameBuffer()
  {
    return m_d->m_frameBuffer;
  }

  const Luminous::FrameBuffer & PostProcessContext::frameBuffer() const
  {
    return m_d->m_frameBuffer;
  }

  const Luminous::Texture & PostProcessContext::texture() const
  {
    return m_d->m_texture;
  }

  const Luminous::RenderBuffer & PostProcessContext::depthStencilBuffer() const
  {
    return m_d->m_depthStencilBuffer;
  }
}
