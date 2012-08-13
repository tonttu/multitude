/* COPYRIGHT
 */

#ifndef LUMINOUS_RENDERCONTEXT_HPP
#define LUMINOUS_RENDERCONTEXT_HPP

#include <Luminous/Luminous.hpp>
// <Luminousv2>
#include <Luminous/RenderDriver.hpp>
// </Luminousv2>

#include <Luminous/FramebufferObject.hpp>
#include <Luminous/Transformer.hpp>
#include <Luminous/Style.hpp>
#include <Luminous/GLResource.hpp>
#include <Luminous/GLResources.hpp>
#include <Luminous/RenderContext.hpp>
#include <Luminous/Export.hpp>
#include <Luminous/VertexBuffer.hpp>
#include <Luminous/GLSLProgramObject.hpp>
#include <Luminous/FramebufferResource.hpp>
#include <Luminous/Buffer.hpp>
#include <Luminous/VertexHolder.hpp>
#include "OpenGL/RenderTargetGL.hpp"
#include "OpenGL/BufferGL.hpp"

#include <Nimble/Rectangle.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Splines.hpp>

#include <Radiant/Defines.hpp>

#include <QRectF>

namespace Luminous
{
  class Texture2D;
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

/// @cond

    class FBOPackage;

    class LUMINOUS_API FBOPackage : public GLResource
    {
    public:
      friend class FBOHolder;
      friend class RenderContext;

      FBOPackage(Luminous::RenderContext *res = 0) : GLResource(res), m_fbo(res), m_rbo(res), m_tex(res), m_users(0) {}
      virtual ~FBOPackage();

      void setSize(Nimble::Vector2i size);
      void attach();

      void activate(RenderContext & r);
      void deactivate(RenderContext & r);

      Luminous::Texture2D & texture() { return m_tex; }

    private:

      int userCount() const { return m_users; }

      Luminous::Framebuffer   m_fbo;
      Luminous::Renderbuffer  m_rbo;
      Luminous::Texture2D     m_tex;
      int m_users;
    };

/// @endcond

/// @cond
    /** Experimental support for getting temporary FBOs for this context.
        */
    class FBOHolder
    {
      friend class RenderContext;
    public:

      LUMINOUS_API FBOHolder();
      LUMINOUS_API FBOHolder(RenderContext * context, std::shared_ptr<FBOPackage> package);
      LUMINOUS_API FBOHolder(const FBOHolder & that);

      LUMINOUS_API ~FBOHolder();

      /** Copies the data pointers from the argument object. */
      LUMINOUS_API FBOHolder & operator = (const FBOHolder & that);

      LUMINOUS_API Luminous::Texture2D * finish();
      /** The relative texture coordinates for this useful texture area. */
      inline const Nimble::Vector2 & texUV() const { return m_texUV; }

    private:

      void release();

      RenderContext * m_context;
      std::shared_ptr<FBOPackage> m_package;
      Nimble::Vector2 m_texUV;
    };

    template <typename Vertex, typename UniformBlock>
    struct RenderBuilder
    {
      unsigned int * idx;
      UniformBlock * uniform;
      Vertex * vertex;
      RenderCommand * command;
      float depth;
    };

    enum {
      FBO_EXACT_SIZE = 0x1,
      /* these are just some big enough number, exact size is smaller */
      VBO_VERBUF_SIZE = 2 * (8 + 3000) * sizeof(GL_FLOAT),
      VBO_INDBUF_SIZE = 6000,
      LOD_MINIMUM = 2,
      LOD_MAXIMUM = 8
    };

/// @endcond

    /// Constructs a new render context and associates the given resources to it
    /// @param resources OpenGL resource container to associate with the context
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

    /// Pushes a clipping rectangle to the context
    void pushClipRect(const Nimble::Rectangle & r);
    /// Pops a clipping rectangle from the context
    void popClipRect();

    /// Returns the clipping rectangle stack
    const std::vector<Nimble::Rectangle> & clipStack() const;

    /// Checks if the given rectangle is visible (not clipped).
    bool isVisible(const Nimble::Rectangle & area);

    /// @cond
    void pushDrawBuffer(GLenum dest, FBOPackage * );
    void popDrawBuffer();
    
    // Render utility functions:

    /** Draws an arc
      @param center center of the arc
      @param radius radius of the arc
      @param width width of the arc
      @param fromRadians start angle in radians
      @param toRadians end angle in radians
      @param style color and other parameters for the arc
      @param lineSegments number of steps
      */
    void drawArc(Nimble::Vector2f center, float radius, float fromRadians, float toRadians, Luminous::Style & style, unsigned int lineSegments = 0);

    /** Draws a circle
      @param center center of the circle
      @param radius radius of the circle
      @param width width of the circle
      @param style color and other parameters for the circle
      @param lineSegments number of steps
      */
    void drawCircle(Nimble::Vector2f center, float radius, Luminous::Style & style, unsigned int lineSegments = 0, float fromRadians=0, float toRadians=Nimble::Math::TWO_PI);

