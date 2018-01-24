/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_RENDERCONTEXT_HPP
#define LUMINOUS_RENDERCONTEXT_HPP

#include <Luminous/Luminous.hpp>
#include <Luminous/RenderDriver.hpp>
#include <Luminous/Transformer.hpp>
#include <Luminous/Style.hpp>
#include <Luminous/RenderContext.hpp>
#include <Luminous/Export.hpp>
#include <Luminous/VertexArray.hpp>
#include <Luminous/Buffer.hpp>
#include <Luminous/RenderCommand.hpp>
#include <Luminous/PostProcessFilter.hpp>
#include <Luminous/MultiHead.hpp>

#include "FrameBufferGL.hpp"
#include "BufferGL.hpp"
#include "CullMode.hpp"

#include <Nimble/Rectangle.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Splines.hpp>
#include <Nimble/ClipStack.hpp>

#include <Radiant/Defines.hpp>

#include <QRectF>

// #define RENDERCONTEXT_SHAREDBUFFER_MAP

namespace Luminous
{
  class GLSLProgramObject;

  /// RenderContext contains the current rendering state.
  /// Each RenderContext is tied to single RenderThread.
  class LUMINOUS_API RenderContext : public Transformer, public OpenGLAPI
  {
  public:

    /// TextFlags can be used to give performance hint to RenderContext about
    /// the layout of the text.
    enum TextFlags
    {
      /// Text layout changes usually every frame
      TextDynamic,
      /// Text layout does not change often
      TextStatic
    };

    /// How are UV coordinates generated for objects
    enum TextureMappingMode
    {
      /// Flat mapping looks like the texture was projected to the object
      /// bounding box, basically clipping the texture to the shape of the
      /// rendered object.
      TEXTURE_MAPPING_FLAT,
      /// UV coordinates follow the ellipse tangent and normal. U-coordinate
      /// rotates with the object from 0 to 1 in clockwise direction.
      /// V-coordinate is 0 on the internal edge or center and 1 at the outer
      /// edge.
      TEXTURE_MAPPING_TANGENT
    };

    /** Proxy object for building rendering command.

        When created contains abstraction of the actual rendering command including
        shader program and the placeholders for the data to be rendered (vertices, indices
        and uniforms).

        Instance of this class is returned from low level rendering commands.
        User needs to fill vertex data to the returned object. In some cases
        (depending which function returned the object and is indexed drawing used)
        one has to to fill indices and uniforms as well.

        @tparam Vertex Type of the vertex feed to the shader program.
        @tparam UniformBlock Type of the uniform block feed to the shader program.
    */
    template <typename Vertex, typename UniformBlock>
    struct RenderBuilder
    {
    private:
      RenderBuilder() : idx(), uniform(), vertex(), command(), depth(0.0f) {}

      friend class RenderContext;

    public:
      /// Indices for rendering
      unsigned int * idx;
      /// Uniform block for uniforms
      UniformBlock * uniform;
      /// Vertices for rendering
      Vertex * vertex;
/// @cond
      /// Abstraction of the rendering command. Users do not need to interact with this.
      RenderCommand * command;
/// @endcond
      /// Depth of rendering. Precalculated automatically based on the order of rendering calls.
      float depth;
    };

/// @cond

    struct SharedBuffer
    {
      SharedBuffer(Buffer::Type type) : type(type), reservedBytes(0) {}
      SharedBuffer(SharedBuffer && shared)
        : buffer(std::move(shared.buffer)),
#ifndef RENDERCONTEXT_SHAREDBUFFER_MAP
          data(std::move(shared.data)),
#endif
          type(shared.type),
          reservedBytes(shared.reservedBytes)
      {}
      SharedBuffer & operator=(SharedBuffer && shared)
      {
        buffer = std::move(shared.buffer);
#ifndef RENDERCONTEXT_SHAREDBUFFER_MAP
        std::swap(data, shared.data);
#endif
        type = shared.type;
        reservedBytes = shared.reservedBytes;
        return *this;
      }

      Buffer buffer;
#ifndef RENDERCONTEXT_SHAREDBUFFER_MAP
      std::vector<char> data;
#endif
      Buffer::Type type;
      std::size_t reservedBytes;
    };

/// @endcond

    /// Constructs a new render context and associates the given resources to it
    /// @param driver render driver to use
    /// @param window window to associate this context with
    RenderContext(Luminous::RenderDriver & driver, const Luminous::MultiHead::Window * window = 0);
    /// Closes this render context. Invalidates all GL resources tied to this context.
    /// However note, that CPU equivalents of GL classes are still valid.
    virtual ~RenderContext();

    Luminous::RenderDriver &renderDriver();

    /// Sets the associated area for this context at the moment
    /// @param area area to associate
    void setWindowArea(const Luminous::MultiHead::Window * window, const Luminous::MultiHead::Area * area);

    /// Returns the window associated to the current area.
    /// @return Pointer to current window.
    const Luminous::MultiHead::Window * window() const;
    /// Returns the current area.
    /// @return Pointer to current area
    const Luminous::MultiHead::Area * area() const;

    /// Initializes the context. Is called automatically by rendering thread.
    void initialize();

    /// Called once for every frame before rendering. For internal implementation.
    void beginFrame(Radiant::TimeStamp frameTime);
    /// Called once for every frame after rendering. For internal implementation.
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

    void processFilter(Luminous::PostProcessFilterPtr filter, Nimble::Rect filterRect);

    /// @endcond

    /// Returns transformation from the world coordinates (pixels) to projected eye
    /// coordinates (normalized device coordinates).
    /// @param Matrix representing current view transform
    const Nimble::Matrix4 & viewTransform() const;
    /// Push new view transform to the view transform stack.
    /// @param m Matrix representing view transform
    void pushViewTransform(const Nimble::Matrix4 & m);
    /// Pop the current matrix from view transform stack.
    void popViewTransform();

