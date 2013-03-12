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
    }

    const Luminous::PostProcessFilterPtr m_filter;

    Luminous::RenderTarget m_renderTarget;

    Luminous::Texture m_framebuffer;
    Luminous::RenderBuffer m_depthBuffer;
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

  void PostProcessContext::initialize(RenderContext & rc)
  {
    m_d->m_renderTarget.attach(GL_COLOR_ATTACHMENT0, m_d->m_framebuffer);
    m_d->m_renderTarget.attach(GL_DEPTH_ATTACHMENT, m_d->m_depthBuffer);

    m_d->m_renderTarget.setSize(Nimble::Size(rc.contextSize().x, rc.contextSize().y));

    m_d->m_filter->initialize(rc, *this);
  }

  void PostProcessContext::doFilter(Luminous::RenderContext & rc)
  {
    Luminous::Style style;

    style.setFillColor(1.0f, 1.0f, 1.0f, 1.0f);
    style.setTexture("tex", texture());

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

  Luminous::RenderTarget & PostProcessContext::renderTarget()
  {
    return m_d->m_renderTarget;
  }

  const Luminous::RenderTarget & PostProcessContext::renderTarget() const
  {
    return m_d->m_renderTarget;
  }

  const Luminous::Texture & PostProcessContext::texture() const
  {
    return m_d->m_framebuffer;
  }

  const Luminous::RenderBuffer & PostProcessContext::depthBuffer() const
  {
    return m_d->m_depthBuffer;
  }
}
