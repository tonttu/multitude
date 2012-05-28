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
    LUMINOUS_API virtual std::shared_ptr<HardwareBuffer> createHardwareBuffer(BufferType type) OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<ShaderConstantBlock> createShaderConstantBlock() OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<ShaderProgram> createShaderProgram() OVERRIDE;
    LUMINOUS_API virtual std::shared_ptr<ShaderGLSL> createShader(ShaderType type) OVERRIDE;

    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil) OVERRIDE;
    LUMINOUS_API virtual void draw(PrimitiveType type, size_t primitives, size_t offset) OVERRIDE;
    LUMINOUS_API virtual void drawIndexed(PrimitiveType type, size_t primitives, size_t offset) OVERRIDE;

    // Shaders: Manually set shader uniforms
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const int & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const float & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector2i & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector3i & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector4i & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector2f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector3f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector4f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Matrix2f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Matrix3f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Matrix4f & value) OVERRIDE;

    LUMINOUS_API virtual void preFrame(unsigned int threadIndex) OVERRIDE;
    LUMINOUS_API virtual void postFrame(unsigned int threadIndex) OVERRIDE;

    // LUMINOUS_API virtual void setTarget(RenderTarget & target) OVERRIDE;
    LUMINOUS_API virtual void setViewport(unsigned int threadIndex, float x, float y, float width, float height) OVERRIDE;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const HardwareBuffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const VertexAttributeBinding & binding) OVERRIDE;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const ShaderConstantBlock & constants) OVERRIDE;
    LUMINOUS_API virtual void bind(unsigned int threadIndex, const ShaderProgram & shader) OVERRIDE;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const HardwareBuffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const VertexAttributeBinding & buffer) OVERRIDE;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const ShaderConstantBlock & buffer) OVERRIDE;
    LUMINOUS_API virtual void unbind(unsigned int threadIndex, const ShaderProgram & shader) OVERRIDE;

  private:
    virtual void removeResource(RenderResource::Id id) OVERRIDE;
  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_RENDERDRIVERGL_HPP
