#if !defined (LUMINOUS_RENDERDRIVER_HPP)
#define LUMINOUS_RENDERDRIVER_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

#include <Radiant/Color.hpp>
#include <Radiant/RefPtr.hpp>

namespace Luminous
{
  class RenderDriver
  {
  public:
    LUMINOUS_API virtual ~RenderDriver() {}

    LUMINOUS_API virtual std::shared_ptr<VertexDescription> createVertexDescription() = 0;
    LUMINOUS_API virtual std::shared_ptr<VertexAttributeBinding> createVertexAttributeBinding() = 0;
    LUMINOUS_API virtual std::shared_ptr<HardwareBuffer> createVertexBuffer() = 0;
    LUMINOUS_API virtual std::shared_ptr<ShaderConstantBlock> createConstantBuffer() = 0;
    LUMINOUS_API virtual std::shared_ptr<ShaderProgram> createShaderProgram() = 0;
    LUMINOUS_API virtual std::shared_ptr<ShaderGLSL> createShader(ShaderType type) = 0;

    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::Color & color = Radiant::Color(0.f,0.f,0.f,1.f), double depth = 0, int stencil = 0) = 0;
    LUMINOUS_API virtual void draw(PrimitiveType type, size_t primitives, size_t offset) = 0;
    LUMINOUS_API virtual void drawIndexed(PrimitiveType type, size_t primitives, size_t offset) = 0;

    // Threaded calls
    LUMINOUS_API virtual void preFrame(unsigned int threadIndex) = 0;
    LUMINOUS_API virtual void postFrame(unsigned int threadIndex) = 0;

    // LUMINOUS_API virtual void setTarget(RenderTarget & target) = 0;
    LUMINOUS_API virtual void setViewport(unsigned int threadIndex, float x, float y, float width, float height) = 0;

    // Resource binding
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const HardwareBuffer & buffer) = 0;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const VertexAttributeBinding & binding) = 0;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const ShaderConstantBlock & constants) = 0;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const ShaderProgram & shader) = 0;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const HardwareBuffer & buffer) = 0;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const VertexAttributeBinding & buffer) = 0;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const ShaderConstantBlock & buffer) = 0;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const ShaderProgram & shader) = 0;

    LUMINOUS_API static std::shared_ptr<RenderDriver> createInstance(unsigned int renderThreads);

  private:
    // Not exported, should only be used by render resources
    friend class RenderResource;
    virtual void removeResource(RenderResource::Id id) = 0;
  };
}
#endif // LUMINOUS_RENDERDRIVER_HPP