    /** Draws a constant width donut.
      @param center center of the donut
      @param majorAxisLength length of the major axis
      @param minorAxisLength length of the minor axis
      @param width width of the donut
      @param style color and other parameters for the donut
      @param linesegments number of steps to use
      @param fromRadians
      @param toRadians
     */
    void drawDonut(Nimble::Vector2f center,
                   float majorAxisLength,
                   float minorAxisLength,
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
      @param width width of the wedge edge
      @param blendWidth width of the blending region
      @param rgba color of the wedge
      @param segments number of segments to use
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
    /// DEPRECATED FUNCTIONS
    /// @todo remove or replace with Styled-equivalent
    //////////////////////////////////////////////////////////////////////////
    void drawCurve(Nimble::Vector2*, float, const float * = 0) {}
    void drawSpline(Nimble::Interpolating &, float /*width*/, const float * = 0, float = 1.0f) {}
    void drawLine(const Nimble::Vector2f & /*p1*/, const Nimble::Vector2f & /*p2*/, float /*width*/, float * /*rgba*/) {}
    void drawCircle(Nimble::Vector2f /*center*/, float /*radius*/, const float * /*rgba*/, int /*segments*/ = 0)  {}
    void drawRect(const Nimble::Rectf & /*rect*/, const float * /*rgba*/) {}
    void drawLineRect(const Nimble::Rectf & /*rect*/, float /*width*/, const float * /*rgba*/) {}
    void drawTexRect(const Nimble::Rectf & /*rect*/, float * /*rgba*/) {}

    //////////////////////////////////////////////////////////////////////////
    // Implementation
    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> drawPrimitiveT(Luminous::PrimitiveType primType, const Nimble::Vector2f * vertices, unsigned int vertexCount,
      const Luminous::Program & shader, const Radiant::Color & color, float width, const Luminous::Style & style);

    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> drawTexPrimitiveT(Luminous::PrimitiveType primType, const Nimble::Vector2f * vertices, const Nimble::Vector2f * uvs, unsigned int vertexCount,
      const Luminous::Program & shader, const std::map<QByteArray, const Texture *> & textures, const Radiant::Color & color, float width, const Luminous::Style & style);

    void drawRectWithHole(const Nimble::Rect & area, const Nimble::Rect & hole, const Luminous::Style & style);
    void drawLine(const Nimble::Vector2 & p1, const Nimble::Vector2 & p2, const Luminous::Style & style);
    void drawPolyLine(const Nimble::Vector2 * vertices, unsigned int numVertices, const Luminous::Style & style);
    void drawPoints(const Nimble::Vector2f * points, size_t numPoints, const Luminous::Style & style);
    void drawRect(const Nimble::Vector2f & min, const Nimble::Vector2f & max, const Style &style);
    void drawRect(const Nimble::Rectf & rect, const Style & style);
    void drawRect(const Nimble::Rectf & rect, const Nimble::Rectf & uvs, const Style & style);

    /// Adds the render counter by one
    /** The render counter is used to track how many objects have been rendered since the counter was
        last reset. This can be useful for checking that object culling works as intended. */
    void addRenderCounter();

    /** Returns the size of the window of this #RenderContext object.

        @return If the window is null, then Nimble::Vector2(10,10) is returned.
    */
    Nimble::Vector2 contextSize() const;

    /// @cond
    void pushViewStack();
    /// Pops view stack, leaves current texture attached
    void popViewStack();

    /// @endcond

    /// Get a temporary texture render target
    // RenderTargetObject pushRenderTarget(Nimble::Vector2 size, float scale);
    /// Pop a temporary texture render target
    // Luminous::Texture2D & popRenderTarget(RenderTargetObject & trt);

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


    static void setThreadContext(RenderContext * rsc);

    /// Returns the RenderContext for the calling thread
    /// @todo not really implemented on Windows
    static RenderContext * getThreadContext();

    /// Returns the RenderContext for the calling thread
    /// @todo not really implemented on Windows
    /// @todo not really implemented on anything?
    /// static RenderContext * GLSLreadContext();

    void bindTexture(GLenum textureType, GLenum textureUnit, GLuint textureId);
    void bindBuffer(GLenum type, GLuint id);
    /// Bind GLSL program object
    void bindProgram(GLSLProgramObject * program);

    void flush();
    void flush2();
    void restart();

    //////////////////////////////////////////////////////////////////////////
    /// <Luminousv2>
    //////////////////////////////////////////////////////////////////////////

    void setBuffer(Buffer::Type type, const Luminous::Buffer & buffer);
    void setVertexArray(const VertexArray & vertexArray);
    void setShaderProgram(const Program & program);
    template <typename T> bool setShaderUniform(const char * name, const T & value);

    template <typename T>
    T * mapBuffer(const Buffer & buffer, int offset, std::size_t length,
                  Radiant::FlagsT<Buffer::MapAccess> access);

    template <typename T>
    inline T * mapBuffer(const Buffer & buffer,
                         Radiant::FlagsT<Buffer::MapAccess> access);

    void unmapBuffer(const Buffer & buffer, int offset = 0, std::size_t length = std::size_t(-1));

    void draw(PrimitiveType primType, unsigned int offset, unsigned int primitives);
    void drawIndexed(PrimitiveType primType, unsigned int offset, unsigned int primitives);

    void clear(ClearMask mask, const Radiant::Color & color = Radiant::Color(0,0,0,0), double depth = 1.0, int stencil = 0);

    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> render( bool translucent,
                                                Luminous::PrimitiveType type, int indexCount, int vertexCount, float primitiveSize,
                                                const Luminous::Program & program, const std::map<QByteArray, const Texture *> & textures);

    ProgramGL & handle(const Program & program);
    TextureGL & handle(const Texture & texture);
    RenderTargetGL & handle(const RenderTarget & target);
    BufferGL & handle(const Buffer & buffer);
    VertexArrayGL & handle(const VertexArray & vertexarray);

    const Program & basicShader() const;
    const Program & texShader() const;

  private:
    RenderCommand & createRenderCommand(bool translucent,
                                        int indexCount, int vertexCount,
                                        std::size_t vertexSize, std::size_t uniformSize,
                                        unsigned *& mappedIndexBuffer,
                                        void *& mappedVertexBuffer,
                                        void *& mappedUniformBuffer,
                                        float & depth,
                                        const Program & program,
                                        const std::map<QByteArray, const Texture *> & textures);

    template <typename Vertex, typename UniformBlock>
    RenderCommand & createRenderCommand(bool translucent,
                                        int indexCount, int vertexCount,
                                        unsigned *& mappedIndexBuffer,
                                        Vertex *& mappedVertexBuffer,
                                        UniformBlock *& mappedUniformBuffer,
                                        float & depth,
                                        const Program & program,
                                        const std::map<QByteArray, const Texture *> & textures);

    struct SharedBuffer;
    template <typename T>
    std::pair<T *, SharedBuffer *> sharedBuffer(
        std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset);

    std::pair<void *, SharedBuffer *> sharedBuffer(
        std::size_t vertexSize, std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset);

    //////////////////////////////////////////////////////////////////////////
    /// </Luminousv2>
    //////////////////////////////////////////////////////////////////////////
  protected:
    virtual void beforeTransformChange();
  private:
    void drawCircleWithSegments(Nimble::Vector2f center, float radius, const float *rgba, int segments);
    void drawCircleImpl(Nimble::Vector2f center, float radius, const float *rgba);

    void clearTemporaryFBO(std::shared_ptr<FBOPackage> fbo);

    Luminous::RenderContext * m_resources;
    class Internal;
    Internal * m_data;
  };

