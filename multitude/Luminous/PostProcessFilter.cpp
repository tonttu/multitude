#include "PostProcessFilter.hpp"

#include <Luminous/PostProcessContext.hpp>
#include <Luminous/RenderContext.hpp>

namespace Luminous
{
  class PostProcessFilter::D
  {
  public:
    D()
      : m_enabled(true)
      , m_order(0)
    {
    }

    bool m_enabled;
    unsigned int m_order;
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  PostProcessFilter::PostProcessFilter(Valuable::Node * host, const QByteArray & name)
    : Node(host, name)
    , m_d(new D())
  {
  }

  PostProcessFilter::~PostProcessFilter()
  {
    delete m_d;
  }

  void PostProcessFilter::initialize(RenderContext &,
                                     PostProcessContext &) const
  {
  }

  void PostProcessFilter::filter(Luminous::RenderContext & rc,
                                 Luminous::PostProcessContext &,
                                 Luminous::Style style) const
  {
    rc.clear(Luminous::ClearMask_ColorDepth);

    const Nimble::Vector2f size = rc.contextSize();
    const Luminous::Program & program = style.fillProgram() ? *style.fillProgram() : rc.texShader();

    auto b = rc.drawPrimitiveT<Luminous::BasicVertexUV, Luminous::BasicUniformBlock>(
          Luminous::PrimitiveType_TriangleStrip, 0, 4, program, style.fillColor(), 1.f, style);

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

  unsigned int PostProcessFilter::order() const
  {
    return m_d->m_order;
  }

  void PostProcessFilter::setOrder(unsigned int order)
  {
    m_d->m_order = order;
  }
}
