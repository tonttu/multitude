/* COPYRIGHT
 */

#ifndef LUMINOUS_RENDERCONTEXT_HPP
#define LUMINOUS_RENDERCONTEXT_HPP

#include <Luminous/Luminous.hpp>
#include <Luminous/RenderDriver.hpp>
#include <Luminous/Transformer.hpp>
#include <Luminous/Style.hpp>
#include <Luminous/GLResource.hpp>
#include <Luminous/GLResources.hpp>
#include <Luminous/RenderContext.hpp>
#include <Luminous/Export.hpp>
#include <Luminous/VertexBuffer.hpp>
#include <Luminous/VertexArray.hpp>
#include <Luminous/Buffer.hpp>
#include <Luminous/RenderCommand.hpp>
#include <Luminous/PostProcessFilter.hpp>
#include "RenderTargetGL.hpp"
#include "BufferGL.hpp"
#include "CullMode.hpp"

#include <Nimble/Rectangle.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Splines.hpp>

#include <Radiant/Defines.hpp>

#include <QRectF>

namespace Luminous
{
  class GLSLProgramObject;



  /// RenderContext contains the current rendering state.
  class LUMINOUS_API RenderContext : public Transformer, public GLResources
  {
  public:

    class OpacityGuard : public Patterns::NotCopyable
    {
    public:
      OpacityGuard(RenderContext & r) : m_rc(&r) {}
      OpacityGuard(OpacityGuard && o) : m_rc(o.m_rc) { o.m_rc = nullptr; }
      ~OpacityGuard() { m_rc->popOpacity(); }

    private:
      RenderContext * m_rc;
    };

    /// TextFlags can be used to give performance hint to RenderContext about
    /// the layout of the text.
    enum TextFlags
    {
      /// Text layout changes usually every frame
      TextDynamic,
      /// Text layout does not change often
      TextStatic
    };


    /** Proxy object for building rendering command.

        When created contains abstraction of the actual rendering command including
        shader program and the placeholders for the data to be rendered (vertices, indices
        and uniforms).
    */
    template <typename Vertex, typename UniformBlock>
    struct RenderBuilder
    {
      RenderBuilder() : idx(), uniform(), vertex(), command(), depth(0.0f) {}
      /// Indices for rendering
      unsigned int * idx;
      /// Uniform block for uniforms
      UniformBlock * uniform;
      /// Vertices for rendering
      Vertex * vertex;
      /// Abstraction of the rendering command
      RenderCommand * command;
      /// Depth of rendering
      float depth;
    };

/// @cond
    struct SharedBuffer
    {
      SharedBuffer(Buffer::Type type) : type(type), reservedBytes(0) {}
      SharedBuffer(SharedBuffer && shared)
        : buffer(std::move(shared.buffer)),
          type(shared.type),
          reservedBytes(shared.reservedBytes)
      {}
      SharedBuffer & operator=(SharedBuffer && shared)
      {
        buffer = std::move(shared.buffer);
        type = shared.type;
        reservedBytes = shared.reservedBytes;
        return *this;
      }

      Buffer buffer;
      Buffer::Type type;
      std::size_t reservedBytes;
    };
    /// @endcond

    /// Constructs a new render context and associates the given resources to it
    /// @param driver render driver to use
    /// @param window window to associate this context with
    RenderContext(Luminous::RenderDriver & driver, const Luminous::MultiHead::Window * window = 0);
    virtual ~RenderContext();

    /// Sets the associated area for this context
    /// @param area area to associate
    void setArea(const Luminous::MultiHead::Area * area);

    const Luminous::MultiHead::Window * window() const;
    const Luminous::MultiHead::Area * area() const;

    bool initialize();

    /// Returns the resources of this context
    /// @todo make deprecated
    Luminous::RenderContext * resources() { return this; }

    /// Called once for every frame before rendering.
    void beginFrame();
    /// Called once for every frame after rendering.
    void endFrame();

    /// Called once for every area before rendering anything in it. Can be
    /// called multiple times per frame depending on configuration.
    void beginArea();
    /// Called once for every area after rendering it. Can be
    /// called multiple times per frame depending on configuration.
    void endArea();

