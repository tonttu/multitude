#if !defined (LUMINOUS_RENDERDRIVER_HPP)
#define LUMINOUS_RENDERDRIVER_HPP

#include "Luminous/Luminous.hpp"

#include <Radiant/Color.hpp>
#include <Radiant/RefPtr.hpp>

namespace Luminous
{
  class RenderDriver
  {
  public:
    virtual ~RenderDriver() {}

    virtual std::shared_ptr<VertexDescription> createVertexDescription() = 0;
    virtual std::shared_ptr<VertexAttributeBinding> createVertexAttributeBinding() = 0;
    virtual std::shared_ptr<HardwareBuffer> createVertexBuffer(size_t vertexSize, size_t vertexCount, BufferUsage usage) = 0;

    virtual void clear(ClearMask mask, const Radiant::Color & color = Radiant::Color(0.f,0.f,0.f,1.f), double depth = 0, int stencil = 0) = 0;
    virtual void drawArrays(PrimitiveType type, size_t primitives, size_t offset) = 0;
    virtual void drawIndexedArrays(PrimitiveType type, size_t primitives, size_t offset) = 0;
  };
}
#endif // LUMINOUS_RENDERDRIVER_HPP
