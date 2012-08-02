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

    /// Blending function type
    enum BlendFunc {
      BLEND_USUAL,
      BLEND_NONE,
      BLEND_ADDITIVE,
      BLEND_SUBTRACTIVE
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

    /// Sets the associated window for this context
    /// @param window window to associate
    void setWindow(const Luminous::MultiHead::Window * window,
                   const Luminous::MultiHead::Area * area);

    const Luminous::MultiHead::Window * window() const;
    const Luminous::MultiHead::Area * area() const;

    /// Returns the resources of this context
    /// @todo make deprecated
    Luminous::RenderContext * resources() { return this; }

    /// Prepares the context for rendering a frame. This is called once for
    /// every frame before rendering.
    virtual void prepare();

    /// Notifies the context that a frame has been rendered. This is called
    /// once after each frame.
    virtual void finish();

    void pushViewTransform();
    void popViewTransform();
    void setViewTransform(const Nimble::Matrix4 &);
    const Nimble::Matrix4 & viewTransform() const;
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

    // Returns the visible area (bottom of the clip stack).
    // @todo does not return anything useful
    //const Nimble::Rectangle & visibleArea() const;

    FBOHolder getTemporaryFBO(Nimble::Vector2 basicsize,
                              float scaling, uint32_t flags = 0);
    /// @endcond

    // Render utility functions:

    /** Draws an arc
      @param center center of the arc
      @param radius radius of the arc
      @param width width of the arc
      @param fromRadians start angle in radians
      @param toRadians end angle in radians
      @param fill color and other parameters for the arc
      @param lineSegments number of steps
      */
    void drawArc(Nimble::Vector2f center, float radius, float width, float fromRadians, float toRadians, Luminous::Style & fill, unsigned int lineSegments = 0);

    /** Draws a circle
      @param center center of the circle
      @param radius radius of the circle
      @param width width of the circle
      @param fill color and other parameters for the circle
      @param lineSegments number of steps
      */
    void drawCircle(Nimble::Vector2f center, float radius, float width, Luminous::Style & fill, unsigned int lineSegments = 0);

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
    void drawWedge(const Nimble::Vector2f & center, float radius1, float radius2, float fromRadians, float toRadians, float width, Style & style, int segments);

    void drawRect(const QRectF & area, Style &fill);
    void drawRect(const Nimble::Rect & area, Luminous::Style & fill);

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
    RenderBuilder<Vertex, UniformBlock> drawTriStripT(const Nimble::Vector2f * vertices, unsigned int vertexCount, Style & style);

    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> drawTexTriStripT(const Nimble::Vector2f * vertices, const Nimble::Vector2f * uvs, unsigned int vertexCount, Style & style);    

    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> drawPointsT(const Nimble::Vector2f * vertices, unsigned int vertexCount, float size, Style & style);

    template <typename Vertex, typename UniformBlock>
    RenderContext::RenderBuilder<Vertex, UniformBlock> drawLineStripT(const Nimble::Vector2f * vertices, unsigned int vertexCount, float width, Style & style);

    void drawRectWithHole(const Nimble::Rect & area, const Nimble::Rect & hole, Luminous::Style & fill);
    void drawLine(const Nimble::Vector2 & p1, const Nimble::Vector2 & p2, float width, Luminous::Style & style);
    void drawPolyLine(const Nimble::Vector2 * vertices, unsigned int numVertices, float width, Luminous::Style & style);
    void drawPoints(const Nimble::Vector2f * points, size_t numPoints, float size, Luminous::Style & style);
    void drawQuad(const Nimble::Vector2f * corners, Luminous::Style & fill);

    /// Sets the current blend function, and enables blending
    /** If the function is BLEND_NONE, then blending is disabled.
    @param f blend function */
    void setBlendFunc(BlendFunc f);
    /// Enables the current blend mode defined with setBlendFunc
    void useCurrentBlendMode();

    /// Returns a pointer to an array of human-readable blending mode strings
    static const char ** blendFuncNames();

    /// Adds the render counter by one
    /** The render counter is used to track how many objects have been rendered since the coutner was
        last reset. Thsi can be useful for checking that object culling works as intended. */
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

    void draw(PrimitiveType primType, unsigned int offset, unsigned int primitives);
    void drawIndexed(PrimitiveType primType, unsigned int offset, unsigned int primitives);

  private:
    RenderCommand & createRenderCommand(int indexCount, int vertexCount,
                                        std::size_t vertexSize, std::size_t uniformSize,
                                        unsigned *& mappedIndexBuffer,
                                        void *& mappedVertexBuffer,
                                        void *& mappedUniformBuffer,
                                        float & depth, const Style & style);

    template <typename Vertex, typename UniformBlock>
    RenderCommand & createRenderCommand(int indexCount, int vertexCount,
                                        unsigned *& mappedIndexBuffer,
                                        Vertex *& mappedVertexBuffer,
                                        UniformBlock *& mappedUniformBuffer,
                                        float & depth, const Style & style);

    struct SharedBuffer;
    template <typename T>
    std::pair<T *, SharedBuffer *> sharedBuffer(
        std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset);

    std::pair<void *, SharedBuffer *> sharedBuffer(
        std::size_t vertexSize, std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset);

    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> render(Luminous::PrimitiveType type, int indexCount, int vertexCount, float primitiveSize, const Style & style);

    TextureGL & handle(Texture & texture);

    Program & basicShader();
    Program & texShader();

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
  RenderCommand & RenderContext::createRenderCommand(int indexCount, int vertexCount,
                                                     unsigned *& mappedIndexBuffer,
                                                     Vertex *& mappedVertexBuffer,
                                                     Uniform *& mappedUniformBuffer,
                                                     float & depth,
                                                     const Style & style)
  {
    return createRenderCommand(indexCount, vertexCount,
                               sizeof(Vertex), sizeof(Uniform),
                               mappedIndexBuffer, reinterpret_cast<void *&>(mappedVertexBuffer),
                               reinterpret_cast<void *&>(mappedUniformBuffer),
                               depth, style);
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

#endif