    /// Returns the target framebuffer for the current rendering operations.
    /// @return Draw frame buffer
    const FrameBuffer & currentFrameBuffer() const;

    /// Sets the rendering recursion limit for the context. This is relevant
    /// for ViewWidgets which can cause recursive rendering of the scene.
    /// @param limit recursion depth limit
    void setRecursionLimit(size_t limit) ;
    /// Returns the recursion limit
    /// @return Recursion limit
    size_t recursionLimit() const;

    /// Increment the current clip mask recursion depth
    void pushClipMaskStack(size_t depth);
    /// Get current clip mask recursion depth
    /// @return Current clip mask recursion depth
    size_t currentClipMaskDepth() const;

    /// Decrement the current clip mask recursion depth
    void popClipMaskStack();

    /// Save the current clipping stack and start with a empty one
    void pushClipStack();

    /// Restores the previously saved clipping stack
    void popClipStack();

    /// Pushes a clipping rectangle to the context
    void pushClipRect(const Nimble::Rectangle & r);
    /// Pops a clipping rectangle from the context
    void popClipRect();

    /// Get the active clip stack. The clip stack is only valid during
    /// rendering. Do not call this function if the clip stack is empty.
    /// @return current clip stack
    /// @sa isClipStackEmpty()
    const Nimble::ClipStack & clipStack() const;

    /// Check if the current clip stack is empty.
    /// @return true if the clip stack is empty; otherwise false
    bool isClipStackEmpty() const;

    /// Checks if the given rectangle is visible (not clipped).
    /// @param area Area to check
    /// @return Was the area visible
    bool isVisible(const Nimble::Rectangle & area);

    /// Called by ViewWidget::renderContent before rendering the view scene.
    /// Can be called from other view-type widgets that render a widget
    /// hierarchy manually. This updates viewWidgetPath and viewWidgetPathId.
    /// Every call to pushViewWidget needs matching popViewWidget call.
    /// @param id Widget id
    void pushViewWidget(Valuable::Node::Uuid id);
    /// Pops last view widget from viewWidgetPath / viewWidgetPathId
    void popViewWidget();
    /// When called from a widget rendering functions, this returns the list of
    /// view widgets that are currently rendering this widget. Returns an empty
    /// vector if the widget is not inside a view.
    const std::vector<Valuable::Node::Uuid> & viewWidgetPath() const;
    /// String representation of viewWidgetPath that can be used as a key or id.
    const QByteArray & viewWidgetPathId() const;

    // Render utility functions:

    /// Draw an arc defined as a segment of circle
    /// @param center Center of the arc
    /// @param radius Radius of the arc
    /// @param fromRadians Start angle in radians
    /// @param toRadians End angle in radians
    /// @param style Stroke definition of the arc (color, width)
    /// @param lineSegments Number of steps (precision)
    void drawArc(const Nimble::Vector2f & center, float radius, float fromRadians, float toRadians, const Luminous::Style & style, unsigned int lineSegments = 0);

    /// Draw a possibly filled circle
    /// @param center Center of the circle
    /// @param radius Radius of the circle
    /// @param style Stroke and fill definitions of the circle (colors and stroke width)
    /// @param lineSegments Number of steps (precision)
    /// @param fromRadians Start angle in radians
    /// @param toRadians End angle in radians
    void drawCircle(const Nimble::Vector2f & center, float radius, const Luminous::Style & style, unsigned int lineSegments = 0, float fromRadians=0, float toRadians=Nimble::Math::TWO_PI);

    /// Draws an ellipse
    /// @param center Center of the ellipse
    /// @param axis Axis of the ellipse (orientation and size)
    /// @param otherAxisLength The length of the other axis
    /// @param style Stroke and fill definitions similar to circle
    /// @param lineSegments Number of steps (precision)
    /// @param fromRadians Start angle in radians
    /// @param toRadians End angle in radians
    void drawEllipse(Nimble::Vector2f center,
                     Nimble::Vector2f axis,
                     float otherAxisLength,
                     const Luminous::Style & style,
                     unsigned int lineSegments = 0,
                     float fromRadians = 0, float toRadians = Nimble::Math::TWO_PI);

    /// Draws a constant width donut defined as an ellipse and a hole in the center.
    /// @param center Center of the donut
    /// @param axis Axis of the ellipse (orientation and size)
    /// @param otherAxisLength Other axis legth of the ellipse.
    /// @param width The width of the donut.
    /// @param style Stroke, fill and texturing definitions.
    /// @param linesegments Number of steps (precision)
    /// @param fromRadians Start angle in radians
    /// @param toRadians End angle in radians
    /// @param textureCoordMode How are the texture coordinates generated
    void drawDonut(const Nimble::Vector2f & center,
                   Nimble::Vector2 axis,
                   float otherAxisLength,
                   float width,
                   const Luminous::Style & style,
                   unsigned int linesegments = 0,
                   float fromRadians=0, float toRadians=Nimble::Math::TWO_PI,
                   TextureMappingMode textureMappingMode = TEXTURE_MAPPING_FLAT);

    /// Push the given opacity to render context. The resulting opacity will be
    /// the current opacity multiplied by the given value.
    /// @param opacity opacity to push
    void pushOpacity(float opacity);
    /// Pop the current opacity from the stack
    void popOpacity();
    /// Get the current opacity
    /// @return the current opacity
    float opacity() const;
    void setOpacity(float opacity);

