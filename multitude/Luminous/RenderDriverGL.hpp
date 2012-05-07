#if !defined (LUMINOUS_RENDERDRIVERGL_HPP)
#define LUMINOUS_RENDERDRIVERGL_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderDriver.hpp"
#include <Radiant/RefPtr.hpp>

namespace Luminous
{
  class RenderDriverGL : public RenderDriver
  {
  public:
    RenderDriverGL();

    virtual std::shared_ptr<VertexDescription> createVertexDescription() OVERRIDE;
    virtual std::shared_ptr<VertexAttributeBinding> createVertexAttributeBinding() OVERRIDE;
    virtual std::shared_ptr<HardwareBuffer> createVertexBuffer(size_t vertexSize, size_t vertexCount, BufferUsage usage) OVERRIDE;

    virtual void clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil) OVERRIDE;
    virtual void drawArrays(PrimitiveType type, size_t primitives, size_t offset) OVERRIDE;
    virtual void drawIndexedArrays(PrimitiveType type, size_t primitives, size_t offset) OVERRIDE;

  private:
    class Impl;
    std::shared_ptr<Impl> m_impl;
  };
}

#endif // LUMINOUS_RENDERDRIVERGL_HPP