    /// @cond
    void initPostProcess(Luminous::PostProcessFilters & filters);
    void postProcess();
    /// @endcond

    /// Transformation from the world coordinates (pixels) to projected eye
    /// coordinates (normalized device coordinates).
    const Nimble::Matrix4 & viewTransform() const;
    void pushViewTransform(const Nimble::Matrix4 & m);
    void popViewTransform();

    const RenderTarget & currentRenderTarget() const;

    /// Sets the rendering recursion limit for the context. This is relevant
    /// for ViewWidgets which can cause recursive rendering of the scene.
    /// @param limit recursion depth limit
    void setRecursionLimit(size_t limit) ;
    /// Returns the recursion limit
    size_t recursionLimit() const;

    /// Sets the current recursion depth.
    void setRecursionDepth(size_t rd);
    /// Returns current recursion depth
    size_t recursionDepth() const;

    /// Save the current clipping stack and start with a empty one
    void pushClipStack();

    /// Restores the previously saved clipping stack
    void popClipStack();

    /// Pushes a clipping rectangle to the context
    void pushClipRect(const Nimble::Rectangle & r);
    /// Pops a clipping rectangle from the context
    void popClipRect();

    /// Checks if the given rectangle is visible (not clipped).
    bool isVisible(const Nimble::Rectangle & area);

    // Render utility functions:

    /** Draw an arc
      @param center center of the arc
      @param radius radius of the arc
      @param fromRadians start angle in radians
      @param toRadians end angle in radians
      @param style color and other parameters for the arc
      @param lineSegments number of steps
      */
    void drawArc(const Nimble::Vector2f & center, float radius, float fromRadians, float toRadians, const Luminous::Style & style, unsigned int lineSegments = 0);

    /** Draw a circle
      @param center center of the circle
      @param radius radius of the circle
      @param style color and other parameters for the circle
      @param lineSegments number of steps
      @param fromRadians start angle in radians
      @param toRadians end angle in radians
      */
    void drawCircle(const Nimble::Vector2f & center, float radius, const Luminous::Style & style, unsigned int lineSegments = 0, float fromRadians=0, float toRadians=Nimble::Math::TWO_PI);


    void drawEllipse(Nimble::Vector2f center,
                     Nimble::Vector2f axis,
                     float otherAxisLength,
                     const Luminous::Style & style,
                     unsigned int lineSegments = 0,
                     float fromRadians = 0, float toRadians = Nimble::Math::TWO_PI);

    /** Draws a constant width donut.
      @param center center of the donut
      @param axis axis of the ellipse
      @param otherAxisLength axis of the ellipse
      @param width width of the donut
      @param style color and other parameters for the donut
      @param linesegments number of steps to use
      @param fromRadians start angle in radians
      @param toRadians end angle in radians
     */
    void drawDonut(const Nimble::Vector2f & center,
                   Nimble::Vector2 axis,
                   float otherAxisLength,
                   float width,
                   const Luminous::Style & style,
                   unsigned int linesegments = 0,
                   float fromRadians=0, float toRadians=Nimble::Math::TWO_PI);

    /** Draws a cut sector in a circle or a wedge.
      @param center center of the circle
      @param radius1 inner radius
      @param radius2 outer radius
      @param fromRadians start angle in radians
      @param toRadians end angle in radians
      @param segments number of segments to use
      @param style color and other parameters for the wedge
      */
    void drawWedge(const Nimble::Vector2f & center, float radius1, float radius2, float fromRadians, float toRadians, Style & style, int segments);

    /// Push the given opacity to render context. The resulting opacity will be
    /// the current opacity multiplied by the given value.
    /// @param opacity opacity to push
    OpacityGuard pushOpacity(float opacity);
    /// Pop the current opacity from the stack
    void popOpacity();
    /// Get the current opacity
    /// @return the current opacity
    float opacity() const;

    RenderTargetGuard pushRenderTarget(const RenderTarget & target);
    void popRenderTarget();

    //////////////////////////////////////////////////////////////////////////
    // Implementation