    /// Pushes new frame buffer to the stack.
    /// @param target frame buffer for rendering commands.
    void pushFrameBuffer(const FrameBuffer & target);
    /// Pops the current frame buffer from the stack.
    void popFrameBuffer();

    typedef uint64_t ObjectMask;
    void pushBlockObjects(ObjectMask objectMask);
    void popBlockObjects();
    bool blockObject(ObjectMask mask) const;

    //////////////////////////////////////////////////////////////////////////
    // Implementation

    /** Returns RenderBuilder for the given drawing query. Call to this function allocates the queried resources
        from driver and associates those with the given shader and other parameters. After calling this function
        user needs to feed data through the returned builder object.

        @code
        // Draw a triangle using drawPrimitiveT
        Luminous::Style style;
        // Call to drawPrimitiveT adds the rendering command to queue
        RenderBuilder<BasicVertex, BasicUniformBlock> builder = rc.drawPrimitiveT(Luminous::PRIMITIVE_TRIANGLE,
                                                                                  0, // Do not use indexed drawing
                                                                                  3, // Three vertices
                                                                                  rc.basicShader(), // Use basic shader
                                                                                  Radiant::Color("red"),
                                                                                  0, // This would be width of line or point
                                                                                  style // No need to pass additional parameters
                                                                                  );
        // Now fill the data of the builder (vertices of triangle)
        builder.vertex[0].location = Nimble::Vector2(0, 0);
        builder.vertex[1].location = Nimble::Vector2(1, 0);
        builder.vertex[2].location = Nimble::Vector2(.5, 1);

        // That is all we needed: request to draw and supported that request with data
        @endcode

        @param primType Primitives to render
        @param indexCount How many indices are to be specified. If zero, indices are effectively set to 0,1,...,vertexCount-1
        @param vertexCount How many vertices are to be specified.
        @param shader GLSL-program to use.
        @param color Color for the corresponding uniform.
        @param width Width for the rendered primitive. Only for points and lines.
        @param style Style for the rendering.

        @tparam Vertex Type of the vertex feed to the shader shader.
        @tparam UniformBlock Type of the uniformblock. Needs at least to include public fields @c projMatrix (Nimble::Matrix4),
                             @c modelMatrix (Nimble::Matrix4), @c color (Nimble::Vector4) and @c depth (float)
                             (see Luminous::BasicUniformBlock).

        @return Returns RenderBuilder-object which contains the interface to the rendering resources acquired by call to this function.
      */
    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> drawPrimitiveT(Luminous::PrimitiveType primType, unsigned int indexCount, unsigned int vertexCount,
      const Luminous::Program & shader, const Radiant::ColorPMA & color, float width, const Luminous::Style & style);

    /// Draws a rectangle with rectangular hole
    /// @param area Outer rectangle
    /// @param hole Inner rectangle defining the hole
    /// @param style Stroke, fill and texturing options
    void drawRectWithHole(const Nimble::Rectf & area, const Nimble::Rect & hole, const Luminous::Style & style);

    /// Draws a line
    /// @param p1 Start point of the line
    /// @param p2 End point of the line
    /// @param style Stroke definition of a line.
    void drawLine(const Nimble::Vector2f & p1, const Nimble::Vector2f & p2, const Luminous::Style & style);

    /// Draws a polyline
    /// @param begin Initial position of sequence
    /// @param numVertices Number of vertices (one more than lines in polyline)
    /// @param style Stroke definition of a line.
    /// @tparam InputIterator Iterator to vertices. Needs to have operator++ and operator*
    template <typename InputIterator>
    void drawPolyLine(InputIterator begin, size_t numVertices, const Luminous::Style & style);

    /// Draws a set of points
    /// @param begin Initial position of sequence
    /// @param numPoints Number of points
    /// @param style Color and size passed as stroke parameters
    /// @tparam InputIterator Iterator to vertices. Needs to have operator++ and operator*
    template <typename InputIterator>
    void drawPoints(InputIterator begin, size_t numPoints, const Luminous::Style & style);

    /// Draws a rectangle
    /// @param min Bottom left corner of a rectangle
    /// @param max Top right corner of a rectangle
    /// @param style Stroke, fill and texturing options
    MULTI_ATTR_DEPRECATED("This version of drawRect is deprecated, use drawRect(RectT, style) instead.", void drawRect(const Nimble::Vector2f & min, const Nimble::Vector2f & max, const Style &style));

    /// Draws a rectangle
    /// @param min Bottom left corner of a rectangle
    /// @param size Rectangle size
    /// @param style Stroke, fill and texturing options
    MULTI_ATTR_DEPRECATED("This version of drawRect is deprecated, use drawRect(RectT, style) instead.", void drawRect(const Nimble::Vector2f & min, const Nimble::SizeF & size, const Style & style));

    /// Draws a rectangle
    /// @param rext Rectangle to draw
    /// @param style Stroke, fill and texturing options
    template<typename T>
    void drawRect(const Nimble::RectT<T>& rect, const Style & style);

    /// Draws a rectangle
    /// @param rect Rectangle to draw
    /// @param uvs Texture coordinates
    /// @param style Stroke, fill and texturing options
    template<typename T>
    void drawRect(const Nimble::RectT<T>& rect, const Nimble::Rectf & uvs, const Style & style);

    /// Draws a quad with two triangles.
    /// The vertices of the first triangle are v[0], v[1], v[2]. The second triangle is
    /// defined by vertices v[1], v[3] and v[2]. Both of the triangles are assumed to have a
    /// counterclockwise orientation.
    /// @param v Vertices of a quad
    /// @param uvs Texture coordinates for the vertices
    /// @param style Stroke, fill and texturing options
    void drawQuad(const Nimble::Vector2 * v, const Nimble::Vector2 * uvs, const Style & style);

    /// @cond

