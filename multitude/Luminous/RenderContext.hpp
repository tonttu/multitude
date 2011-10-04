/* COPYRIGHT
 */

#ifndef LUMINOUS_RENDERCONTEXT_HPP
#define LUMINOUS_RENDERCONTEXT_HPP

#include <Luminous/Luminous.hpp>

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

#include <Nimble/Rectangle.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Splines.hpp>

#include <Radiant/Defines.hpp>

namespace Luminous
{
  class Texture2D;
  class GLContext;
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

      FBOPackage(Luminous::RenderContext *res = 0) : m_fbo(res), m_rbo(res), m_tex(res), m_users(0) {}
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
    RenderContext(const Luminous::MultiHead::Window * window = 0);
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

    static QString locateStandardShader(const QString & filename);

    /// Prepares the context for rendering a frame. This is called once for
    /// every frame before rendering.
    virtual void prepare();

    /// Notifies the context that a frame has been renreded. This is called
    /// once after each frame.
    virtual void finish();

    void pushViewTransform();
    void popViewTransform();
    void setViewTransform(const Nimble::Matrix4 &);

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

    // Render functions:

    /** Draw a rectangle outline with given thickness and color.
    @param rect rectangle to draw
    @param thickness thickness of the outline
    @param rgba color of the outline */
    void drawLineRect(const Nimble::Rectf & rect, float thickness, const float * rgba);
    /** Draws a solid, anti-aliased rectangle, with given color. If textures are active,
        the rectangle is filled with the current texture modulated by the given color.
    @param rect rectangle to draw
    @param rgba fill color */
    void drawRect(const Nimble::Rectf & rect, const float * rgba);

    /** Draws a solid, antialiased circle
        @param center Center of the circle

        @param radius Radius of the circle

        @param rgba The color of the circle in RGBA format

        @param segments Number of segments used in the circle. Deprecated, specifying segments will actually slow rendering.
    */
    void drawCircle(Nimble::Vector2f center, float radius,
                    const float * rgba, int segments = -1);

    /** Draws an arc
      @param center center of the arc
      @param radius radius of the arc
      @param fromRadians start angle in radians
      @param toRadians end angle in radians
      @param width width of the arc
      @param blendWidth width of the blending region
      @param rgba color of the arc in RGBA format
      @param segments number of segments to use
      */
    void drawArc(Nimble::Vector2f center, float radius, float fromRadians, float toRadians, float width, float blendWidth, const float * rgba, int segments);

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
    void drawWedge(Nimble::Vector2f center, float radius1, float radius2, float fromRadians, float toRadians, float width, float blendWidth, const float * rgba, int segments);

    /** Draws a line that contains multiple segments.

        @param vertices Pointer to the line vertices

        @param n Number of vertices

        @param width Width of the line

        @param rgba The line color in RGBA format
     */
    void drawPolyLine(const Nimble::Vector2f * vertices, int n,
                      float width, const float * rgba);

    /** Draws a line between two points.

        @param p1 The first point
        @param p2 The second point

        @param width Width of the line

        @param rgba The line color in RGBA format
    */

    void drawLine(Nimble::Vector2f p1, Nimble::Vector2f p2,
                  float width, const float * rgba);

    /** Draw a cubic bzier curve
        @param controlPoints array of 4 control points
        @param width width of the curve
        @param rgba array of 4 color components
    */
    void drawCurve(Nimble::Vector2* controlPoints, float width, const float * rgba=0);

    /** Draws a spline.
      @param spline spline to draw
      @param width width of the spline
      @param rgba color of the spline
      @param step step to use when evaluating the spline
    */
    void drawSpline(Nimble::Interpolating & spline, float width, const float * rgba=0, float step=1.0f);
    /** Draw a textured rectangle with given color.

        @param size The size of the rectangle to be drawn.
        @param rgba The color in RGBA format. If the argument is null,
        then it will be ignored.
    */
    void drawTexRect(Nimble::Vector2 size, const float * rgba);
    /** @copybrief drawTexRect

        @param size The size of the rectangle to be drawn.
        @param rgba The color in RGBA format. If the argument is null,
               then it will be ignored.
        @param texUV The maximum texture coordinate values **/
    void drawTexRect(Nimble::Vector2 size, const float * rgba,
                     const Nimble::Rect & texUV);
    /** @copybrief drawTexRect
    @param area The rectangle to draw.
    @param rgba The color in RGBA format. If the argument is null,
           then it will be ignored.
    @param texUV The maximum texture coordinate values **/
    void drawTexRect(const Nimble::Rect & area, const float * rgba,
                     const Nimble::Rect & texUV);
    /** @copybrief drawTexRect
    @param area The rectangle to draw.
    @param rgba The color in RGBA format. If the argument is null,
           then it will be ignored.
    @param texUV Array of texture coordinates for multitexturing.
    @param uvCount The number of texture coordinates to fill.**/
    void drawTexRect(const Nimble::Rect & area, const float * rgba,
                     const Nimble::Rect * texUV, int uvCount);
    /** @copybrief drawTexRect
        @param size The size of the rectangle to be drawn.
        @param rgba The color in RGBA format. If the argument is null,
               then it will be ignored.
        @param texUV The maximum texture coordinate values **/
    void drawTexRect(Nimble::Vector2 size, const float * rgba,
                     Nimble::Vector2 texUV);
    /** @copybrief drawTexRect
        @param rgba The color in RGBA format. If the argument is null,
               then it will be ignored.
        @param area The rectangle to drawn **/
    void drawTexRect(const Nimble::Rect & area, const float * rgba);

    void drawRect(const Nimble::Rect & area, const Luminous::Style & fill);
    void drawRectWithHole(const Nimble::Rect & area,
                          const Nimble::Rect & hole,
                          const Luminous::Style & fill);

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

    /// @internal
    /// Sets the current rendering context
    void setGLContext(Luminous::GLContext *);

    /// Returns a handle to the current OpenGL rendering context
    /** This function is seldom necessary, and its use is deprecated and unsupported.
        On some platforms this call may return null.
        */
    /// @internal
    MULTI_ATTR_DEPRECATED(Luminous::GLContext * glContext());

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
    static RenderContext * GLSLreadContext();

    void bindTexture(GLenum textureType, GLenum textureUnit, GLuint textureId);
    /// Bind GLSL program object
    void bindProgram(GLSLProgramObject * program);

    void flush();

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


  class VertexAttribArrayStep : public Patterns::NotCopyable
  {
  public:
    VertexAttribArrayStep(int pos, int elems, GLenum type, size_t stride,
                          void * offset)
                            : m_pos(pos)
    {
      glEnableVertexAttribArray(pos);
      glVertexAttribPointer(pos, elems, type, GL_FALSE,
                            stride, offset);
    }

    ~VertexAttribArrayStep ()
    {
      glDisableVertexAttribArray(m_pos);
    }

  private:
    int m_pos;
  };

}

#endif
