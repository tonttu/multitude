/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (LUMINOUS_RENDERDRIVERGL_HPP)
#define LUMINOUS_RENDERDRIVERGL_HPP

/// @cond

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderDriver.hpp"
#include "Luminous/BufferGL.hpp"
#include "Luminous/VertexArrayGL.hpp"
#include "Luminous/FrameBufferGL.hpp"

#include <Radiant/Flags.hpp>

namespace Luminous
{
  class RenderDriverGL : public RenderDriver
  {
  public:
    LUMINOUS_API RenderDriverGL(GfxDriver & gfxDriver, unsigned int threadIndex, QScreen * screen, const QSurfaceFormat & format);
    LUMINOUS_API ~RenderDriverGL();

    LUMINOUS_API void initGl(OpenGLAPI & opengl, OpenGLAPI45 * opengl45);

    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::ColorPMA & color, double depth, int stencil) OVERRIDE;
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

    LUMINOUS_API virtual MultiDrawCommand & createMultiDrawCommand(
        bool translucent,
        int drawCount,
        const VertexArray & vertexArray,
        const Buffer & uniformBuffer,
        const Luminous::Program & shader,
        const std::map<QByteArray, const Texture *> * textures,
        const std::map<QByteArray, ShaderUniform> * uniforms) override;

    LUMINOUS_API virtual void setDefaultState() OVERRIDE;
    LUMINOUS_API virtual void flush() OVERRIDE;

    LUMINOUS_API ProgramGL & handle(const Program & program);
    LUMINOUS_API TextureGL & handle(const Texture & texture);
    LUMINOUS_API BufferGL & handle(const Buffer & buffer);
    LUMINOUS_API VertexArrayGL & handle(const VertexArray & vertexArray, ProgramGL * program);
    LUMINOUS_API RenderBufferGL & handle(const RenderBuffer & buffer);
    LUMINOUS_API FrameBufferGL & handle(const FrameBuffer & target);

    /// @todo All handle()-functions should return pointers and have optional
    ///       flags argument specifying if we want to create missing handles
    ///       and synchronize (upload) data.
    LUMINOUS_API TextureGL * findHandle(const Texture & texture);

    LUMINOUS_API void pushFrameBuffer(const FrameBuffer & target);
    LUMINOUS_API void popFrameBuffer();

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

    LUMINOUS_API virtual void enableClipDistance(const QList<int> & planes) OVERRIDE;
    LUMINOUS_API virtual void disableClipDistance(const QList<int> & planes) OVERRIDE;

    LUMINOUS_API virtual void setDrawBuffers(const std::vector<GLenum>& buffers) OVERRIDE;

    LUMINOUS_API virtual void setViewport(const Nimble::Recti & rect) OVERRIDE;
    LUMINOUS_API virtual void setScissor(const Nimble::Recti & rect) OVERRIDE;

    LUMINOUS_API virtual void blit(const Nimble::Recti & src, const Nimble::Recti & dst,
                                   Luminous::ClearMask mask = Luminous::CLEARMASK_COLOR_DEPTH,
                                   Luminous::Texture::Filter filter = Luminous::Texture::FILTER_NEAREST) OVERRIDE;

    LUMINOUS_API int uniformBufferOffsetAlignment() const OVERRIDE;

    LUMINOUS_API bool setupSwapGroup(int group, int screen) OVERRIDE;

    LUMINOUS_API void setGPUId(unsigned int gpuId) OVERRIDE;
    LUMINOUS_API unsigned int gpuId() const OVERRIDE;

    LUMINOUS_API OpenGLAPI & opengl();
    LUMINOUS_API OpenGLAPI45 * opengl45();

    LUMINOUS_API StateGL & stateGl();

    LUMINOUS_API BufferGL & uploadBuffer(uint32_t size);

    /// Add a new task that is going to be executed on a different
    /// thread with a shared OpenGL context. Used for GPU data transfers.
    /// @todo use folly::Executor here instead
    LUMINOUS_API void addTask(std::function<void()> task);

  private:

    virtual void releaseResource(RenderResource::Id id) OVERRIDE;

    class D;
    D * m_d;
  };
}

/// @endcond

#endif // LUMINOUS_RENDERDRIVERGL_HPP
