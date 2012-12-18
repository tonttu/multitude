#if !defined (LUMINOUS_RENDERDRIVERGL_HPP)
#define LUMINOUS_RENDERDRIVERGL_HPP

/// @cond

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderDriver.hpp"
#include "Luminous/BufferGL.hpp"
#include "Luminous/VertexArrayGL.hpp"
#include "Luminous/RenderTargetGL.hpp"

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

    LUMINOUS_API virtual void preFrame() OVERRIDE;
    LUMINOUS_API virtual void postFrame() OVERRIDE;

    LUMINOUS_API virtual bool initialize() OVERRIDE;
    LUMINOUS_API virtual void deInitialize() OVERRIDE;

    LUMINOUS_API virtual void setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) OVERRIDE;

    //////////
    ////////// "Unused" -^

    LUMINOUS_API virtual void * mapBuffer(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length,
                                          Radiant::FlagsT<Buffer::MapAccess> access) OVERRIDE;
    LUMINOUS_API virtual void unmapBuffer(const Buffer & buffer, Buffer::Type type, int offset = 0,
                                          std::size_t length = std::size_t(-1)) OVERRIDE;

    LUMINOUS_API virtual RenderCommand & createRenderCommand(bool translucent,
                                                             const VertexArray & vertexArray,
                                                             const Buffer & uniformBuffer,
                                                             const Luminous::Program & shader,
                                                             const std::map<QByteArray, const Texture *> * textures,
                                                             const std::map<QByteArray, ShaderUniform> * uniforms) OVERRIDE;

    LUMINOUS_API virtual void setDefaultState() OVERRIDE;
    LUMINOUS_API virtual void flush() OVERRIDE;

    LUMINOUS_API ProgramGL & handle(const Program & program);
    LUMINOUS_API TextureGL & handle(const Texture & texture);
    LUMINOUS_API BufferGL & handle(const Buffer & buffer);
    LUMINOUS_API VertexArrayGL & handle(const VertexArray & vertexArray, ProgramGL * program);
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
    LUMINOUS_API virtual void setCullMode(const CullMode & mode) OVERRIDE;
    LUMINOUS_API virtual void setFrontFace(FaceWinding winding) OVERRIDE;

    LUMINOUS_API virtual void setViewport(const Nimble::Recti & rect) OVERRIDE;
    LUMINOUS_API virtual void setScissor(const Nimble::Recti & rect) OVERRIDE;

    LUMINOUS_API virtual void blit(const Nimble::Recti & src, const Nimble::Recti & dst,
                                   Luminous::ClearMask mask = Luminous::CLEARMASK_COLOR_DEPTH,
                                   Luminous::Texture::Filter filter = Luminous::Texture::FILTER_NEAREST) OVERRIDE;

    LUMINOUS_API unsigned long availableGPUMemory() const OVERRIDE;
    LUMINOUS_API unsigned long maxGPUMemory() const OVERRIDE;

    LUMINOUS_API int uniformBufferOffsetAlignment() const;

    LUMINOUS_API void setVSync(bool vsync) OVERRIDE;
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

/// @endcond

#endif // LUMINOUS_RENDERDRIVERGL_HPP
