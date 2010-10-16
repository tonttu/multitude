/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef LUMINOUS_RENDERCONTEXT_HPP
#define LUMINOUS_RENDERCONTEXT_HPP

#include <Luminous/Luminous.hpp>
#include <Luminous/FramebufferObject.hpp>
#include <Luminous/Transformer.hpp>
#include <Luminous/GLResource.hpp>
#include <Luminous/GLResources.hpp>
#include <Luminous/Export.hpp>
#include <Luminous/VertexBuffer.hpp>
#include <Luminous/GLSLProgramObject.hpp>

#include <Nimble/Rectangle.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Splines.hpp>

#include <Radiant/Defines.hpp>

namespace Luminous
{
  class Texture2D;
  class GLContext;

  /// RenderContext contains the current rendering state.
  class LUMINOUS_API RenderContext : public Transformer
  {
  public:

    /// Blending function type
    enum BlendFunc {
      BLEND_USUAL,
      BLEND_NONE,
      BLEND_ADDITIVE,
      BLEND_SUBTRACTIVE
    };

    class FBOPackage;

    class FBOPackage : public GLResource
    {
    public:
      friend class FBOHolder;
      friend class RenderContext;

      FBOPackage(Luminous::GLResources *res = 0) : m_fbo(res), m_rbo(res), m_tex(res), m_users(0) {}
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
    RenderContext(Luminous::GLResources * resources, const Luminous::MultiHead::Window * window);
    virtual ~RenderContext();

    void setWindow(const Luminous::MultiHead::Window * window);

    /// Returns the resources of this context
    Luminous::GLResources * resources() { return m_resources; }

    /// Prepares the context for rendering a frame. This is called once for
    /// every frame before rendering.
    virtual void prepare();

    /// Notifies the context that a frame has been renreded. This is called
    /// once after each frame.
    virtual void finish();

    /// Sets the rendering recursion limit for the context. This is relevant
    /// for ViewWidgets which can cause recursive rendering of the scene.
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

    /// Checks if the given rectangle is visible (not clipped).
    bool isVisible(const Nimble::Rectangle & area);

    void pushDrawBuffer(GLenum dest, FBOPackage * );
    void popDrawBuffer();

    // Returns the visible area (bottom of the clip stack).
    // @todo does not return anything useful
    //const Nimble::Rectangle & visibleArea() const;

    /// @cond
    FBOHolder getTemporaryFBO(Nimble::Vector2 basicsize,
                              float scaling, uint32_t flags = 0);
    /// @endcond

    // Render functions:

    /** Draw a line rectangle, with given thickness and color. */
    void drawLineRect(const Nimble::Rectf & rect, float thickness, const float * rgba);
    /** Draws a solid (antialiased) rectangle, with given color. If textures are active,
        the rectangle is filled with the current texture */
    void drawRect(const Nimble::Rectf & rect, const float * rgba);

    /** Draws a solid, antialiased circle
        @arg center Center of the circle

        @arg radius Radius of the circle

        @arg rgba The color of the circle in RGBA format

        @arg segments Number of segments used in the circle. Deprecated, specifying segments will actually slow rendering.

    */
    void drawCircle(Nimble::Vector2f center, float radius,
                    const float * rgba, int segments = -1);

    /** Draws an arc
      @arg center center of the arc
      @arg radius radius of the arc
      @arg fromRadians start angle in radians
      @arg toRadians end angle in radians
      @arg width width of the arc
      @arg blendWidth width of the blending region
      @arg rgba color of the arc in RGBA format
      @arg segments number of segments to use
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

        @arg vertices Pointer to the line vertices

        @arg n Number of vertices

        @arg width Width of the line

        @arg rgba The line color in RGBA format
     */
    void drawPolyLine(const Nimble::Vector2f * vertices, int n,
                      float width, const float * rgba);

    /** Draws a line between two points.

        @arg p1 The first point
        @arg p2 The second point

        @arg width Width of the line

        @arg rgba The line color in RGBA format
    */

    void drawLine(Nimble::Vector2f p1, Nimble::Vector2f p2,
                  float width, const float * rgba);

    /** Draw a cubic bézier curve
        @arg controlPoints array of 4 control points
        @arg width width of the curve
        @arg rgba array of 4 color components
    */
    void drawCurve(Nimble::Vector2* controlPoints, float width, const float * rgba=0);

    /** Draws a spline.
      @arg spline spline to draw
      @arg width width of the spline
      @arg rgba color of the spline
      @arg step step to use when evaluating the spline
    */
    void drawSpline(Nimble::Splines::Interpolating & spline, float width, const float * rgba=0, float step=1.0f);
    /** Draw a textured rectangle with given color.

        @arg size The size of the rectangle to be drawn.

        @arg rgba The color in RGBA format. If the argument is null,
        then it will be ignored.
    */
    void drawTexRect(Nimble::Vector2 size, const float * rgba);
    /// @copydoc drawTexRect
    void drawTexRect(Nimble::Vector2 size, const float * rgba,
                     const Nimble::Rect & texUV);
    /// @copydoc drawTexRect
    void drawTexRect(Nimble::Vector2 size, const float * rgba,
                     Nimble::Vector2 texUV);
    /// @copydoc drawTexRect
    void drawTexRect(const Nimble::Rect & area, const float * rgba);

    /// Sets the current blend function, and enables blending
    /** If the function is BLEND_NONE, then blending is disabled. */
    void setBlendFunc(BlendFunc f);
    /// Enables the current blend mode defined with setBlendFunc
    void useCurrentBlendMode();

    /// Returns a pointer to an array of human-readable blending mode strings
    static const char ** blendFuncNames();

    /// Adds the render counter by one
    /** */
    void addRenderCounter();

    Nimble::Vector2 contextSize() const;

    /// @internal
    void pushViewStack();
    /// @internal
    /// Pops view stack, leaves current texture attached
    void popViewStack();

    /// @internal
    /// Sets the current rendering context
    void setGLContext(Luminous::GLContext *);

    /// Returns a handle to the current OpenGL rendering context
    /** This function is seldom necessary, and its use is deprecated and unsupported.
        On some platforms this call may return null.
        */
    MULTI_ATTR_DEPRECATED(Luminous::GLContext * glContext());


  private:
    void drawCircleWithSegments(Nimble::Vector2f center, float radius, const float *rgba, int segments);
    void drawCircleImpl(Nimble::Vector2f center, float radius, const float *rgba);

    void clearTemporaryFBO(std::shared_ptr<FBOPackage> fbo);

    Luminous::GLResources * m_resources;
    class Internal;
    Internal * m_data;
  };

}

#endif
