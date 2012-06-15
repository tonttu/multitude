#if !defined (LUMINOUS_RENDERDRIVER_HPP)
#define LUMINOUS_RENDERDRIVER_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

#include <Radiant/Color.hpp>
#include <Radiant/RefPtr.hpp>
#include <QString>

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector3.hpp>
#include <Nimble/Vector4.hpp>
#include <Nimble/Matrix2.hpp>
#include <Nimble/Matrix3.hpp>
#include <Nimble/Matrix4.hpp>

namespace Luminous
{
  class RenderDriver
  {
  public:
    virtual ~RenderDriver() {}

    LUMINOUS_API virtual std::shared_ptr<VertexDescription> createVertexDescription() = 0;
    LUMINOUS_API virtual std::shared_ptr<VertexAttributeBinding> createVertexAttributeBinding() = 0;
    LUMINOUS_API virtual std::shared_ptr<HardwareBuffer> createHardwareBuffer() = 0;
    LUMINOUS_API virtual std::shared_ptr<ShaderProgram> createShaderProgram() = 0;
    LUMINOUS_API virtual std::shared_ptr<ShaderGLSL> createShader(ShaderType type) = 0;

    /// Clear the currently bound rendertarget
    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::Color & color = Radiant::Color(0.f,0.f,0.f,1.f), double depth = 0, int stencil = 0) = 0;

    // Draw primitives
    LUMINOUS_API virtual void draw(PrimitiveType type, size_t primitives, size_t offset) = 0;
    // Draw indexed primitives
    LUMINOUS_API virtual void drawIndexed(PrimitiveType type, size_t primitives, size_t offset) = 0;

    // Threaded calls
    LUMINOUS_API virtual void preFrame(unsigned int threadIndex) = 0;
    LUMINOUS_API virtual void postFrame(unsigned int threadIndex) = 0;

    // Shaders
    /// @note Can't do this with templates since they're pure virtual and require different implementation per datatype
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const int & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const float & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector2i & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector3i & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector4i & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector2f & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector3f & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Vector4f & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Matrix2f & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Matrix3f & value) = 0;
    LUMINOUS_API virtual bool setShaderConstant(unsigned int threadIndex, const QString & name, const Nimble::Matrix4f & value) = 0;
    LUMINOUS_API virtual void setShaderProgram(unsigned int threadIndex, const ShaderProgram & shader) = 0;

    // Buffer objects
    LUMINOUS_API virtual void setVertexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer) = 0;
    LUMINOUS_API virtual void setIndexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer) = 0;
    LUMINOUS_API virtual void setConstantBuffer(unsigned int threadIndex, const HardwareBuffer & buffer) = 0;

    LUMINOUS_API virtual void setVertexBinding(unsigned int threadIndex, const VertexAttributeBinding & binding) = 0;

    // Texturing
    LUMINOUS_API virtual void setTexture(unsigned int threadIndex, unsigned int textureUnit, const Texture2 & texture) = 0;

    // Factory
    LUMINOUS_API static std::shared_ptr<RenderDriver> createInstance(unsigned int renderThreads);

  private:
    // Not exported, should only be used by render resources
    friend class RenderResource;
    virtual void releaseResource(RenderResource::Id id) = 0;
  };
}
#endif // LUMINOUS_RENDERDRIVER_HPP