    // Only for JavaScript. Do not use directly.
    void javascriptDrawRect(const Nimble::Rectf& rect, const Style& style);

    /// @endcond

    /// Draws text
    /// @param layout Text object to render
    /// @param location Text location in viewRect. Anything outside will be clipped.
    /// @param viewRect Where the text is located
    /// @param style Text style properties
    /// @param ignoreVerticalAlign If true the rendering assumes that vertical alignment is already
    ///        handled and baked in rendering location. Layout's options about vertical alignment are
    ///        ignored in that case.
    void drawText(const TextLayout & layout, const Nimble::Vector2f & location, const Nimble::Rectf & viewRect,
                  const TextStyle & style, bool ignoreVerticalAlign=false);

    /// Draws given string
    /// @param text String to render
    /// @param rect Where to render, acts also as a clipping rectangle
    /// @param style Text style properties
    /// @param flags Will text layout be dynamic or static
    void drawText(const QString & text, const Nimble::Rectf & rect, const TextStyle & style, TextFlags flags = TextStatic);

    /// Adds the render counter by one
    /// The render counter is used to track how many objects have been rendered since the counter was
    /// last reset. This can be useful for checking that object culling works as intended.
    void addRenderCounter();

    unsigned long renderCounter() const;

    /// Increases the unfinished render counter by one.
    /// @see unfinishedRenderCounter
    void addUnfinishedRenderCounter();

    /// The unfinished render counter is used to track how many objects didn't
    /// finish their rendering because of some resource wasn't ready.
    /// For example ImageWidget increases this counter if the best mipmap level
    /// wasn't yet generated and VideoWidget does the same if required video
    /// frame wasn't decoded yet. Unfixable permanent errors do not increase
    /// this counter.
    /// This is useful for example when rendering a scene to FBO and checking
    /// if everything was rendered there properly already this frame, or should
    /// the rendering be tried again on next frame.
    unsigned long unfinishedRenderCounter() const;

    /// Returns the size of the window of this RenderContext object.
    /// @return If the window is null, then Nimble::Size(10,10) is returned.
    Nimble::Size contextSize() const;

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

    /// Push scissor rectangle into scissor stack
    void pushScissorRect(const Nimble::Recti & scissorArea);
    /// Pop current scissor rectangle from stack
    void popScissorRect();
    /// Returns current scissor area
    /// @return Current scissor area
    const Nimble::Recti & currentScissorArea() const;

    /// Copies pixels from the read frame buffer to the draw frame buffer
    /// @sa FrameBuffer::setTargetBind
    /// @param src Rectangle defining source area in the read frame buffer
    /// @param dst Rectangle defining destination are in the draw frame buffer
    /// @param mask Buffers to blit
    /// @param filter Filtering mode for sampling
    void blit(const Nimble::Recti & src, const Nimble::Recti & dst,
              ClearMask mask = CLEARMASK_COLOR,
              Texture::Filter filter = Texture::FILTER_NEAREST);

    /// Set the active render buffers
    /// @param colorBuffer enables drawing to colorbuffer if set to true
    /// @param depthBuffer enables drawing to depthbuffer if set to true
    /// @param stencilBuffer enables drawing to stencilbuffer if set to true
    void setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer);

    /// Set the active blendmode
    /// @param mode Description of new blend mode.
    void setBlendMode(const BlendMode & mode);

    /// Set the active depthmode
    /// @param mode Description of the new depth mode
    void setDepthMode(const DepthMode & mode);

    /// Set the active stencil mode
    /// @param mode Stencil mode to use
    void setStencilMode(const StencilMode & mode);

    StencilMode stencilMode() const;

    /// Set the active cull mode
    /// @param mode Cull mode to use
    void setCullMode(const CullMode & mode);

    /// Set the active draw buffers
    /// @param buffers List of buffers to use
    /// The buffers may be one or more of the following:
    /// GL_NONE, GL_FRONT_LEFT, GL_FRONT_RIGHT, GL_BACK_LEFT, GL_BACK_RIGHT, GL_COLOR_ATTACHMENTn
    /// where n is a numerical value starting at 0
    void setDrawBuffers(const std::vector<GLenum>& buffers);

    /// Reset the active draw buffers to the default
    void setDefaultDrawBuffers();

    /// Specify front-facing polygons
    /// @param winding Winding of the front-facing polygons
    void setFrontFace(enum FaceWinding winding);

    /// Enable clipplanes for use in shaders
    /// @param planes a list of planes that will be enabled
    void enableClipPlanes(const QList<int> & planes);

    /// Disable clipplanes from being used in shaders
    /// @param planes a list of planes that will be disabled
    void disableClipPlanes(const QList<int> & planes);

    /// Reset the OpenGL state to default. The usage of this function by manually
    /// is not recommended
    void setDefaultState();

    /// Forces all queued rendering commands to execute. Is called internally
    /// at the end of frame so under normal conditions there is no need call
    /// it manually.
    void flush();

    /// Returns the GL resources handle corresponding to given program.
    /// @param program CPU side object representing the shader program
    /// @return Handle to OpenGL resources of given program
    ProgramGL & handle(const Program & program);

    /// Returns the GL resources handle corresponding to given texture.
    /// @param texture CPU side object representing the texture
    /// @return Handle to OpenGL resources of given texture
    TextureGL & handle(const Texture & texture);

    /// @see RenderDriverGL::findHandle
    TextureGL * findHandle(const Texture & texture);

    /// Returns the GL resources handle corresponding to given frame buffer.
    /// @param target CPU side object representing the frame buffer
    /// @return Handle to OpenGL resources of given frame buffer
    FrameBufferGL & handle(const FrameBuffer & target);

