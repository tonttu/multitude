#include "PostProcessFilter.hpp"

#include <Radiant/Trace.hpp>

namespace Luminous
{
  PostProcessFilter::PostProcessFilter()
    : m_enabled(true)
  {
    m_renderTarget.attach(GL_COLOR_ATTACHMENT0, m_framebuffer);
    m_renderTarget.attach(GL_DEPTH_ATTACHMENT, m_depthBuffer);
  }

  PostProcessFilter::~PostProcessFilter()
  {
  }

  void PostProcessFilter::initialize(RenderContext & rc)
  {
    m_renderTarget.setSize(Nimble::Size(rc.contextSize().x, rc.contextSize().y));
  }

  Luminous::Style PostProcessFilter::style() const
  {
    Luminous::Style style;

    style.setFillColor(1.0f, 1.0f, 1.0f, 1.0f);
    style.setTexture("tex", m_framebuffer);

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

    b.uniform->depth = 0;
  }
}