    /** Returns RenderBuilder for the given program. Assumes that UniformBlock has at least fields: @c projMatrix (Nimble::Matrix4),
        @c modelMatrix (Nimble::Matrix4), @c color (Nimble::Vector4) and @c depth (float).
        @todo rest of the documentation

        @param primType Primitives to render
        @param indexCount How many indices are to be specified. If zero, indices are effectively set to 0,1,...,vertexCount-1
        @param vertexCount How many vertices are to be specified.
        @param shader GLSL-program to use.
        @param color Color for the corresponding uniform.
        @param width Width for the rendered primitive.
        @param style Style for the rendering.
      */
    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> drawPrimitiveT(Luminous::PrimitiveType primType, unsigned int indexCount, unsigned int vertexCount,
      const Luminous::Program & shader, const Radiant::Color & color, float width, const Luminous::Style & style);

    void drawRectWithHole(const Nimble::Rectf & area, const Nimble::Rect & hole, const Luminous::Style & style);
    void drawLine(const Nimble::Vector2f & p1, const Nimble::Vector2f & p2, const Luminous::Style & style);
    void drawPolyLine(const Nimble::Vector2f * vertices, unsigned int numVertices, const Luminous::Style & style);
    void drawPoints(const Nimble::Vector2f * points, size_t numPoints, const Luminous::Style & style);
    void drawRect(const Nimble::Vector2f & min, const Nimble::Vector2f & max, const Style &style);
    void drawRect(const Nimble::Rectf & rect, const Style & style);
    void drawRect(const Nimble::Rectf & rect, const Nimble::Rectf & uvs, const Style & style);
    /// Renders a quad
    /// Stroke is not implemented for quads at the moment
    void drawQuad(const Nimble::Vector2 * vertices, const Nimble::Vector2 * uvs, const Style & style);
    void drawText(const TextLayout & layout, const Nimble::Vector2f & location, const Nimble::Rectf & viewRect, const TextStyle & style);
    void drawText(const QString & text, const Nimble::Rectf & rect, const TextStyle & style, TextFlags flags = TextStatic);

    /// Adds the render counter by one
    /** The render counter is used to track how many objects have been rendered since the counter was
        last reset. This can be useful for checking that object culling works as intended. */
    void addRenderCounter();

    /** Returns the size of the window of this #RenderContext object.

        @return If the window is null, then Nimble::Vector2(10,10) is returned.
    */
    Nimble::Vector2 contextSize() const;

    /// Push a viewport to the viewport stack
    /// Pushes a viewport to the top of the viewport stack.
    /// @param viewport viewport to push
    void pushViewport(const Nimble::Recti & viewport);
    /// Pop a viewport from the viewport stack
    /// Pops the viewport from the top of the viewport stack
    void popViewport();
    /// Get the current viewport
    /// @return the viewport from the top of the viewport stack
    const Nimble::Recti & currentViewport() const;

    void pushScissorRect(const Nimble::Recti & scissorArea);
    void popScissorRect();
    const Nimble::Recti & currentScissorArea() const;

    /// Copies pixels from the read render target to the draw render target
    /// @sa RenderTarget::setTargetBind
    void blit(const Nimble::Recti & src, const Nimble::Recti & dst,
              ClearMask mask = CLEARMASK_COLOR_DEPTH,
              Texture::Filter filter = Texture::FILTER_NEAREST);

    /// Set the active render buffers
    /// @param colorBuffer enables drawing to colorbuffer if set to true
    /// @param depthBuffer enables drawing to depthbuffer if set to true
    /// @param stencilBuffer enables drawing to stencilbuffer if set to true
    void setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer);

    /// Set the active blendmode
    void setBlendMode(const BlendMode & mode);

    /// Set the active depthmode
    void setDepthMode(const DepthMode & mode);

    /// Set the active stencilmode
    void setStencilMode(const StencilMode & mode);

    /// Set the active cull mode
    void setCullMode(const CullMode mode);

    /// Specify front-facing polygons
    void setFrontFace(enum FaceWinding winding);

    /// @todo REMOVE US
    static void setThreadContext(RenderContext * rsc);
    static RenderContext * getThreadContext();
    void bindTexture(GLenum target, GLenum /*unit*/, GLuint name) {glBindTexture(target, name);}
    void bindBuffer(GLenum target, GLuint name) { glBindBuffer(target, name);}
    void bindProgram(GLSLProgramObject *) {}

