#if !defined (LUMINOUS_RENDERDRIVERGL_HPP)
#define LUMINOUS_RENDERDRIVERGL_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderDriver.hpp"

namespace Luminous
{
  class RenderDriverGL : public RenderDriver
  {
  public:
    LUMINOUS_API RenderDriverGL(unsigned int threadId, unsigned int threadCount);
    LUMINOUS_API ~RenderDriverGL();

    LUMINOUS_API virtual std::shared_ptr<VertexDescription> createVertexDescription() OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<VertexAttributeBinding> createVertexAttributeBinding() OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<HardwareBuffer> createVertexBuffer(size_t vertexSize, size_t vertexCount, BufferUsage usage) OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<HardwareBuffer> createConstantBuffer(BufferUsage usage) OVERRIDE;

    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil) OVERRIDE;
    LUMINOUS_API virtual void drawArrays(PrimitiveType type, size_t primitives, size_t offset) OVERRIDE;
    LUMINOUS_API virtual void drawIndexedArrays(PrimitiveType type, size_t primitives, size_t offset) OVERRIDE;
  private:
    class Impl;
    Impl * m_impl;
  };
}

#endif // LUMINOUS_RENDERDRIVERGL_HPP