    /// Returns the GL resources handle corresponding to given render buffer.
    /// @param buffer CPU side object representing the render buffer
    /// @return Handle to OpenGL resources of given render buffer
    RenderBufferGL & handle(const RenderBuffer & buffer);

    /// Returns the GL resources handle corresponding to given buffer (vertex/index).
    /// @param buffer CPU side object representing the buffer
    /// @return Handle to OpenGL resources of given buffer
    BufferGL & handle(const Buffer & buffer);

    /// Returns the GL resources handle corresponding to given vertex array.
    /// @param vertexarray CPU side object representing the vertex array
    /// @param program Handle to OpenGL program associated with vertexarray
    /// @return Handle to OpenGL resources of queried vertex array
    VertexArrayGL & handle(const VertexArray & vertexarray, ProgramGL * program);

    /// Check if the given OpenGL extension is supported on the underlying
    /// OpenGL context
    /// @param name OpenGL extension name to check (case-sensitive)
    /// @return true if the extension is supported; otherwise false
    bool isOpenGLExtensionSupported(const QByteArray& name);

    /// Returns approximate of currently available GPU memory
    /// @return approximate of available GPU memory in kilobytes
    GLint availableGPUMemory();

    /// Returns approximate of total available GPU memory
    /// @return approximate of total GPU memory in kilobytes
    GLint maximumGPUMemory();

    /// Time at the beginning of this frame, typically same as
    /// MultiWidgets::FrameInfo::currentTime on the same frame
    Radiant::TimeStamp frameTime() const;

    /// Maximum texture size supported by this context, same as GL_MAX_TEXTURE_SIZE
    unsigned int maxTextureSize() const;

    /// The area in graphics coordinates that the audio panning should be limited to.
    /// Typically ViewWidgets might change this.
    /// @see MultiWidgets::ViewWidget::isAudioPanningEnabled
    const Nimble::Rect & audioPanningArea() const;
    void setAudioPanningArea(const Nimble::Rect & area);

  private:

/// @cond

    //////////////////////////////////////////////////////////////////////////
    /// Direct mode API
    //////////////////////////////////////////////////////////////////////////
    friend class D;
    friend class CustomOpenGL;
    void draw(PrimitiveType primType, unsigned int offset, unsigned int primitives);
    void drawIndexed(PrimitiveType primType, unsigned int offset, unsigned int primitives);

/// @endcond

    /// Finds shared buffer from buffer pool.
    /// @param elementSize Size of the single element to be stored
    /// @param elementCount Number of elements
    /// @param type Type of the buffer (Array/Index/Uniform)
    SharedBuffer * findAvailableBuffer(std::size_t elementSize, std::size_t elementCount,
                                       Buffer::Type type);

    /// Returns the memory mapping to GL buffer.
    /// @param buffer Buffer to map.
    /// @param type Type of the buffer
    /// @param offset Offset from the beginning of the buffer
    /// @param length Requested length of the mapped memory region.
    /// @param access Flags describing the access patterns of the mapped buffer.
    template <typename T>
    T * mapBuffer(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length,
                  Radiant::FlagsT<Buffer::MapAccess> access);

    /// Returns the memory mapping to GL buffer.
    /// @param buffer Buffer to map.
    /// @param type Type of the buffer
    /// @param access Flags describing the access patterns of the mapped buffer.
    template <typename T>
    inline T * mapBuffer(const Buffer & buffer, Buffer::Type type,
                         Radiant::FlagsT<Buffer::MapAccess> access);

    /// Unmaps the given buffer or range of it. If unmapping the whole buffer then this
    /// invalidates all the pointers returned by mapBuffer
    /// @param buffer Buffere to unmap
    /// @param type Type of the buffer
    /// @param offset Offset to the region unmapped
    /// @param length Length of the unmapped memory region
    void unmapBuffer(const Buffer & buffer, Buffer::Type type, int offset = 0, std::size_t length = std::size_t(-1));

  public:
    /// Clears the buffers of current frame buffer
    /// @param mask Mask to define what buffers to clear
    /// @param clearColor All values in color buffer will be initialized to this if it is cleared
    /// @param clearDepth All values in depth buffer will be initialized to this if it is cleared
    /// @param clearStencil All values in stencil buffer will be initialized to this if it is cleared
    void clear(ClearMask mask, const Radiant::ColorPMA & clearColor = Radiant::ColorPMA(0,0,0,0),
               double clearDepth = 1.0, int clearStencil = 0);

    /// Adds rendering command to the rendering queue. The difference between this and
    /// @ref render(bool, Luminous::PrimitiveType, int, int, float, const Luminous::Program&, const std::map< QByteArray, const Texture * > * , const std::map< QByteArray, ShaderUniform > *) "render"
    /// is that this version enables manual management of vertex data with the
    /// vertex array. In addition the used uniform block has free form. User needs to do
    /// his/hers own geometry transformations when using this function.
    /// @sa drawPrimitiveT
    /// @param translucent Does this rendering command involve transparency
    /// @param type Primitives to render
    /// @param offset Geometry and index offset in given vertex array
    /// @param vertexCount Number of vertices to draw
    /// @param primitiveSize Size of the point/line
    /// @param vertexArray Storage of the vertex data
    /// @param program Shader program to use in rendering
    /// @param textures Mapping from names to textures. Each texture is fed with the given name to shader
    /// @param uniforms Mapping from names to uniforms. Each uniform is fed with the given name to shader
    ///
    /// @tparam Vertex C++ class describing the vertex used in shader program. Doesn't require anything from this class
    /// @tparam UniformBlock C++ class corresponding the uniform block used by shader program. Doesn't require anything from this class
    /// @return Builder object for this drawing command. One has to fill the uniform block of the builder object.
    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> render(bool translucent,
                                               Luminous::PrimitiveType type,
                                               int offset, int vertexCount,
                                               float primitiveSize,
                                               const Luminous::VertexArray & vertexArray,
                                               const Luminous::Program & program,
                                               const std::map<QByteArray, const Texture *> * textures = nullptr,
                                               const std::map<QByteArray, ShaderUniform> * uniforms = nullptr);

