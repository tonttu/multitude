#if !defined (LUMINOUS_RENDERCONTEXTIMMEDIATE_HPP)
#define LUMINOUS_RENDERCONTEXTIMMEDIATE_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderContext2.hpp"
#include <Radiant/RefPtr.hpp>

namespace Luminous
{
  class RenderContextImmediate : public RenderContext2
  {
  public:
    RenderContextImmediate(const std::shared_ptr<GLContext> & deviceContext, const std::shared_ptr<RenderDriver> & driver);

    virtual size_t frame() const OVERRIDE;
    virtual float framerate() const OVERRIDE;

    virtual void bind(VertexAttributeBinding & binding) OVERRIDE;
    virtual void draw(PrimitiveType type, size_t offset, size_t primitiveCount) OVERRIDE;

  private:
    class Impl;
    std::shared_ptr<Impl> m_impl;
  };
}
#endif // LUMINOUS_RENDERCONTEXTIMMEDIATE_HPP