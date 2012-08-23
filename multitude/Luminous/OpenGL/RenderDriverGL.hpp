#if !defined (LUMINOUS_RENDERDRIVERGL_HPP)
#define LUMINOUS_RENDERDRIVERGL_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderDriver.hpp"
#include "Luminous/OpenGL/BufferGL.hpp"
#include "Luminous/OpenGL/VertexArrayGL.hpp"
#include "Luminous/OpenGL/RenderTargetGL.hpp"

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
    LUMINOUS_API virtual void setShaderProgram(const Program & shader) OVERRIDE;

    LUMINOUS_API virtual void preFrame() OVERRIDE;
    LUMINOUS_API virtual void postFrame() OVERRIDE;

    LUMINOUS_API virtual bool initialize() OVERRIDE;
    LUMINOUS_API virtual void deInitialize() OVERRIDE;

    LUMINOUS_API virtual void setVertexBuffer(const Buffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void setIndexBuffer(const Buffer & buffer) OVERRIDE;
    LUMINOUS_API virtual void setUniformBuffer(const Buffer & buffer) OVERRIDE;

    LUMINOUS_API virtual void setVertexArray(const VertexArray & vertexArray) OVERRIDE;

    LUMINOUS_API virtual void setTexture(unsigned int textureUnit, const Texture & texture) OVERRIDE;

    LUMINOUS_API virtual void clearState() OVERRIDE;

    LUMINOUS_API virtual void setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) OVERRIDE;

    //////////
    ////////// "Unused" -^

    LUMINOUS_API virtual void * mapBuffer(const Buffer & buffer, int offset, std::size_t length,
                                          Radiant::FlagsT<Buffer::MapAccess> access) OVERRIDE;
    LUMINOUS_API virtual void unmapBuffer(const Buffer & buffer, int offset = 0,
                                          std::size_t length = std::size_t(-1)) OVERRIDE;

    LUMINOUS_API virtual RenderCommand & createRenderCommand(bool translucent,
                                                             VertexArray & vertexArray,
                                                             Buffer & uniformBuffer,
                                                             const Luminous::Program & shader,
                                                             const std::map<QByteArray, const Texture *> & textures) OVERRIDE;

    LUMINOUS_API virtual void flush() OVERRIDE;

    LUMINOUS_API ProgramGL & handle(const Program & program);
    LUMINOUS_API TextureGL & handle(const Texture & texture);
    LUMINOUS_API BufferGL & handle(const Buffer & buffer);
    LUMINOUS_API VertexArrayGL & handle(const VertexArray & vertexArray, ProgramGL * program = nullptr);
    LUMINOUS_API RenderBufferGL & handle(const RenderBuffer & buffer);
    LUMINOUS_API RenderTargetGL & handle(const RenderTarget & target);

    LUMINOUS_API void pushRenderTarget(const RenderTarget & target);
    LUMINOUS_API void popRenderTarget();

    /// @todo Add function wrapper(s) for:
    /// * glLogicOp
    /// * FBOs    /// * Reading framebuffer/target (see also FBOs)
    /// * Uniform blocks
    /// * glViewport / glScissor
    /// * glMapBuffer/glMapBufferRange
    LUMINOUS_API virtual void setBlendMode(const BlendMode & mode) OVERRIDE;
    LUMINOUS_API virtual void setDepthMode(const DepthMode & mode) OVERRIDE;
    LUMINOUS_API virtual void setStencilMode(const StencilMode & mode) OVERRIDE;

  private:
    /// @todo hackish, is there a cleaner solution to access the shared_ptr ?
    std::shared_ptr<BufferGL> bufferPtr(const Buffer & buffer);
    friend class VertexArrayGL;

    virtual void releaseResource(RenderResource::Id id) OVERRIDE;
  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_RENDERDRIVERGL_HPP