    /// Similar to drawPrimitiveT but has less restrictions on the uniform block. It is enough for the
    /// uniform block to have fields @c projMatrix and @c vievMatrix (both Nimble::Matrix4).
    /// @sa drawPrimitiveT
    /// @param translucent Does this rendering command involve transparency
    /// @param type Primitives to render
    /// @param offset Geometry and index offset in given vertex array
    /// @param vertexCount Number of vertices to draw
    /// @param primitiveSize Size of the point/line
    /// @param vertexArray Storage of the vertex data
    /// @param program Shader program to use in rendering
    /// @param textures Mapping from names to textures. Each texture is fed with the given name to shader
    /// @param uniforms Mapping from names to uniforms. Each uniform is fed with the given name to shader
    ///
    /// @tparam Vertex C++ class describing the vertex used in shader program.
    /// @tparam UniformBlock C++ class corresponding the uniform block used by shader program. See the applying restrictions above.
    /// @return Builder object for this drawing command. User has to fill the uniform and vertex (and possibly index) buffers of the builder object.
    template <typename Vertex, typename UniformBlock>
    RenderBuilder<Vertex, UniformBlock> render( bool translucent,
                                                Luminous::PrimitiveType type, int indexCount, int vertexCount, float primitiveSize,
                                                const Luminous::Program & program,
                                                const std::map<QByteArray, const Texture *> * textures = nullptr,
                                                const std::map<QByteArray, ShaderUniform> * uniforms = nullptr);

    /// Returns the basic shader used in Luminous. This shader program transforms the geometry and
    /// paints it with single color. Needs BasicVertex for vertex type and BasicUniformBlock for uniform block
    /// type to be able to work.
    /// @return Basic shader used in Luminous
    const Program & basicShader() const;
    /// Returns the shader used for texturing in Luminous. This shader program does the same as the one
    /// returned by basicShader but also textures the geometry.
    /// Needs BasicVertexUV for vertex type and
    /// BasicUniformBlock for uniform block type to be able to work. The texture used is the one set
    /// as fill texture to Luminous::Style (or the one with name "tex"). Final color for the fragment
    /// is texture sample modulated with uniform color.
    /// @return Shader program used for texturing in Luminous.
    const Program & texShader() const;

    /// Returns the shader used for trilinear texturing in Luminous. Similar to
    /// texShader, but needs two textures "tex[0]" and "tex[1]" and works only
    /// with TrilinearFilteringUniformBlock. This is mostly used internally by
    /// ImageWidget and Widget background image renderer
    /// @return Shader program used for trilinear filtering
    const Program & trilinearTexShader() const;

    /// Get the approximate scaling factor applied by the transform.
    /// @return approximate scaling applied
    float approximateScaling() const;

/// @cond

    const Program & fontShader() const;
    const Program & splineShader() const;

/// @endcond

   private:
    int uniformBufferOffsetAlignment() const;