  class CustomOpenGL : Patterns::NotCopyable
  {
  public:
    CustomOpenGL(RenderContext & r) : m_r(r)
    {
      r.flush2();
      r.bindProgram(0);
      // glDisable(GL_TEXTURE_2D);
    }
    ~CustomOpenGL() { /*m_r.restart();*/ }
  private:
    RenderContext & m_r;
  };

  class VertexAttribArrayStep : public Patterns::NotCopyable
  {
  public:
    VertexAttribArrayStep(int pos, int elems, GLenum type, GLboolean normalized, size_t stride,
                          size_t offset);

    VertexAttribArrayStep(GLSLProgramObject & prog, const char * attribname,
                          int elems, GLenum type, GLboolean normalized, size_t stride,
                          size_t offset, const char * userstr = 0);

    ~VertexAttribArrayStep ();

  private:
    int m_pos;
  };

  template <>
  void * RenderContext::mapBuffer<void>(const Buffer & buffer, int offset, std::size_t length,
                                        Radiant::FlagsT<Buffer::MapAccess> access);

  template <typename T>
  T * RenderContext::mapBuffer(const Buffer & buffer, int offset, std::size_t length,
                               Radiant::FlagsT<Buffer::MapAccess> access)
  {
    return reinterpret_cast<T*>(mapBuffer<void>(buffer, offset, length, access));
  }

  template <typename T>
  inline T * RenderContext::mapBuffer(const Buffer & buffer,
                                      Radiant::FlagsT<Buffer::MapAccess> access)
  {
    return mapBuffer<T>(buffer, 0, buffer.size(), access);
  }

  template <typename Vertex, typename Uniform>
  RenderCommand & RenderContext::createRenderCommand(bool translucent,
                                                     int indexCount, int vertexCount,
                                                     unsigned *& mappedIndexBuffer,
                                                     Vertex *& mappedVertexBuffer,
                                                     Uniform *& mappedUniformBuffer,
                                                     float & depth,
                                                     const Program & program,
                                                     const std::map<QByteArray, const Texture *> & textures)
  {
    return createRenderCommand(translucent,
                               indexCount, vertexCount,
                               sizeof(Vertex), sizeof(Uniform),
                               mappedIndexBuffer, reinterpret_cast<void *&>(mappedVertexBuffer),
                               reinterpret_cast<void *&>(mappedUniformBuffer),
                               depth, program, textures);
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