    /// Reset the OpenGL state to default
    void setDefaultState();
    void flush();

  private:
    //////////////////////////////////////////////////////////////////////////
    /// Direct mode API
    //////////////////////////////////////////////////////////////////////////
    friend class D;
    friend class CustomOpenGL;
    void draw(PrimitiveType primType, unsigned int offset, unsigned int primitives);
    void drawIndexed(PrimitiveType primType, unsigned int offset, unsigned int primitives);

    ProgramGL & handle(const Program & program);
    TextureGL & handle(const Texture & texture);
    RenderTargetGL & handle(const RenderTarget & target);
    RenderBufferGL & handle(const RenderBuffer & buffer);
    BufferGL & handle(const Buffer & buffer);
    VertexArrayGL & handle(const VertexArray & vertexarray, ProgramGL * program);

  public:

    SharedBuffer * findAvailableBuffer(std::size_t vertexSize, std::size_t vertexCount,
                                       Buffer::Type type);

    template <typename T>
    T * mapBuffer(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length,
                  Radiant::FlagsT<Buffer::MapAccess> access);

    template <typename T>
    inline T * mapBuffer(const Buffer & buffer, Buffer::Type type,
                         Radiant::FlagsT<Buffer::MapAccess> access);

    void unmapBuffer(const Buffer & buffer, Buffer::Type type, int offset = 0, std::size_t length = std::size_t(-1));

    void clear(ClearMask mask, const Radiant::Color & color = Radiant::Color(0,0,0,0), double depth = 1.0, int stencil = 0);

    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> render(bool translucent,
                                               Luminous::PrimitiveType type,
                                               int offset, int vertexCount,
                                               float primitiveSize,
                                               const Luminous::VertexArray & vertexArray,
                                               const Luminous::Program & program,
                                               const std::map<QByteArray, const Texture *> * textures = nullptr,
                                               const std::map<QByteArray, ShaderUniform> * uniforms = nullptr);

    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> render( bool translucent,
                                                Luminous::PrimitiveType type, int indexCount, int vertexCount, float primitiveSize,
                                                const Luminous::Program & program,
                                                const std::map<QByteArray, const Texture *> * textures = nullptr,
                                                const std::map<QByteArray, ShaderUniform> * uniforms = nullptr);

    const Program & basicShader() const;
    const Program & texShader() const;
    const Program & fontShader() const;

    int uniformBufferOffsetAlignment() const;

    std::size_t alignUniform(std::size_t uniformSize) const;
  private:
    RenderCommand & createRenderCommand(bool translucent,
                                        const Luminous::VertexArray & vertexArray,
                                        const Luminous::Buffer & uniformBuffer,
                                        float & depth,
                                        const Program & shader,
                                        const std::map<QByteArray,const Texture *> * textures = nullptr,
                                        const std::map<QByteArray, ShaderUniform> * uniforms = nullptr);

    RenderCommand & createRenderCommand(bool translucent,
                                        int indexCount, int vertexCount,
                                        std::size_t vertexSize, std::size_t uniformSize,
                                        unsigned *& mappedIndexBuffer,
                                        void *& mappedVertexBuffer,
                                        void *& mappedUniformBuffer,
                                        float & depth,
                                        const Program & program,
                                        const std::map<QByteArray, const Texture *> * textures = nullptr,
                                        const std::map<QByteArray, ShaderUniform> * uniforms = nullptr);

    template <typename Vertex, typename Uniform>
    RenderCommand & createRenderCommand(bool translucent,
                                        const Luminous::VertexArray & vertexArray,
                                        const Luminous::Buffer & uniformBuffer,
                                        Uniform *& mappedUniformBuffer,
                                        float & depth,
                                        const Program & shader,
                                        const std::map<QByteArray,const Texture *> * textures = nullptr,
                                        const std::map<QByteArray, ShaderUniform> * uniforms = nullptr);

    template <typename Vertex, typename UniformBlock>
    RenderCommand & createRenderCommand(bool translucent,
                                        int indexCount, int vertexCount,
                                        unsigned *& mappedIndexBuffer,
                                        Vertex *& mappedVertexBuffer,
                                        UniformBlock *& mappedUniformBuffer,
                                        float & depth,
                                        const Program & program,
                                        const std::map<QByteArray, const Texture *> * textures = nullptr,
                                        const std::map<QByteArray, ShaderUniform> * uniforms = nullptr);

