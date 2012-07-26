#if !defined (LUMINOUS_RENDERDRIVERGL_HPP)
#define LUMINOUS_RENDERDRIVERGL_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderDriver.hpp"

#include <Radiant/Flags.hpp>

namespace Luminous
{
  class RenderDriverGL : public RenderDriver
  {
  public:
    LUMINOUS_API RenderDriverGL(unsigned int threadIndex);
    LUMINOUS_API ~RenderDriverGL();

    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil) OVERRIDE;
    LUMINOUS_API virtual void draw(PrimitiveType type, unsigned int offset, unsigned int primitives) OVERRIDE;
    LUMINOUS_API virtual void drawIndexed(PrimitiveType type, unsigned int offset, unsigned int primitives) OVERRIDE;

    LUMINOUS_API virtual bool setShaderUniform(const char * name, const int & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const unsigned int & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const float & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Vector2i & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Vector3i & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Vector4i & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Vector2T<unsigned int> & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Vector3T<unsigned int> & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Vector4T<unsigned int> & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Vector2f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Vector3f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Vector4f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Matrix2f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Matrix3f & value) OVERRIDE;
    LUMINOUS_API virtual bool setShaderUniform(const char * name, const Nimble::Matrix4f & value) OVERRIDE;
    LUMINOUS_API virtual void setShaderProgram(const ShaderProgram & shader) OVERRIDE;

    LUMINOUS_API virtual void preFrame() OVERRIDE;
    LUMINOUS_API virtual void postFrame() OVERRIDE;

    LUMINOUS_API virtual void setVertexBuffer(const Buffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void setIndexBuffer(const Buffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void setUniformBuffer(const Buffer & buffer) OVERRIDE;

    LUMINOUS_API virtual void setVertexBinding(const VertexAttributeBinding & binding) OVERRIDE;
    
    LUMINOUS_API virtual void setTexture(unsigned int textureUnit, const Texture & texture) OVERRIDE;

    LUMINOUS_API virtual void clearState() OVERRIDE;

    LUMINOUS_API virtual void setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) OVERRIDE;


    LUMINOUS_API virtual void * mapBuffer(const Buffer & buffer, int offset, std::size_t length,
                                          Radiant::FlagsT<Buffer::MapAccess> access) OVERRIDE;
    LUMINOUS_API virtual void unmapBuffer(const Buffer & buffer) OVERRIDE;

    LUMINOUS_API virtual RenderCommand & createRenderCommand(VertexAttributeBinding & binding,
                                                             Buffer & uniformBuffer,
                                                             const Luminous::Style & style) OVERRIDE;

    LUMINOUS_API virtual void flush() OVERRIDE;

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