    std::size_t alignUniform(std::size_t uniformSize) const;

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
                      const Nimble::Matrix4f & modelview, bool ignoreVerticalAlign);
    template<typename T>
    void drawRectStroke(const Nimble::RectT<T>& rect, const Style & style);

    class Internal;
    Internal * m_data;
  };

  /** Guard for executing plain OpenGL commands.
    *
    * With the help of this object one can inject custom OpenGL commands
    * into Cornerstone application. The usage of this class is strongly
    * discouraged because it can cause big performance losses.
    * When using more exotic states, user should set the state of
    * OpenGL state machine to the state it was before.
    */
  class CustomOpenGL : Patterns::NotCopyable
  {
  public:
    /// Creates guard for the given OpenGL context. Executes all queued drawing
    /// commands in the given context.
    /// @param r Context to guard.
    /// @param reset Do we reset OpenGL state
    LUMINOUS_API CustomOpenGL(RenderContext & r, bool reset=false);
    /// Sets the given render context to the default state
    LUMINOUS_API ~CustomOpenGL();

    /// Returns the GL resources handle corresponding to given program.
    /// @param program CPU side object representing the shader program
    /// @return Handle to OpenGL resources of given program
    inline ProgramGL & handle(const Program & program) { return m_r.handle(program); }

    /// Returns the GL resources handle corresponding to given texture.
    /// @param texture CPU side object representing the texture
    /// @return Handle to OpenGL resources of given texture
    inline TextureGL & handle(const Texture & texture) { return m_r.handle(texture); }

    /// Returns the GL resources handle corresponding to given buffer (vertex/index).
    /// @param buffer CPU side object representing the buffer
    /// @return Handle to OpenGL resources of given buffer
    inline BufferGL & handle(const Buffer & buffer) { return m_r.handle(buffer); }

    /// Returns the GL resources handle corresponding to given vertex array.
    /// @param vertexArray CPU side object representing the vertex array
    /// @param program Handle to OpenGL program associated with vertexarray
    /// @return Handle to OpenGL resources of queried vertex array
    inline VertexArrayGL & handle(const VertexArray & vertexArray, ProgramGL * program) { return m_r.handle(vertexArray, program); }

    /// Returns the GL resources handle corresponding to given render buffer.
    /// @param buffer CPU side object representing the render buffer
    /// @return Handle to OpenGL resources of given render buffer
    inline RenderBufferGL & handle(const RenderBuffer & buffer) { return m_r.handle(buffer); }

    /// Returns the GL resources handle corresponding to given frame buffer.
    /// @param target CPU side object representing the frame buffer
    /// @return Handle to OpenGL resources of given frame buffer
    inline FrameBufferGL & handle(const FrameBuffer & target) { return m_r.handle(target); }

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

  /// This class provides a simple guard for setting opacity. It will
  /// automatically pop opacity in its destructor so the user doesn't need to
  /// remember to do it manually. It is equivalent to calling
  /// "RenderContext::pushOpacity(float)" and "RenderContext::popOpacity"
  class OpacityGuard : public Patterns::NotCopyable
  {
  public:
    /// Construct a new guard
    /// @param r render context
    OpacityGuard(RenderContext & r, float opacity) : m_rc(&r) { r.pushOpacity(opacity); }
    /// Construct a guard by moving
    /// @param o guard to move
    OpacityGuard(OpacityGuard && o) : m_rc(o.m_rc) { o.m_rc = nullptr; }
    /// Destructor. This function automatically calls RenderContext::popOpacity().
    ~OpacityGuard() { if(m_rc) m_rc->popOpacity(); }

  private:
    RenderContext * m_rc;
  };

  class ClipStackGuard : public Patterns::NotCopyable
  {
  public:
    /// Constructor. Automatically calls RenderContext::pushClipStack()
    /// @param r render context
    ClipStackGuard(RenderContext& r) : m_rc(&r) { r.pushClipStack(); }
    /// Move constructor
    ClipStackGuard(ClipStackGuard && rhs) : m_rc(rhs.m_rc) { rhs.m_rc = nullptr; }
    /// Destructor. This function automatically calls RenderContext::popClipStack()
    ~ClipStackGuard() { if(m_rc) m_rc->popClipStack(); }

  private:
    RenderContext * m_rc;
  };

  /// This class provides a simple guard for setting the active view transform. It will
  /// automatically pop the transform in its destructor so the user doesn't need to
  /// remember to do it manually. It is equivalent to calling
  /// "RenderContext::pushViewTransform(const Nimble::Matrix4 & m)" and "RenderContext::popViewTransform"
  class ViewTransformGuard : public Patterns::NotCopyable
  {
  public:
    /// Construct a new guard
    /// @param r render context
    ViewTransformGuard(RenderContext & r, const Nimble::Matrix4f & m) : m_rc(&r) { r.pushViewTransform(m); }
    /// Construct a guard by moving
    /// @param rhs guard to move
    ViewTransformGuard(ViewTransformGuard && rhs) : m_rc(rhs.m_rc) { rhs.m_rc = nullptr; }
    /// Destructor. This function automatically calls RenderContext::popViewTransform().
    ~ViewTransformGuard() { if (m_rc) m_rc->popViewTransform(); }
  private:
    RenderContext * m_rc;
  };

  /// This class provides a simple guard for setting the active viewport. It will
  /// automatically pop the viewport in its destructor so the user doesn't need to
  /// remember to do it manually. It is equivalent to calling
  /// "RenderContext::pushViewport(const Nimble::Recti &)" and "RenderContext::popViewport"
  class ViewportGuard : public Patterns::NotCopyable
  {
  public:
    /// Construct a new guard
    /// @param r render context
    ViewportGuard(RenderContext & r, const Nimble::Recti & viewport) : m_rc(&r) { r.pushViewport(viewport); }
    /// Construct a guard by moving
    /// @param rhs guard to move
    ViewportGuard(ViewportGuard && rhs) : m_rc(rhs.m_rc) { rhs.m_rc = nullptr; }
    /// Destructor. This function automatically calls RenderContext::popViewport().
    ~ViewportGuard() { if (m_rc) m_rc->popViewport(); }
  private:
    RenderContext * m_rc;
  };

  /// This class provides a simple guard for setting the active scissor area. It will
  /// automatically pop the area in its destructor so the user doesn't need to
  /// remember to do it manually. It is equivalent to calling
  /// "RenderContext::pushScissorRect(const Nimble::Recti &)" and "RenderContext::popScissorRect"
  class ScissorGuard : public Patterns::NotCopyable
  {
  public:
    /// Construct a new guard
    /// @param r render context
    ScissorGuard(RenderContext & r, const Nimble::Recti & scissor) : m_rc(&r) { r.pushScissorRect(scissor); }
    /// Construct a guard by moving
    /// @param rhs guard to move
    ScissorGuard(ScissorGuard && rhs) : m_rc(rhs.m_rc) { rhs.m_rc = nullptr; }
    /// Destructor. This function automatically calls RenderContext::popScissorRect().
    ~ScissorGuard() { if (m_rc) m_rc->popScissorRect(); }
  private:
    RenderContext * m_rc;
  };

  /// This class provides a simple guard for setting the active clipping area
  /// for widgets. It will automatically pop the area in its destructor so the
  /// user doesn't need to remember to do it manually. It is equivalent to
  /// calling "RenderContext::pushClipRect(const Nimble::Rectangle &)" and
  /// "RenderContext::popClipRect" This clipping only affects clipping
  /// individual widgets.
  class ClipGuard : public Patterns::NotCopyable
  {
  public:
    /// Construct a new guard
    /// @param r render context
    ClipGuard(RenderContext & r, const Nimble::Rectangle & rect) : m_rc(&r) { r.pushClipRect(rect); }
    /// Construct a guard by moving
    /// @param rhs guard to move
    ClipGuard(ClipGuard && rhs) : m_rc(rhs.m_rc) { rhs.m_rc = nullptr; }
    /// Destructor. This function automatically calls RenderContext::popClipRect().
    ~ClipGuard() { if (m_rc) m_rc->popClipRect(); }
  private:
    RenderContext * m_rc;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderContext;

  /// This class provides a simple guard for setting the active render target. It will
  /// automatically pop the target in its destructor so the user doesn't need to
  /// remember to do it manually. It is equivalent to calling
  /// "RenderContext::pushFrameBuffer(const FrameBuffer &)" and "RenderContext::popFrameBuffer"
  class FrameBufferGuard : public Patterns::NotCopyable
  {
  public:
    /// Construct a new guard
    /// @param r render context to pop a target from
    FrameBufferGuard(RenderContext & r, const FrameBuffer &target) : m_rc(&r) { r.pushFrameBuffer(target); }
    /// Construct a guard by moving
    /// @param rhs guard to move
    FrameBufferGuard(FrameBufferGuard && rhs) : m_rc(rhs.m_rc) { rhs.m_rc = nullptr; }
    /// Destructor. This function automatically calls RenderContext::popFrameBuffer().
    ~FrameBufferGuard() { if (m_rc) m_rc->popFrameBuffer(); }

  private:
    RenderContext * m_rc;
  };

  class BlockObjectsGuard : public Patterns::NotCopyable
  {
  public:
    BlockObjectsGuard(Luminous::RenderContext & r, RenderContext::ObjectMask mask)
      : m_rc(&r)
    {
      r.pushBlockObjects(mask);
    }

    ~BlockObjectsGuard() { if (m_rc) m_rc->popBlockObjects(); }

  private:
    Luminous::RenderContext * m_rc;
  };

  /// This class provides a simple guard for setting transformations. It will
  /// automatically pop transforms in its destructor so the user doesn't need to
  /// remember to do it manually. It is equivalent to calling
  /// "RenderContext::pushTransform(m)" and "RenderContext::popTransform()".
  class TransformGuard : public Patterns::NotCopyable
  {
  public:
    /// Construct a new guard. This calls RenderContext::pushTransform(m)
    TransformGuard(Luminous::RenderContext & r, const Nimble::Matrix4f & m) : m_rc(&r) { m_rc->pushTransform(m); }
    /// @copydoc TransformGuard
    TransformGuard(Luminous::RenderContext & r, const Nimble::Matrix3f & m) : m_rc(&r) { m_rc->pushTransform(m); }
    /// Construct a guard by moving
    /// @param rhs guard to move
    TransformGuard(TransformGuard && rhs) : m_rc(rhs.m_rc) { rhs.m_rc = nullptr; }
    ~TransformGuard() { if (m_rc) m_rc->popTransform(); }

    /// This class provides a simple guard for setting transformations. It will
    /// automatically pop transforms in its destructor so the user doesn't need to
    /// remember to do it manually. It is equivalent to calling
    /// "RenderContext::pushTransformLeftMul(m)" and "RenderContext::popTransform()".
    class LeftMul : public Patterns::NotCopyable
    {
    public:
      /// Construct a new guard. This calls RenderContext::pushTransformLeftMul(m)
      /// @param r render context
      /// @param m Transformation matrix
      LeftMul(Luminous::RenderContext & r, const Nimble::Matrix4f & m) : m_rc(&r) { m_rc->pushTransformLeftMul(m); }
      /// @copydoc LeftMul
      LeftMul(Luminous::RenderContext & r, const Nimble::Matrix3f & m) : m_rc(&r) { m_rc->pushTransformLeftMul(m); }
      /// Construct a guard by moving
      /// @param rhs guard to move
      LeftMul(LeftMul && rhs) : m_rc(rhs.m_rc) { rhs.m_rc = nullptr; }
      /// Destroys the guard. This calls RenderContext::popTransform()
      ~LeftMul() { if (m_rc) m_rc->popTransform(); }
    private:
      Luminous::RenderContext * m_rc;
    };

    /// This class provides a simple guard for setting transformations. It will
    /// automatically pop transforms in its destructor so the user doesn't need to
    /// remember to do it manually. It is equivalent to calling
    /// "RenderContext::pushTransformRightMul(m)" and "RenderContext::popTransform()".
    class RightMul : public Patterns::NotCopyable
    {
    public:
      /// Construct a new guard. This calls RenderContext::pushTransformRightMul(m)
      /// @param r render context
      /// @param m Transformation matrix
      RightMul(Luminous::RenderContext & r, const Nimble::Matrix4f & m) : m_rc(&r) { m_rc->pushTransformRightMul(m); }
      /// @copydoc RightMul
      RightMul(Luminous::RenderContext & r, const Nimble::Matrix3f & m) : m_rc(&r) { m_rc->pushTransformRightMul(m); }
      /// Construct a guard by moving
      /// @param rhs guard to move
      RightMul(RightMul && rhs) : m_rc(rhs.m_rc) { rhs.m_rc = nullptr; }
      /// Destroys the guard. This calls RenderContext::popTransform()
      ~RightMul() { if (m_rc) m_rc->popTransform(); }
    private:
      Luminous::RenderContext * m_rc;
    };

  private:
    Luminous::RenderContext * m_rc;
  };

  /// Simple guard for setting and restoring audio panning area
  /// @see RenderContext::audioPanningArea
  class AudioPanningGuard
  {
  public:
    AudioPanningGuard(Luminous::RenderContext & r, const Nimble::Rect & area)
      : m_rc(r)
      , m_oldArea(r.audioPanningArea())
    {
      r.setAudioPanningArea(area);
    }

    ~AudioPanningGuard()
    {
      m_rc.setAudioPanningArea(m_oldArea);
    }

  private:
    Luminous::RenderContext & m_rc;
    const Nimble::Rectf m_oldArea;
  };
}

#include <Luminous/RenderContextImpl.hpp>



#endif
