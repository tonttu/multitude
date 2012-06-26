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

    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil) OVERRIDE;
    LUMINOUS_API virtual void draw(PrimitiveType type, unsigned int offset, unsigned int primitives) OVERRIDE;
    LUMINOUS_API virtual void drawIndexed(PrimitiveType type, unsigned int offset, unsigned int primitives) OVERRIDE;

    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const int & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const float & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const Nimble::Vector2i & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const Nimble::Vector3i & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const Nimble::Vector4i & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const Nimble::Vector2f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const Nimble::Vector3f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const Nimble::Vector4f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const Nimble::Matrix2f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const Nimble::Matrix3f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(unsigned int threadIndex, const char * name, const Nimble::Matrix4f & value) OVERRIDE;
    LUMINOUS_API virtual void setShaderProgram(unsigned int threadIndex, const ShaderProgram & shader) OVERRIDE;

    LUMINOUS_API virtual void preFrame(unsigned int threadIndex) OVERRIDE;
    LUMINOUS_API virtual void postFrame(unsigned int threadIndex) OVERRIDE;

    LUMINOUS_API virtual void setVertexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void setIndexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void setUniformBuffer(unsigned int threadIndex, const HardwareBuffer & buffer) OVERRIDE;

    LUMINOUS_API virtual void setVertexBinding(unsigned int threadIndex, const VertexAttributeBinding & binding) OVERRIDE;
    
    LUMINOUS_API virtual void setTexture(unsigned int threadIndex, unsigned int textureUnit, const Texture & texture) OVERRIDE;

    LUMINOUS_API virtual void clearState(unsigned int threadIndex) OVERRIDE;

    LUMINOUS_API virtual void setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) OVERRIDE;

    /// @todo Add function wrapper(s) for:
    /// * glLogicOp
    /// * glBlendFunc, glBlendEquation
    /// * glStencilFunc, glStencilMask, glStencilOp
    /// * FBOs
    /// * Reading framebuffer/target (see also FBOs)
    /// * Uniform blocks
    /// * glViewport / glScissor
    /// * glMapBuffer/glMapBufferRange
  private:
    virtual void releaseResource(RenderResource::Id id) OVERRIDE;
  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_RENDERDRIVERGL_HPP
