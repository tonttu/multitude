#include "PostProcessFilter.hpp"

#include <Radiant/Trace.hpp>

namespace Luminous
{
  class PostProcessFilter::D
  {
  public:
    D()
      : m_enabled(true)
      , m_order(0)
    {}

    bool m_enabled;
    int m_order;

    Luminous::RenderTarget m_renderTarget;

    Luminous::Texture m_framebuffer;
    Luminous::RenderBuffer m_depthBuffer;
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  PostProcessFilter::PostProcessFilter()
    : m_d(new D())
  {
  }

  PostProcessFilter::~PostProcessFilter()
  {
    delete m_d;
  }

  void PostProcessFilter::initialize(RenderContext & rc)
  {
    m_d->m_renderTarget.attach(GL_COLOR_ATTACHMENT0, m_d->m_framebuffer);
    m_d->m_renderTarget.attach(GL_DEPTH_ATTACHMENT, m_d->m_depthBuffer);

    m_d->m_renderTarget.setSize(Nimble::Size(rc.contextSize().x, rc.contextSize().y));
  }

  Luminous::Style PostProcessFilter::style() const
  {
    Luminous::Style style;

    style.setFillColor(1.0f, 1.0f, 1.0f, 1.0f);
    style.setTexture("tex", texture());

    return style;
  }

  void PostProcessFilter::begin(Luminous::RenderContext & rc)
  {
    rc.clear(Luminous::ClearMask_ColorDepth);
  }

  void PostProcessFilter::apply(RenderContext & rc)
  {
    const Luminous::Style & s = style();
    const Nimble::Vector2f size = rc.contextSize();
    const Luminous::Program & program = s.fillProgram() ? *s.fillProgram() : rc.texShader();

    auto b = rc.drawPrimitiveT<Luminous::BasicVertexUV, Luminous::BasicUniformBlock>(
          Luminous::PrimitiveType_TriangleStrip, 0, 4, program, s.fillColor(), 1.f, s);

    b.vertex[0].location.make(0, 0);
    b.vertex[0].texCoord.make(0, 0);
    b.vertex[1].location.make(size.x, 0);
    b.vertex[1].texCoord.make(1, 0);
    b.vertex[2].location.make(0, size.y);
    b.vertex[2].texCoord.make(0, 1);
    b.vertex[3].location.make(size.x, size.y);
    b.vertex[3].texCoord.make(1, 1);
  }

  bool PostProcessFilter::enabled() const
  {
    return m_d->m_enabled;
  }

  void PostProcessFilter::setEnabled(bool enabled)
  {
    m_d->m_enabled = enabled;
  }

  int PostProcessFilter::order() const
  {
    return m_d->m_order;
  }

  void PostProcessFilter::setOrder(int order)
  {
    m_d->m_order = order;
  }

  Luminous::RenderTarget & PostProcessFilter::renderTarget()
  {
    return m_d->m_renderTarget;
  }

  const Luminous::RenderTarget & PostProcessFilter::renderTarget() const
  {
    return m_d->m_renderTarget;
  }

  const Luminous::Texture & PostProcessFilter::texture() const
  {
    return m_d->m_framebuffer;
  }

  const Luminous::RenderBuffer & PostProcessFilter::depthBuffer() const
  {
    return m_d->m_depthBuffer;
  }
}