    template <typename T>
    std::pair<T *, SharedBuffer *> sharedBuffer(
        std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset);

    std::pair<void *, SharedBuffer *> sharedBuffer(
        std::size_t vertexSize, std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset);

    //////////////////////////////////////////////////////////////////////////
    // <Luminousv2>
    //////////////////////////////////////////////////////////////////////////
  protected:
    virtual void beforeTransformChange();
  private:
    void drawCircleWithSegments(Nimble::Vector2f center, float radius, const float *rgba, int segments);
    void drawCircleImpl(Nimble::Vector2f center, float radius, const float *rgba);
    void drawTextImpl(const TextLayout & layout, const Nimble::Vector2f & location,
                      const Nimble::Vector2f & offset,
                      const Nimble::Rectf & viewRect, const TextStyle & style,
                      FontUniformBlock & uniform, const Program & program,
                      const Nimble::Matrix4f & modelview);

    Luminous::RenderContext * m_resources;
    class Internal;
    Internal * m_data;
  };

  /** Guard for executing plain OpenGL commands.
    *
    * When using more exotic states, user should set the state of
    * OpenGL state machine to the state it was before.
    */
  class CustomOpenGL : Patterns::NotCopyable
  {
  public:
    LUMINOUS_API CustomOpenGL(RenderContext & r, bool reset=false);
    LUMINOUS_API ~CustomOpenGL();

    inline ProgramGL & handle(const Program & program) { return m_r.handle(program); }
    inline TextureGL & handle(const Texture & texture) { return m_r.handle(texture); }
    inline BufferGL & handle(const Buffer & buffer) { return m_r.handle(buffer); }
    inline VertexArrayGL & handle(const VertexArray & vertexArray, ProgramGL * program) { return m_r.handle(vertexArray, program); }
    inline RenderBufferGL & handle(const RenderBuffer & buffer) { return m_r.handle(buffer); }
    inline RenderTargetGL & handle(const RenderTarget & target) { return m_r.handle(target); }

  private:
    RenderContext & m_r;
  };

  template <> LUMINOUS_API
  void * RenderContext::mapBuffer<void>(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length,
                                        Radiant::FlagsT<Buffer::MapAccess> access);

  template <typename T>
  T * RenderContext::mapBuffer(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length,
                               Radiant::FlagsT<Buffer::MapAccess> access)
  {
    return reinterpret_cast<T*>(mapBuffer<void>(buffer, type, offset, length, access));
  }

  template <typename T>
  inline T * RenderContext::mapBuffer(const Buffer & buffer, Buffer::Type type,
                                      Radiant::FlagsT<Buffer::MapAccess> access)
  {
    return mapBuffer<T>(buffer, type, 0, buffer.size(), access);
  }

  template <typename Vertex, typename Uniform>
  RenderCommand & RenderContext::createRenderCommand(bool translucent,
                                                     int indexCount, int vertexCount,
                                                     unsigned *& mappedIndexBuffer,
                                                     Vertex *& mappedVertexBuffer,
                                                     Uniform *& mappedUniformBuffer,
                                                     float & depth,
                                                     const Program & program,
                                                     const std::map<QByteArray, const Texture *> * textures,
                                                     const std::map<QByteArray, ShaderUniform> * uniforms)
  {
    return createRenderCommand(translucent,
                               indexCount, vertexCount,
                               sizeof(Vertex), sizeof(Uniform),
                               mappedIndexBuffer, reinterpret_cast<void *&>(mappedVertexBuffer),
                               reinterpret_cast<void *&>(mappedUniformBuffer),
                               depth, program, textures, uniforms);
  }

  template <typename T>
  std::pair<T *, RenderContext::SharedBuffer *> RenderContext::sharedBuffer(
      std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset)
  {
    void * t;
    SharedBuffer * buffer;
    std::tie(t, buffer) = sharedBuffer(sizeof(T), maxVertexCount, type, offset);
    return std::make_pair(reinterpret_cast<T*>(t), buffer);
  }
}

#include <Luminous/RenderContextImpl.hpp>

#endif
