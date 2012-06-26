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

    // Clear the current rendertarget
    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::Color & color = Radiant::Color(0.f,0.f,0.f,1.f), double depth = 0, int stencil = 0) = 0;

    // Draw primitives
    LUMINOUS_API virtual void draw(PrimitiveType type, unsigned int offset, unsigned int primitives) = 0;
    // Draw indexed primitives
    LUMINOUS_API virtual void drawIndexed(PrimitiveType type, unsigned int offset, unsigned int primitives) = 0;

    // Called at the beginning of every frame
    LUMINOUS_API virtual void preFrame(unsigned int threadIndex) = 0;
    // Called at the end of every frame
    LUMINOUS_API virtual void postFrame(unsigned int threadIndex) = 0;

    // Shaders
    /// @note Can't do this with templates since they're pure virtual and require different implementation per datatype
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const int & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const float & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const Nimble::Vector2i & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const Nimble::Vector3i & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const Nimble::Vector4i & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const Nimble::Vector2f & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const Nimble::Vector3f & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const Nimble::Vector4f & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const Nimble::Matrix2f & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const Nimble::Matrix3f & value) = 0;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const QString & name, const Nimble::Matrix4f & value) = 0;
    LUMINOUS_API virtual void setShaderProgram(unsigned int threadIndex, const ShaderProgram & shader) = 0;

    // Bind a hardwarebuffer for use as a vertex buffer
    LUMINOUS_API virtual void setVertexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer) = 0;
    // Bind a hardwarebuffer for use as an index buffer
    LUMINOUS_API virtual void setIndexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer) = 0;
    // Bind a hardwarebuffer for use as a uniform buffer
    LUMINOUS_API virtual void setUniformBuffer(unsigned int threadIndex, const HardwareBuffer & buffer) = 0;

    // Setup the vertexbuffers and attributes
    LUMINOUS_API virtual void setVertexBinding(unsigned int threadIndex, const VertexAttributeBinding & binding) = 0;

    // Texturing
    LUMINOUS_API virtual void setTexture(unsigned int threadIndex, unsigned int textureUnit, const Texture & texture) = 0;

    // Reset the renderstate to its default
    LUMINOUS_API virtual void clearState(unsigned int threadIndex) = 0;

    // Enable/disable renderbuffers
    LUMINOUS_API virtual void setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) = 0;

    // Driver factory
    LUMINOUS_API static std::shared_ptr<RenderDriver> createInstance(unsigned int renderThreads);

  private:
    // Not exported, should only be used by the render manager
    friend class RenderManager;
    // Marks a resource as deleted, queuing it for removal on GPU
    virtual void releaseResource(RenderResource::Id id) = 0;
  };
}
#endif // LUMINOUS_RENDERDRIVER_HPP
