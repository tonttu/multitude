#if !defined (LUMINOUS_RENDERDRIVERGL_HPP)
#define LUMINOUS_RENDERDRIVERGL_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderDriver.hpp"

namespace Luminous
{
  class RenderDriverGL : public RenderDriver
  {
  public:
    LUMINOUS_API RenderDriverGL(unsigned int threadCount);
    LUMINOUS_API ~RenderDriverGL();

    LUMINOUS_API virtual std::shared_ptr<VertexDescription> createVertexDescription() OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<VertexAttributeBinding> createVertexAttributeBinding() OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<HardwareBuffer> createVertexBuffer() OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<ShaderConstantBlock> createConstantBuffer() OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<ShaderProgram> createShaderProgram() OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<ShaderGLSL> createShader(ShaderType type) OVERRIDE;

    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil) OVERRIDE;
    LUMINOUS_API virtual void draw(PrimitiveType type, size_t primitives, size_t offset) OVERRIDE;
    LUMINOUS_API virtual void drawIndexed(PrimitiveType type, size_t primitives, size_t offset) OVERRIDE;

    LUMINOUS_API virtual void preFrame(unsigned int threadIndex) OVERRIDE;
    LUMINOUS_API virtual void postFrame(unsigned int threadIndex) OVERRIDE;

    LUMINOUS_API virtual void bind(unsigned int threadIndex, const HardwareBuffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const VertexAttributeBinding & binding) OVERRIDE;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const ShaderConstantBlock & constants) OVERRIDE;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const ShaderProgram & shader) OVERRIDE;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const HardwareBuffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const VertexAttributeBinding & buffer) OVERRIDE;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const ShaderConstantBlock & buffer) OVERRIDE;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const ShaderProgram & shader) OVERRIDE;
  private:
    class Impl;
    Impl * m_impl;
  };
}

#endif // LUMINOUS_RENDERDRIVERGL_HPP
