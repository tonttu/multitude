/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

// For PRIx64
#define __STDC_FORMAT_MACROS
#include <cinttypes>

#include "DxInterop.hpp"

#include "RenderContext.hpp"

#include "Error.hpp"

#include "Image.hpp"
#include "RenderCommand.hpp"

// Luminous v2
#include "Luminous/VertexArray.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/Buffer.hpp"
#include "Luminous/RenderDriverGL.hpp"
#include "Luminous/SimpleTextLayout.hpp"
#include "Luminous/ColorCorrectionFilter.hpp"
#include "Luminous/PostProcessChain.hpp"
#include "Luminous/PostProcessContext.hpp"
#include "Luminous/PostProcessFilter.hpp"

#include <Nimble/Matrix4.hpp>

#include <Radiant/Mutex.hpp>
#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Thread.hpp>

#include <tuple>
#include <vector>

#define DEFAULT_RECURSION_LIMIT 4
#define SHADER(str) #str

namespace Luminous
{
  class RenderContext::Internal
  {
  public:
    /// Optimal value for BUFFERSETS is actually dependent on glFinish
    /// in RenderThread, see comment there.
    enum { MAX_TEXTURES = 64, BUFFERSETS = 1 };
    Internal(RenderDriver & renderDriver, const Luminous::MultiHead::Window * win)
        : m_recursionLimit(DEFAULT_RECURSION_LIMIT)
        , m_renderCount(0)
        , m_unfinishedRenderCount(0)
        , m_frameNumber(0)
        , m_area(0)
        , m_window(win)
        , m_initialized(false)
        , m_uniformBufferOffsetAlignment(0)
        , m_automaticDepthDiff(-1.0f/100000.0f)
        , m_driver(renderDriver)
        , m_driverGL(static_cast<RenderDriverGL*>(&renderDriver))
        , m_bufferIndex(0)
        , m_useOffScreenFrameBuffer(false)
        , m_finalFrameBuffer(FrameBuffer::WINDOW)
        , m_postProcessFilters(0)
        , m_supports_GL_NVX_gpu_memory_info(false)
        , m_supports_GL_ATI_meminfo(false)
    {
      // Reset render call count
      m_renderCalls.push(0);

      // Initialize default frame buffer size
      assert(win);
      m_finalFrameBuffer.setSize(win->size());

      if (win->directRendering())
        resizeOffScreenFrameBuffers(1);

      m_basicShader.loadShader("cornerstone:Luminous/GLSL150/basic.vs", Shader::Vertex);
      m_basicShader.loadShader("cornerstone:Luminous/GLSL150/basic.fs", Shader::Fragment);
      Luminous::VertexDescription desc;
      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      m_basicShader.setVertexDescription(desc);

      m_texShader.loadShader("cornerstone:Luminous/GLSL150/tex.vs", Shader::Vertex);
      m_texShader.loadShader("cornerstone:Luminous/GLSL150/tex.fs", Shader::Fragment);
      desc = Luminous::VertexDescription();
      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      desc.addAttribute<Nimble::Vector2>("vertex_uv");
      m_texShader.setVertexDescription(desc);
      m_texShader.setSampleShading(1.f);

      m_trilinearTexShader.loadShader("cornerstone:Luminous/GLSL150/trilinear_filtering.vs", Shader::Vertex);
      m_trilinearTexShader.loadShader("cornerstone:Luminous/GLSL150/trilinear_filtering.fs", Shader::Fragment);
      desc = Luminous::VertexDescription();
      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      desc.addAttribute<Nimble::Vector2>("vertex_uv");
      m_trilinearTexShader.setSampleShading(1.f);
      m_trilinearTexShader.setVertexDescription(desc);

      m_fontShader.loadShader("cornerstone:Luminous/GLSL150/distance_field.vs", Shader::Vertex);
      m_fontShader.loadShader("cornerstone:Luminous/GLSL150/distance_field.fs", Shader::Fragment);
      desc = Luminous::VertexDescription();
      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      desc.addAttribute<Nimble::Vector2>("vertex_uv");
      desc.addAttribute<float>("vertex_invsize");
      m_fontShader.setVertexDescription(desc);
      m_fontShader.setSampleShading(1.0f);

      m_splineShader.loadShader("cornerstone:Luminous/GLSL150/spline.fs", Luminous::Shader::Fragment);
      /// There are two versions of spline.vs: spline.vs has time range and spline2.vs doesn't.
      /// Luminous::Spline uses the old spline.vs, so need to keep it for now.
      /// New Spline uses spline2.vs.
      /// @todo: remove old spline.vs and rename spline2.vs when Luminous::Spline is removed
      m_splineShader.loadShader("cornerstone:Luminous/GLSL150/spline2.vs", Luminous::Shader::Vertex);
      desc = Luminous::VertexDescription();
      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      desc.addAttribute<Nimble::Vector4f>("vertex_color");
      m_splineShader.setVertexDescription(desc);

      // Fetch GPU upload limits from the window configuration
      uint64_t limit = win->uploadLimit();
      uint64_t margin = win->uploadMargin();
      renderDriver.setUploadLimits( limit, margin );
    }

    ~Internal()
    {
    }

    void resizeOffScreenFrameBuffers(size_t newSize)
    {
      std::vector<FrameBuffer> newVector(newSize);
      for (size_t frame = m_frameNumber; frame > m_frameNumber - m_offScreenFrameBuffers.size(); --frame) {
        newVector[frame % newVector.size()] = std::move(m_offScreenFrameBuffers[frame %
            m_offScreenFrameBuffers.size()]);
        if (frame == 0) break;
      }
      std::swap(newVector, m_offScreenFrameBuffers);

      // Set initial data for an off-screen frame buffer
      // The hardware resource is not created if this is actually never bound
      for (auto & fb: m_offScreenFrameBuffers) {
        if (fb.size() != m_window->size()) {
          fb.setSize(m_window->size());
          fb.setSamples(std::max(0, m_window->antiAliasingSamples()));

          fb.createRenderBufferAttachment(GL_COLOR_ATTACHMENT0, GL_RGBA);
          fb.createRenderBufferAttachment(GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8);
        }
      }
    }

    void initialize() {

      assert(m_window != 0);

      if(!m_initialized) {
        m_initialized = true;

        m_uniformBufferOffsetAlignment = m_driver.uniformBufferOffsetAlignment();
      }
    }

    Nimble::Size contextSize() const
    {
      if(m_window)
        return m_window->size();

      /// @todo why not zero vector?
      return Nimble::Size(10, 10);
    }

    void createPostProcessFilters(RenderContext & rc, const Luminous::PostProcessFilters & filters)
    {
      for(Luminous::PostProcessFilters::const_iterator it = filters.begin(); it != filters.end(); ++it) {

        if(m_postProcessChain.contains(*it))
          continue;

        // Create a new context for the filter
        auto context = std::make_shared<PostProcessContext>(*it);

        if(context) {
          // By default resizes new frame buffers to current context size
          context->initialize(rc, rc.contextSize());
          m_postProcessChain.insert(context);
        }
      }
    }

    FrameBuffer & defaultFrameBuffer()
    {
      return m_window->directRendering() ?
            m_finalFrameBuffer :
            m_offScreenFrameBuffers[m_frameNumber % m_offScreenFrameBuffers.size()];
    }

    size_t m_recursionLimit;
    std::stack<size_t, std::vector<size_t> > m_recursionDepthStack;

    std::stack<Nimble::ClipStack> m_clipStacks;
 
    std::vector<Valuable::Node::Uuid> m_viewWidgetPath;
    QByteArray m_viewWidgetPathId;

    unsigned long m_renderCount;
    unsigned long m_unfinishedRenderCount;
    unsigned int m_frameNumber;

    const Luminous::MultiHead::Area * m_area;
    const Luminous::MultiHead::Window * m_window;

    Transformer m_viewTransformer;

    bool m_initialized;

    // Viewports defined as x1,y1,x2,y2
    typedef std::stack<Nimble::Recti, std::vector<Nimble::Recti> > ViewportStack;
    ViewportStack m_viewportStack;

    typedef std::stack<RenderContext::ObjectMask, std::vector<RenderContext::ObjectMask> > BlockObjectsStack;
    BlockObjectsStack m_blockObjectsStack;

    // Scissor rectangles
    typedef std::stack<Nimble::Recti, std::vector<Nimble::Recti> > ScissorStack;
    ScissorStack m_scissorStack;

    // Cache for vertex array objects used in sharedbuffer rendering
    // key is <vertex buffer id, shader>
    
    struct VertexArrayKey
    {
      VertexArrayKey(RenderResource::Id id1 = 0,
		     RenderResource::Id id2 = 0,
		     const ProgramGL* program = 0)
	: m_id1(id1), m_id2(id2), m_program(program) {}

      inline bool operator == (const VertexArrayKey & that) const
      {
        return m_id1 == that.m_id1 && m_id2 == that.m_id2 && m_program == that.m_program;
      }

      inline bool operator < (const VertexArrayKey & that) const
      {
        /// Make sure we keep a strict ordering so we don't get (A < B && B < A) == true
        if (m_id1 != that.m_id1)
          return m_id1 < that.m_id1;

        if (m_id2 != that.m_id2)
          return m_id2 < that.m_id2;

        return m_program < that.m_program;
      }

      const RenderResource::Id m_id1;
      const RenderResource::Id m_id2;
      const ProgramGL* m_program;
    };

    
    typedef std::map<VertexArrayKey, VertexArray> VertexArrayCache;
    VertexArrayCache m_vertexArrayCache;
    
    // List of currently active textures, vbos etc.

    int m_uniformBufferOffsetAlignment;

    float m_automaticDepthDiff;
    // Stack of render call counts
    std::stack<int> m_renderCalls;

    Program m_basicShader;
    Program m_texShader;
    Program m_trilinearTexShader;
    Program m_fontShader;
    Program m_splineShader;

    std::array<Image, 128+1> m_dashedLineImages;

    Luminous::RenderDriver & m_driver;
    Luminous::RenderDriverGL * m_driverGL;
    Luminous::StencilMode m_stencilMode;

    struct BufferPool
    {
      BufferPool() {}
      std::vector<SharedBuffer> buffers;

      BufferPool(BufferPool && pool)
        : buffers(std::move(pool.buffers))
      {}

      BufferPool & operator=(BufferPool && pool)
      {
        buffers = std::move(pool.buffers);
        return *this;
      }

      void flush(RenderContext & ctx)
      {
        for(auto it = buffers.begin(); it != buffers.end(); ++it) {
          if (it->reservedBytes > 0) {
            // @todo Investigate if orphaning is any faster on multi-screen/multi-GPU setups
            // it->buffer.setData(nullptr, it->buffer.size(), Buffer::STREAM_DRAW);
#ifdef RENDERCONTEXT_SHAREDBUFFER_MAP
            ctx.unmapBuffer(it->buffer, it->type, 0, it->reservedBytes);
#else
            BufferGL & b = ctx.handle(it->buffer);
            b.upload(it->type, 0, it->reservedBytes, it->data.data());
#endif
            it->reservedBytes = 0;
          }
        }
      }
    };

    // vertex/uniform struct size -> pool
    std::map<std::size_t, BufferPool> m_vertexBuffers[BUFFERSETS];
    std::map<std::size_t, BufferPool> m_uniformBuffers[BUFFERSETS];
    BufferPool m_indexBuffers[BUFFERSETS];
    int m_bufferIndex;

    // Updated before each frame
    bool m_useOffScreenFrameBuffer;

    // Default window framebuffer. Eventually the result of the rendering pipeline end ups here
    FrameBuffer m_finalFrameBuffer;
    // Used when rendering non-directly/with postprocessing
    std::vector<FrameBuffer> m_offScreenFrameBuffers;

    // owned by Application
    const Luminous::PostProcessFilters * m_postProcessFilters;
    PostProcessChain m_postProcessChain;

    std::stack<float, std::vector<float> > m_opacityStack;

    std::stack<const Luminous::FrameBuffer*, std::vector<const Luminous::FrameBuffer*> > m_frameBufferStack;

    bool m_supports_GL_NVX_gpu_memory_info;
    bool m_supports_GL_ATI_meminfo;

    Radiant::TimeStamp m_frameTime;
    unsigned int m_maxTextureSize = 1024;

    Nimble::Rect m_audioPanningArea{std::numeric_limits<float>::lowest(),
                                    std::numeric_limits<float>::lowest(),
                                    std::numeric_limits<float>::max(),
                                    std::numeric_limits<float>::max()};

    DxInterop m_dxInteropApi;
  };

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  RenderContext::RenderContext(Luminous::RenderDriver & driver, const Luminous::MultiHead::Window * win)
      : Transformer(),
      m_data(new Internal(driver, win))
  {
    // This requires current OpenGL context
    initializeOpenGLFunctions();

    resetTransform();
  }

  RenderContext::~RenderContext()
  {
    debugLuminous("Closing OpenGL context. Rendered %lu things in %u frames, %lu things per frame",
         m_data->m_renderCount, m_data->m_frameNumber,
         m_data->m_renderCount / std::max(m_data->m_frameNumber, 1u));
    delete m_data;
  }

  RenderDriver &RenderContext::renderDriver()
  {
    return m_data->m_driver;
  }

  void RenderContext::setWindowArea(const MultiHead::Window *window, const Luminous::MultiHead::Area * area)
  {
    m_data->m_window = window;
    m_data->m_area = area;
  }

  const Luminous::MultiHead::Window * RenderContext::window() const
  {
    return m_data->m_window;
  }

  const Luminous::MultiHead::Area * RenderContext::area() const
  {
    return m_data->m_area;
  }

  void RenderContext::pushViewTransform(const Nimble::Matrix4 & m)
  {
    m_data->m_viewTransformer.pushTransform();
    m_data->m_viewTransformer.setTransform(m);
  }

  void RenderContext::popViewTransform()
  {
    m_data->m_viewTransformer.popTransform();
  }

  const Nimble::Matrix4 & RenderContext::viewTransform() const
  {
    return m_data->m_viewTransformer.transform();
  }

  const FrameBuffer & RenderContext::currentFrameBuffer() const
  {
    assert(!m_data->m_frameBufferStack.empty());
    return *m_data->m_frameBufferStack.top();
  }

  void RenderContext::setRecursionLimit(size_t limit)
  {
    m_data->m_recursionLimit = limit;
  }

  size_t RenderContext::recursionLimit() const
  {
    return m_data->m_recursionLimit;
  }

  void RenderContext::pushClipMaskStack(size_t depth)
  {
    m_data->m_recursionDepthStack.push(depth);
  }

  size_t RenderContext::currentClipMaskDepth() const
  {
    assert(!m_data->m_recursionDepthStack.empty());
    return m_data->m_recursionDepthStack.top();
  }

  void RenderContext::popClipMaskStack()
  {
    m_data->m_recursionDepthStack.pop();
  }

  /// Save the current clipping stack and start with a empty one
  void RenderContext::pushClipStack()
  {
    m_data->m_clipStacks.push(Nimble::ClipStack());
  }

  /// Restores the previously saved clipping stack
  void RenderContext::popClipStack()
  {
    assert(!m_data->m_clipStacks.empty());
    m_data->m_clipStacks.pop();
  }

  void RenderContext::pushClipRect(const Nimble::Rectangle & r)
  {
//      Radiant::info("RenderContext::pushClipRect # (%f,%f) (%f,%f)", r.center().x, r.center().y, r.size().x, r.size().y);
    assert(!m_data->m_clipStacks.empty());
    m_data->m_clipStacks.top().push(r);
  }

  void RenderContext::popClipRect()
  {
    assert(!m_data->m_clipStacks.empty());
    m_data->m_clipStacks.top().pop();
  }

  const Nimble::ClipStack & RenderContext::clipStack() const
  {
    assert(!m_data->m_clipStacks.empty());
    return m_data->m_clipStacks.top();
  }

  bool RenderContext::isClipStackEmpty() const
  {
    return m_data->m_clipStacks.empty();
  }

  bool RenderContext::isVisible(const Nimble::Rectangle & area)
  {
    if(m_data->m_clipStacks.empty())
      return true;

    return m_data->m_clipStacks.top().isVisible(area);
  }

  void RenderContext::pushViewWidget(Valuable::Node::Uuid id)
  {
    m_data->m_viewWidgetPath.push_back(id);

    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%016" PRIx64, id);
    m_data->m_viewWidgetPathId.append(buffer, 16);
  }

  void RenderContext::popViewWidget()
  {
    m_data->m_viewWidgetPath.pop_back();
    m_data->m_viewWidgetPathId.chop(16);
  }

  const std::vector<Valuable::Node::Uuid> & RenderContext::viewWidgetPath() const
  {
    return m_data->m_viewWidgetPath;
  }

  const QByteArray & RenderContext::viewWidgetPathId() const
  {
    return m_data->m_viewWidgetPathId;
  }

  void RenderContext::drawArc(const Nimble::Vector2f & center, float radius,
                              float fromRadians, float toRadians, const Luminous::Style & style, unsigned int linesegments)
  {
    if (linesegments == 0) {
      /// @todo Automagically determine the proper number of linesegments
      linesegments = 32;
    }

    /// The maximum supported linewidth is often quite low so we'll generate a triangle strip instead
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, (linesegments + 1) * 2,
      program, style.strokeColor(), 1.f, style);

    float step = (toRadians - fromRadians) / linesegments;

    auto v = b.vertex;
    float angle = fromRadians;
    for (unsigned int i = 0; i <= linesegments; ++i) {
      Nimble::Vector2f c(std::cos(angle), std::sin(angle));
      (v++)->location = center + c * (radius - style.strokeWidth());
      (v++)->location = center + c * (radius + style.strokeWidth());

      angle += step;
    }
  }

  void RenderContext::drawCircle(const Nimble::Vector2f & center, float radius, const Luminous::Style & style, unsigned int linesegments, float fromRadians, float toRadians)
  {
    if (linesegments == 0) {

      // Minimize geometric error when approximating a circle with a polygon
      const float scale = this->approximateScaling();

      const float tolerance = 0.25f;
      const float actualRadius = scale * (radius + 0.5f * style.strokeWidth());

      linesegments = std::max<int>(16, Nimble::Math::PI / acos(1.f - (tolerance / actualRadius)));
    }

    // Filler function: Generates vertices in a circle
    auto fill = [=](BasicVertex * vertices) {
      float step = (toRadians - fromRadians) / linesegments;

      // Add the rest of the fan vertices
      float angle = fromRadians;
      for (unsigned int i = 0; i <= linesegments; ++i) {
        Nimble::Vector2f c(std::cos(angle), std::sin(angle));
        vertices[i].location = center + c * radius;
        angle += step;
      }
    };

    // Draw fill
    if (style.hasFill()) {
      const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
      auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_FAN, 0, linesegments + 2, program, style.fillColor(), 1.f, style);
      // Center is the first vertex in a fan
      b.vertex[0].location = center;
      // Create the rest of the vertices
      fill(&b.vertex[1]);
    }

    // Draw stroke
    if (style.hasStroke()) {
      Luminous::Style s = style;
      s.stroke().clear();
      s.setFillColor(style.strokeColor());
      if(style.strokeProgram())
        s.setFillProgram(*style.strokeProgram());
      else
        s.setDefaultFillProgram();

      drawDonut(center,
                Nimble::Vector2(radius, 0),
                radius,
                style.strokeWidth(),
                s,
                linesegments,
                fromRadians,
                toRadians);
    }
  }

  void RenderContext::drawDonut(const Nimble::Vector2f & center,
                                Nimble::Vector2 axis,
                                float otherAxisLength,
                                float width,
                                const Luminous::Style & style,
                                unsigned int linesegments,
                                float fromRadians, float toRadians,
                                TextureMappingMode textureMappingMode)
  {
    if(linesegments == 0) {
      /// @todo automagically determine divisions?
      // This is better than nothing, but the model view transformation matrix is being ignored.
      linesegments = std::max((int) (axis.maximum() * 0.4f), 16);
    }

    float rotation = axis.angle();

    // Ellipse parameters
    float a = axis.length();
    float b = otherAxisLength;

    Luminous::TransformGuard::RightMul transformGuard(*this, Nimble::Matrix3::makeTranslation(center) * Nimble::Matrix3::makeRotation(rotation));

    bool isFilled = style.hasFill();
    bool stroke = style.hasStroke();

    bool needInnerStroke = std::min(a, b) - width/2.0f > 0.0f;

    const float step = (toRadians - fromRadians) / (linesegments-1);

    float angle = fromRadians;

    float r = 0.5f * width;

    float maxLength = std::max(a, b);
    float iSpan = 1.0f/(2.0f*r);
    Nimble::Vector2f low = Nimble::Vector2(maxLength, maxLength);

    RenderBuilder<BasicVertex, BasicUniformBlock> fill;
    RenderBuilder<BasicVertexUV, BasicUniformBlock> textured;

    RenderBuilder<BasicVertex, BasicUniformBlock> innerStroke;
    RenderBuilder<BasicVertex, BasicUniformBlock> outerStroke;

    /// Generate the fill builders
    bool isTextured = false;
    if(isFilled) {
      if (!style.fill().hasTextures()) {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        fill = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, linesegments * 2, program, style.fillColor(), 1.f, style);
      }
      else {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        textured = drawPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, linesegments * 2, program, style.fillColor(), 1.f, style);
        isTextured = true;
      }
    }

    /// Generate the stroke builders
    if(stroke) {
      const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
      if(needInnerStroke)
        innerStroke = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, linesegments * 2, program, style.strokeColor(), 1.0f, style);
      outerStroke = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, linesegments * 2, program, style.strokeColor(), 1.0f, style);
    }

    /// Generate the vertex data
    for (unsigned int i = 0; i < linesegments; ++i) {
      // Expand path of ellipse e(t) = (a cos(t), b sin(t)) along normals

      Nimble::Vector2f c = Nimble::Vector2(std::cos(angle), std::sin(angle));
      Nimble::Vector2f normal = Nimble::Vector2(-b*c.x, -a*c.y).normalized(r);

      Nimble::Vector2 e(a*c.x, b*c.y);


      Nimble::Vector2f in = e + normal;
      Nimble::Vector2f out = e - normal;

      if(isTextured) {
        textured.vertex[2*i].location = in;
        textured.vertex[2*i+1].location = out;
        if(textureMappingMode == TEXTURE_MAPPING_TANGENT) {
          textured.vertex[2*i].texCoord.make(angle/Nimble::Math::TWO_PI, 0.f);
          textured.vertex[2*i+1].texCoord.make(angle/Nimble::Math::TWO_PI, 1.f);
        } else {
          textured.vertex[2*i].texCoord = iSpan * (in-low);
          textured.vertex[2*i+1].texCoord = iSpan * (out-low);
        }
      }
      else if(isFilled) {
        fill.vertex[2*i].location = in;
        fill.vertex[2*i+1].location = out;
      }
      if(stroke) {
        // For the stroke, we need to find normals for along the inner & outer edge:
        //  s(t) = e(t) + g(t), g(t) = r * normal(e(t)) / ||normal(e(t)||

        Nimble::Vector2 e_(-a*c.y, b*c.x);

        // Calculate dg/dt
        Nimble::Vector2 s_(a*a*b*c.y, -a*b*b*c.x);
        s_ *= -r*std::pow(e_.x*e_.x + e_.y*e_.y, -3/2.0f);

        // Add de/dt
        s_ += Nimble::Vector2(-a*c.y, b*c.x);

        Nimble::Vector2 offset = s_.perpendicular().normalized(0.5f * style.strokeWidth());

        if(needInnerStroke) {
          innerStroke.vertex[2*i].location = in + offset;
          innerStroke.vertex[2*i+1].location = in - offset;
        }

        outerStroke.vertex[2*i].location = out + offset; 
        outerStroke.vertex[2*i+1].location = out - offset;
      }

      angle += step;
    }
  }

  void RenderContext::addRenderCounter()
  {
    m_data->m_renderCount++;
  }

  unsigned long RenderContext::renderCounter() const
  {
    return m_data->m_renderCount;
  }

  void RenderContext::addUnfinishedRenderCounter()
  {
    ++m_data->m_unfinishedRenderCount;
  }

  unsigned long RenderContext::unfinishedRenderCounter() const
  {
    return m_data->m_unfinishedRenderCount;
  }

  Nimble::Vector4 proj(const Nimble::Matrix4 & m4, const Nimble::Matrix3 & m3,
                       Nimble::Vector2 v)
  {
    Nimble::Vector3 v3(v.x, v.y, 1);
    v3 = m3 * v3;
    Nimble::Vector4 v4(v3.x, v3.y, 0, v3.z);
    return m4 * v4;
  }

  RenderContext::SharedBuffer* RenderContext::findAvailableBuffer(
      std::size_t elementSize, std::size_t elementCount, Buffer::Type type)
  {
    int bufferIndex = m_data->m_bufferIndex;
    Internal::BufferPool & pool = type == Buffer::INDEX
        ? m_data->m_indexBuffers[bufferIndex]
        : type == Buffer::VERTEX
          ? m_data->m_vertexBuffers[bufferIndex][elementSize]
          : m_data->m_uniformBuffers[bufferIndex][elementSize];

    const std::size_t requiredBytes = elementSize * elementCount;

    SharedBuffer * buffer = nullptr;
    std::size_t nextSize = 1 << 20;
    for(int i = 0; ;++i) {
      if(i >= int(pool.buffers.size())) {
        pool.buffers.emplace_back(type);
        buffer = &pool.buffers.back();
        buffer->buffer.setData(nullptr, std::max(requiredBytes, nextSize), Buffer::STREAM_DRAW);
        /// Fix the generation so it doesn't get automatically overwritten by an upload()
        buffer->buffer.setGeneration(0);
#ifndef RENDERCONTEXT_SHAREDBUFFER_MAP
        buffer->data.resize(buffer->buffer.dataSize());
#endif
        break;
      }

      buffer = &pool.buffers[i];
      if(buffer->buffer.dataSize() - buffer->reservedBytes >= requiredBytes)
        break;

      nextSize <<= 1;
    }
    return buffer;
  }

  std::pair<void *, RenderContext::SharedBuffer *> RenderContext::sharedBuffer(
      std::size_t vertexSize, std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset)
  {
    SharedBuffer * buffer = findAvailableBuffer(vertexSize, maxVertexCount, type);

#ifdef RENDERCONTEXT_SHAREDBUFFER_MAP
    char * data = mapBuffer<char>(buffer->buffer, type, Buffer::MAP_WRITE | Buffer::MAP_UNSYNCHRONIZED |
                                  Buffer::MAP_INVALIDATE_BUFFER | Buffer::MAP_FLUSH_EXPLICIT);
#else
    char * data = buffer->data.data();
#endif
    assert(data);
    data += buffer->reservedBytes;
    offset = static_cast<unsigned int>(buffer->reservedBytes / vertexSize);
    buffer->reservedBytes += vertexSize * maxVertexCount;
    return std::make_pair(data, buffer);
  }

  template <> 
  void * RenderContext::mapBuffer<void>(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length,
                                        Radiant::FlagsT<Buffer::MapAccess> access)
  {
    return m_data->m_driver.mapBuffer(buffer, type, offset, length, access);
  }

  void RenderContext::unmapBuffer(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length)
  {
    m_data->m_driver.unmapBuffer(buffer, type, offset, length);
  }

  // Create a render command using the shared buffers
  RenderCommand & RenderContext::createRenderCommand(bool translucent,
                                                     const Luminous::VertexArray & vertexArray,
                                                     const Luminous::Buffer & uniformBuffer,
                                                     float & depth,
                                                     const Program & shader,
                                                     const std::map<QByteArray,const Texture *> * textures,
                                                     const std::map<QByteArray, ShaderUniform> * uniforms)
  {
    RenderCommand & cmd = m_data->m_driver.createRenderCommand(translucent, vertexArray, uniformBuffer, shader, textures, uniforms);

    depth = 0.99999f + m_data->m_automaticDepthDiff * m_data->m_renderCalls.top();
    ++(m_data->m_renderCalls.top());

    return cmd;
  }

  MultiDrawCommand & RenderContext::createMultiDrawCommand(
      bool translucent,
      int drawCount,
      const VertexArray & vertexArray,
      const Buffer & uniformBuffer,
      float & depth,
      const Program & shader,
      const std::map<QByteArray, const Texture *> * textures,
      const std::map<QByteArray, ShaderUniform> * uniforms)
  {
    MultiDrawCommand & cmd = m_data->m_driver.createMultiDrawCommand(
          translucent, drawCount, vertexArray, uniformBuffer, shader, textures, uniforms);

    depth = 0.99999f + m_data->m_automaticDepthDiff * m_data->m_renderCalls.top();
    ++(m_data->m_renderCalls.top());

    return cmd;
  }

  // Create a render command using the shared buffers
  std::size_t RenderContext::alignUniform(std::size_t uniformSize) const
  {
    return std::ceil(uniformSize / float(uniformBufferOffsetAlignment())) * uniformBufferOffsetAlignment();
  }

  RenderCommand & RenderContext::createRenderCommand(bool translucent,
                                                     int indexCount, int vertexCount,
                                                     std::size_t vertexSize, std::size_t uniformSize,
                                                     unsigned int *& mappedIndexBuffer,
                                                     void *& mappedVertexBuffer,
                                                     void *& mappedUniformBuffer,
                                                     float & depth,
                                                     const Program & shader,
                                                     const std::map<QByteArray,const Texture *> * textures,
                                                     const std::map<QByteArray, ShaderUniform> * uniforms)
  {
    unsigned int indexOffset = 0, vertexOffset, uniformOffset;

    // Align uniforms as required by OpenGL
    uniformSize = alignUniform(uniformSize);

    SharedBuffer * vbuffer;
    std::tie(mappedVertexBuffer, vbuffer) = sharedBuffer(vertexSize, vertexCount, Buffer::VERTEX, vertexOffset);

    SharedBuffer * ubuffer;
    std::tie(mappedUniformBuffer, ubuffer) = sharedBuffer(uniformSize, 1, Buffer::UNIFORM, uniformOffset);

    SharedBuffer * ibuffer;
    RenderResource::Id ibufferId = 0;
    if (indexCount > 0) {
      // Index buffers are implicitly tied to VAO when bound so we make mapping after we
      // are sure that correct VAO is bound
      ibuffer = findAvailableBuffer(sizeof(unsigned int), indexCount, Buffer::INDEX);
      // Get the matching vertexarray from cache or create a new one if needed
      ibufferId = ibuffer->buffer.resourceId();
    }

    Internal::VertexArrayKey key(vbuffer->buffer.resourceId(), ibufferId, &handle(shader));
    
    Internal::VertexArrayCache::const_iterator it = m_data->m_vertexArrayCache.find(key);

    if(it == m_data->m_vertexArrayCache.end()) {
      // No array yet for this combination: Create a new vertexarray
      VertexArray vertexArray;
      vertexArray.addBinding(vbuffer->buffer, shader.vertexDescription());
      if (indexCount > 0)
        vertexArray.setIndexBuffer(ibuffer->buffer);
      
      it = m_data->m_vertexArrayCache.insert(std::make_pair(key, std::move(vertexArray))).first;
      // m_data->m_vertexArrayCache[key] = std::move(vertexArray);
      // it = m_data->m_vertexArrayCache.find(key);
    }

    RenderCommand & cmd = m_data->m_driver.createRenderCommand(
          translucent, it->second, ubuffer->buffer, shader, textures, uniforms);
    if(indexCount > 0) {
      // Now we are ready to bind index buffer (driver made sure that VAO is bound)
#ifdef RENDERCONTEXT_SHAREDBUFFER_MAP
      char * data = mapBuffer<char>(ibuffer->buffer, Buffer::INDEX, Buffer::MAP_WRITE | Buffer::MAP_UNSYNCHRONIZED |
                                    Buffer::MAP_INVALIDATE_BUFFER | Buffer::MAP_FLUSH_EXPLICIT);
#else
      char * data = ibuffer->data.data();
#endif
      mappedIndexBuffer = reinterpret_cast<unsigned int*>(data + ibuffer->reservedBytes);
      indexOffset = static_cast<unsigned int>(ibuffer->reservedBytes) / sizeof(unsigned int);
      ibuffer->reservedBytes += sizeof(unsigned int)*indexCount;
    }

    cmd.primitiveCount = ( indexCount > 0 ? indexCount : vertexCount );
    cmd.indexed = (indexCount > 0);
    cmd.indexOffset = indexOffset;
    cmd.vertexOffset = vertexOffset;
    cmd.uniformOffsetBytes = uniformOffset * static_cast<unsigned int>(uniformSize);
    cmd.uniformSizeBytes = static_cast<unsigned int>(uniformSize);

    depth = 0.99999f + m_data->m_automaticDepthDiff * m_data->m_renderCalls.top();
    ++(m_data->m_renderCalls.top());

    return cmd;
  }

  /// Drawing utility commands
  //////////////////////////////////////////////////////////////////////////

  void RenderContext::drawRect(const Nimble::Vector2f & min, const Nimble::Vector2f & max, const Style & style)
  {
    drawRect(Nimble::Rect(min, max), style);
  }

  void RenderContext::drawRect(const Nimble::Vector2f & min, const Nimble::SizeF & size, const Style & style)
  {
    drawRect(Nimble::Rect(min, size), style);
  }

  void RenderContext::drawQuad(const Nimble::Vector2 *vertices, const Nimble::Vector2 *uvs, const Style &style)
  {
    assert(vertices);
    if (style.hasFill()) {
      if(!style.fill().hasTextures()) {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 4, program, style.fillColor(), 1.f, style);
        b.vertex[0].location = vertices[0];
        b.vertex[1].location = vertices[1];
        b.vertex[2].location = vertices[2];
        b.vertex[3].location = vertices[3];
      }
      else {
        assert(uvs);
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        auto b = drawPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 4, program, style.fillColor(), 1.f, style);

        b.vertex[0].location = vertices[0];
        b.vertex[0].texCoord = uvs[0];

        b.vertex[1].location = vertices[1];
        b.vertex[1].texCoord = uvs[1];

        b.vertex[2].location = vertices[2];
        b.vertex[2].texCoord = uvs[2];

        b.vertex[3].location = vertices[3];
        b.vertex[3].texCoord = uvs[3];
      }
    }

    // Draw the outline
    if (style.hasStroke()) {
      Luminous::Style s;
      s.setFillColor(style.strokeColor());
      if(style.strokeProgram())
        s.setFillProgram(*style.strokeProgram());
      else
        s.setDefaultFillProgram();

      int order[] = {0, 1, 3, 2, 0};
      std::vector<Nimble::Vector2> outline;
      outline.resize(4);
      float w = style.strokeWidth();
      for(size_t i = 0; i < sizeof(order)/sizeof(int) - 1; ++i) {
        const Nimble::Vector2& p0 = vertices[order[i]];
        const Nimble::Vector2& p1 = vertices[order[i+1]];
        const Nimble::Vector2 nDiff = (p1 - p0).normalized();
        const Nimble::Vector2 perp(-nDiff.y, nDiff.x);

        outline[0] = p0 - w*(nDiff + perp);
        outline[1] = p0 - w*(nDiff - perp);
        outline[2] = p1 + w*(nDiff - perp);
        outline[3] = p1 + w*(nDiff + perp);
        drawQuad(outline.data(), nullptr, s);
      }
    }
  }

  void RenderContext::drawRectWithHole(const Nimble::Rectf & area,
                                       const Nimble::Rectf & hole,
                                       const Luminous::Style & style)
  {
    if (style.hasFill()) {
      if (!style.fill().hasTextures()) {
        // Untextured
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 10, program, style.fillColor(), 1.f, style);
        b.vertex[0].location = hole.low();
        b.vertex[1].location = area.low();
        b.vertex[2].location = hole.highLow();
        b.vertex[3].location = area.highLow();
        b.vertex[4].location = hole.high();
        b.vertex[5].location = area.high();
        b.vertex[6].location = hole.lowHigh();
        b.vertex[7].location = area.lowHigh();
        b.vertex[8].location = hole.low();
        b.vertex[9].location = area.low();
      }
      else {
        // Textured
        /// @todo calculate correct UVs for the inside ring
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        auto b = drawPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 10, program, style.fillColor(), 1.f, style);

        b.vertex[0].location = hole.low();
        b.vertex[0].texCoord.make(0,0);

        b.vertex[1].location = area.low();
        b.vertex[1].texCoord.make(0,0);

        b.vertex[2].location = hole.highLow();
        b.vertex[2].texCoord.make(0,0);

        b.vertex[3].location = area.highLow();
        b.vertex[3].texCoord.make(1,0);

        b.vertex[4].location = hole.high();
        b.vertex[4].texCoord.make(0,0);

        b.vertex[5].location = area.high();
        b.vertex[5].texCoord.make(1,1);

        b.vertex[6].location = hole.lowHigh();
        b.vertex[6].texCoord.make(0,0);

        b.vertex[7].location = area.lowHigh();
        b.vertex[7].texCoord.make(0,1);

        b.vertex[8].location = hole.low();;
        b.vertex[8].texCoord.make(0,0);

        b.vertex[9].location = area.low();;
        b.vertex[9].texCoord.make(0,0);
      }
    }

    // Draw the stroke
    if (style.hasStroke()) {
      Luminous::Style s = style;
      s.stroke().clear();
      s.setFillColor(style.strokeColor());
      if(style.strokeProgram())
        s.setFillProgram(*style.strokeProgram());
      else
        s.setDefaultFillProgram();

      Nimble::Rect outer = area;
      Nimble::Rect inner = area;
      outer.grow(0.5f*style.strokeWidth());
      inner.shrink(0.5f*style.strokeWidth());
      drawRectWithHole(outer, inner, s);

      outer = hole;
      inner = hole;
      outer.grow(0.5f*style.strokeWidth());
      inner.shrink(0.5f*style.strokeWidth());
      drawRectWithHole(outer, inner, s);
    }
  }

  void RenderContext::drawLine(const Nimble::Vector2f & p1, const Nimble::Vector2f & p2, const Luminous::Style & style)
  {
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(
          Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 4, program, style.strokeColor(), 1.f, style);
    Nimble::Vector2f n = (p2 - p1).perpendicular().normalized(style.strokeWidth() / 2.f);
    b.vertex[0].location = p1 + n;
    b.vertex[1].location = p1 - n;
    b.vertex[2].location = p2 + n;
    b.vertex[3].location = p2 - n;
  }

  void RenderContext::drawEllipse(Nimble::Vector2f center,
                   Nimble::Vector2f axis,
                   float otherAxisLength,
                   const Luminous::Style & style,
                   unsigned int lineSegments,
                   float fromRadians,
                   float toRadians)
  {


    Nimble::Vector2 otherAxis = axis.perpendicular().normalized(otherAxisLength);

    Nimble::Matrix3 m(axis.x, otherAxis.x, 0,
                      axis.y, otherAxis.y, 0,
                      0, 0, 1);

    Luminous::Style s = style;
    s.stroke().clear();

    // Fill is an affine transform of a circle
    {
      Luminous::TransformGuard::RightMul transformGuard(*this, Nimble::Matrix3::makeTranslation(center) * m);
      drawCircle(Nimble::Vector2(0, 0), 1.0f, s, lineSegments, fromRadians, toRadians);
    }

    // Stroke should be of constant width, so use drawDonut for the outline
    if(style.hasStroke()) {
      s.setFillColor(style.strokeColor());
      drawDonut(center,
                axis,
                otherAxisLength,
                style.strokeWidth(),
                s,
                lineSegments,
                fromRadians,
                toRadians);
    }
  }

  void RenderContext::drawText(const TextLayout & layout, const Nimble::Vector2f & location,
                               const Nimble::Rectf & viewRect, const TextStyle & style,
                               bool ignoreVerticalAlign)
  {
    // Need to check here that we are using correct texture atlas
    // This will be fixed on next generate() call
    if (!layout.correctAtlas())
      return;

    const Nimble::Matrix4f model = transform();

    FontUniformBlock uniform;
    uniform.invscale = 1.0f / Nimble::Vector2f(model[1][0], model[1][1]).length() / style.textSharpness();
    uniform.split = 0.0f;

    /// @todo how to calculate these?
    const float magic = 175.f;
    const float edge = 0.5f - style.fontRenderWidth() / magic;
    const float strokeWidth = std::min(1.0f, style.strokeWidth() / magic);

    /// @fixme check that these work with already premultiplied colors
    if (style.dropShadowColor().alpha() > 0.0f) {
      uniform.colorIn = uniform.colorOut = style.dropShadowColor() * opacity();
      const float blur = style.dropShadowBlur();
      //uniform.outline.make(edge - (blur + strokeWidth) * 0.5f, edge + (blur - strokeWidth) * 0.5f);
      uniform.outline.make(edge - blur * 0.5f - strokeWidth, edge + blur * 0.5f - strokeWidth);
      drawTextImpl(layout, location, style.dropShadowOffset(), viewRect, style, uniform, fontShader(), model, ignoreVerticalAlign);
    }

    if (style.glow() > 0.0f) {
      uniform.colorIn = uniform.colorOut = style.glowColor() * opacity();
      uniform.outline.make(edge * (1.0f - style.glow()), edge);
      drawTextImpl(layout, location, Nimble::Vector2f(0, 0), viewRect, style, uniform, fontShader(), model, ignoreVerticalAlign);
    }

    // To remove color bleeding at the edge, ignore colorOut if there is no border
    // uniform.split = strokeWidth < 0.000001f ? 0 : edge + strokeWidth * 0.5f;
    // uniform.outline.make(edge - strokeWidth * 0.5f, edge - strokeWidth * 0.5f);
    uniform.split = strokeWidth < 0.000001f ? 0 : edge;
    uniform.outline.make(edge - strokeWidth, edge - strokeWidth);

    uniform.colorIn = style.fillColor() * opacity();
    uniform.colorOut = style.strokeColor() * opacity();

    drawTextImpl(layout, location, Nimble::Vector2f(0, 0), viewRect, style, uniform, fontShader(), model, ignoreVerticalAlign);
  }

  void RenderContext::drawTextImpl(const TextLayout & layout, const Nimble::Vector2f & location,
                                   const Nimble::Vector2f & renderOffset,
                                   const Nimble::Rectf & viewRect, const TextStyle & style,
                                   FontUniformBlock & uniform, const Program & program,
                                   const Nimble::Matrix4f & modelview, bool ignoreVerticalAlign)
  {
    const int maxGlyphsPerCmd = 1000;

    std::map<QByteArray, const Texture *> textures;
    if (style.fill().textures())
      textures = *style.fill().textures();

    Nimble::Matrix4f m;
    m.identity();

    Nimble::Vector2f renderLocation = renderOffset - viewRect.low();
    if(!ignoreVerticalAlign)
      renderLocation.y += layout.verticalOffset();

    for (size_t g = 0; g < layout.groupCount(); ++g) {
      textures["tex"] = layout.texture(g);
      auto & group = layout.group(g);
      if (group.color.isValid()) {
        uniform.colorIn = Radiant::ColorPMA(group.color) * opacity();
      }

      for (int i = 0; i < int(group.items.size());) {
        const int count = std::min(static_cast<int>(group.items.size()) - i, maxGlyphsPerCmd);

        auto b = render<FontVertex, FontUniformBlock>(
              true, PRIMITIVE_TRIANGLE_STRIP, count*6 - 2, count*4, 1, program, &textures);
        uniform.projMatrix = b.uniform->projMatrix;
        *b.uniform = uniform;
        b.uniform->depth = b.depth;

        Nimble::Vector3f offset(renderLocation.x + location.x, renderLocation.y + location.y, 0);
        if (style.textOverflow() == OverflowVisible) {
          b.uniform->clip.set(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
        } else {
          b.uniform->clip = viewRect;
          if(!ignoreVerticalAlign)
            b.uniform->clip.move(Nimble::Vector2f(0.f, -layout.verticalOffset()));
          else
            b.uniform->clip.move(Nimble::Vector2f(0.f, -location.y));
        }

        m.setTranslation(offset);
        b.uniform->modelMatrix = (modelview * m).transpose();

        int index = 0;

        const int first = i;
        for (const int m = count + i; i < m; ++i) {
          auto & item = group.items[i];
          std::copy(item.vertices.begin(), item.vertices.end(), b.vertex);
          b.vertex += 4;

          // first vertex twice
          if (i != first)
            *b.idx++ = index;
          *b.idx++ = index++;
          *b.idx++ = index++;
          *b.idx++ = index++;

          // last vertex twice
          *b.idx++ = index;
          if (i != m - 1)
            *b.idx++ = index++;
        }
      }
    }
  }

  void RenderContext::drawText(const QString & text, const Nimble::Rectf & rect,
                               const TextStyle & style, TextFlags flags)
  {
    if (flags == TextStatic) {
      const SimpleTextLayout & layout = SimpleTextLayout::cachedLayout(text, rect.size(), style.font(), style.textOption());
      drawText(layout, rect.low(), Nimble::Rectf(Nimble::Vector2f(0, 0), rect.size()), style);
    } else {
      SimpleTextLayout layout(text, rect.size(), style.font(), style.textOption());
      layout.generate();
      drawText(layout, rect.low(), Nimble::Rectf(Nimble::Vector2f(0, 0), rect.size()), style);
    }
  }

  Nimble::Size RenderContext::contextSize() const
  {
    return m_data->contextSize();
  }

  void RenderContext::flush()
  {
    int bufferIndex = m_data->m_bufferIndex;
    m_data->m_indexBuffers[bufferIndex].flush(*this);

    for(auto it = m_data->m_vertexBuffers[bufferIndex].begin(); it != m_data->m_vertexBuffers[bufferIndex].end(); ++it)
      it->second.flush(*this);
    for(auto it = m_data->m_uniformBuffers[bufferIndex].begin(); it != m_data->m_uniformBuffers[bufferIndex].end(); ++it)
      it->second.flush(*this);

    m_data->m_driver.flush();
  }

  void RenderContext::beforeTransformChange()
  {
    // flush();
  }

  void RenderContext::pushViewport(const Nimble::Recti &viewport)
  {
    m_data->m_viewportStack.push(viewport);

    m_data->m_driver.setViewport(viewport);
  }

  void RenderContext::popViewport()
  {
    m_data->m_viewportStack.pop();

    if(!m_data->m_viewportStack.empty()) {
      const Nimble::Recti & viewport = currentViewport();
      m_data->m_driver.setViewport(viewport);
    }
  }

  const Nimble::Recti & RenderContext::currentViewport() const
  {
    assert(!m_data->m_viewportStack.empty());
    return m_data->m_viewportStack.top();
  }

  //////////////////////////////////////////////////////////////////////////
  // Luminousv2 direct-mode API
  //
  // These commands are executed directly.
  // They are only called from the CustomOpenGL guard
  //
  //////////////////////////////////////////////////////////////////////////
  void RenderContext::draw(PrimitiveType primType, unsigned int offset, unsigned int primitives)
  {
    m_data->m_driver.draw(primType, offset, primitives);
  }

  void RenderContext::drawIndexed(PrimitiveType primType, unsigned int offset, unsigned int primitives)
  {
    m_data->m_driver.drawIndexed(primType, offset, primitives);
  }
  
  TextureGL & RenderContext::handle(const Texture & texture)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->handle(texture);
  }

  TextureGL * RenderContext::findHandle(const Texture & texture)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->findHandle(texture);
  }

  BufferGL & RenderContext::handle(const Buffer & buffer)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->handle(buffer);
  }

  FrameBufferGL & RenderContext::handle(const FrameBuffer & target)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->handle(target);
  }

  RenderBufferGL & RenderContext::handle(const RenderBuffer & buffer)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->handle(buffer);
  }

  ProgramGL & RenderContext::handle(const Program & program)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->handle(program);
  }

  VertexArrayGL & RenderContext::handle(const VertexArray & vao, ProgramGL * program)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->handle(vao, program);
  }

  bool RenderContext::isOpenGLExtensionSupported(const QByteArray& name)
  {
    // Query number of available extensions
    GLint extensionCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);

    // Check if requested extension is available
    for(int i = 0; i < extensionCount; ++i) {
      const char* extensionName = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));

      if(name == extensionName)
        return true;
    }

    return false;
  }

  GLint RenderContext::availableGPUMemory()
  {
    GLint result[4] = {0};

#ifndef RADIANT_OSX
    if(m_data->m_supports_GL_NVX_gpu_memory_info) {
      glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, result);
      return result[0];
    }

    if(m_data->m_supports_GL_ATI_meminfo) {
      glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, result);
      return result[0];
    }
#endif

    return result[0];
  }

  GLint RenderContext::maximumGPUMemory()
  {
    GLint result[4] = {0};

#ifndef RADIANT_OSX
    if(m_data->m_supports_GL_NVX_gpu_memory_info) {
      glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, result);
      return result[0];
    }

    /// @todo this just returns the currently available memory, not total
    if(m_data->m_supports_GL_ATI_meminfo) {
      glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, result);
      return result[0];
    }
#endif

    return result[0];
  }

  Radiant::TimeStamp RenderContext::frameTime() const
  {
    return m_data->m_frameTime;
  }

  unsigned int RenderContext::frameNumber() const
  {
    return m_data->m_frameNumber;
  }

  unsigned int RenderContext::maxTextureSize() const
  {
    return m_data->m_maxTextureSize;
  }

  const Nimble::Rectf & RenderContext::audioPanningArea() const
  {
    return m_data->m_audioPanningArea;
  }

  void RenderContext::setAudioPanningArea(const Nimble::Rect & area)
  {
    m_data->m_audioPanningArea = area;
  }

  DxInterop * RenderContext::dxInteropApi()
  {
    if (!m_data->m_dxInteropApi.init())
      return nullptr;
    return &m_data->m_dxInteropApi;
  }

  //////////////////////////////////////////////////////////////////////////
  // Deferred mode API
  // 
  // All these commands generate a RenderCommand that can be reordered
  //
  //////////////////////////////////////////////////////////////////////////
  void RenderContext::clear(ClearMask mask, const Radiant::ColorPMA & clearColor, double clearDepth, int clearStencil)
  {
    m_data->m_driver.clear(mask, clearColor, clearDepth, clearStencil);
  }

  const Program & RenderContext::basicShader() const
  {
    return m_data->m_basicShader;
  }

  const Program & RenderContext::texShader() const
  {
    return m_data->m_texShader;
  }

  const Program & RenderContext::trilinearTexShader() const
  {
    return m_data->m_trilinearTexShader;
  }

  float RenderContext::approximateScaling() const
  {
    return std::sqrt(std::abs(transform3().upperLeft().det()));
  }

  const Texture & RenderContext::dashedLineTexture(float dashPortion) const
  {
    const unsigned int w = m_data->m_dashedLineImages.size() - 1;
    // Round up to make sure that when this is used to render dashed borders,
    // the texture doesn't stop before bottom or right edges.
    const unsigned int idx = std::min<unsigned int>(w, std::max<int>(0, std::ceil(dashPortion * w)));

    // m_dashedLineImages[x] is an image like this:
    // XXX....
    // XXX....
    // XXX....
    // .......
    // .......
    // .......
    // .......
    // Where X are white pixels and . are transparent pixels.
    // The width and height of box X is x pixels.

    Luminous::Image & img = m_data->m_dashedLineImages[idx];
    if (img.isEmpty()) {
      img.allocate(w, w, Luminous::PixelFormat::bgraUByte());
      for (unsigned int y = 0; y < idx; ++y) {
        unsigned char * data = img.line(y);
        unsigned char * border = data + idx * 4;
        unsigned char * end = data + w * 4;
        for (; data < border; data += 4) {
          data[0] = 255u;
          data[1] = 255u;
          data[2] = 255u;
          data[3] = 255u;
        }
        for (; data < end; data += 4) {
          data[0] = 0;
          data[1] = 0;
          data[2] = 0;
          data[3] = 0;
        }
      }
      for (unsigned int y = idx; y < w; ++y) {
        unsigned char * data = img.line(y);
        memset(data, 0, w*4);
      }
      img.texture().setWrap(Luminous::Texture::WRAP_REPEAT,
                            Luminous::Texture::WRAP_REPEAT,
                            Luminous::Texture::WRAP_REPEAT);
      img.texture().setMagFilter(Luminous::Texture::FILTER_NEAREST);
    }
    return img.texture();
  }

  const Program & RenderContext::fontShader() const
  {
    return m_data->m_fontShader;
  }

  const Program & RenderContext::splineShader() const
  {
    return m_data->m_splineShader;
  }

  StateGL & RenderContext::stateGl()
  {
    return m_data->m_driverGL->stateGl();
  }

  int RenderContext::uniformBufferOffsetAlignment() const
  {
    return m_data->m_uniformBufferOffsetAlignment;
  }

  void RenderContext::pushFrameBuffer(const FrameBuffer &target)
  {
    m_data->m_driverGL->pushFrameBuffer(target);

    m_data->m_frameBufferStack.push(&target);

    // Push new projection matrix
    pushViewTransform(Nimble::Matrix4::ortho3D(0.f, target.size().width(), 0.f, target.size().height(), -1.f, 1.f));

    // Reset transformation matrix to identity
    pushTransform();
    setTransform(Nimble::Matrix4::IDENTITY);

    const Nimble::Recti viewport(0, 0, target.size().width(), target.size().height());

    pushClipRect(viewport.cast<float>());

    // Push a scissor area that is the size of the frame buffer. This is done
    // because the currently defined scissor area might be smaller the viewport
    // defined by the frame buffer.
    pushScissorRect(viewport);

    // Push viewport
    pushViewport(viewport);

    // Push and reset the clip mask
    pushClipMaskStack(0);

    // Reset the render call count for this target
    m_data->m_renderCalls.push(0);
  }

  void RenderContext::popFrameBuffer()
  {
    // Restore viewport
    popViewport();

    // Restore scissor area
    popScissorRect();

    popClipRect();

    // Restore the matrix stack
    popTransform();
    popViewTransform();

    assert(!m_data->m_frameBufferStack.empty());
    m_data->m_frameBufferStack.pop();

    popClipMaskStack();

    m_data->m_renderCalls.pop();
    m_data->m_driverGL->popFrameBuffer();
  }

  void RenderContext::pushBlockObjects(ObjectMask objectMask)
  {
    m_data->m_blockObjectsStack.push(objectMask);
  }

  void RenderContext::popBlockObjects()
  {
    m_data->m_blockObjectsStack.pop();
  }

  bool RenderContext::blockObject(RenderContext::ObjectMask mask) const
  {
    if(m_data->m_blockObjectsStack.empty())
      return false;
    return (m_data->m_blockObjectsStack.top() & mask) != 0;
  }

  void RenderContext::beginFrame(Radiant::TimeStamp frameTime, unsigned int frameNumber)
  {
    m_data->m_frameTime = frameTime;
    m_data->m_frameNumber = frameNumber;
    if(m_data->m_postProcessFilters) {
      m_data->createPostProcessFilters(*this, *m_data->m_postProcessFilters);
      // Reorders the chain is necessary
      m_data->m_postProcessChain.prepare();
    }

    pushClipStack();

    assert(stackSize() == 1);

    m_data->m_driver.preFrame();

    assert(m_data->m_frameBufferStack.empty());

    // Push frame buffer attached to back buffer. Don't use the RenderContext API
    // to avoid the guard.
    m_data->m_driverGL->pushFrameBuffer(m_data->m_finalFrameBuffer);
    m_data->m_frameBufferStack.push(&m_data->m_finalFrameBuffer);

    // Check if we need an auxiliary frame buffer (save this so we can pop when we are done)
    m_data->m_useOffScreenFrameBuffer = !m_data->m_window->directRendering() ||
        m_data->m_postProcessChain.numEnabledFilters() > 0;

    // Push auxiliary frame buffer if needed
    if(m_data->m_useOffScreenFrameBuffer) {
      auto & fbos = m_data->m_offScreenFrameBuffers;
      if (fbos.empty())
        m_data->resizeOffScreenFrameBuffers(1);
      m_data->m_driverGL->pushFrameBuffer(fbos[frameNumber % fbos.size()]);
      m_data->m_frameBufferStack.push(&fbos[frameNumber % fbos.size()]);
    }


    assert(!m_data->m_frameBufferStack.empty());
    assert(m_data->m_frameBufferStack.top()->targetType() != FrameBuffer::INVALID);

    // Push default opacity
    assert(m_data->m_opacityStack.empty());
    m_data->m_opacityStack.push(1.f);
  }

  void RenderContext::endFrame(unsigned int blitFrame)
  {
    if(!m_data->m_window->directRendering()) {
      auto & fbos = m_data->m_offScreenFrameBuffers;

      if (blitFrame > m_data->m_frameNumber) {
        Radiant::warning("RenderContext::endFrame # Tried to blit frame %u, but the latest frame is %u",
                         blitFrame, m_data->m_frameNumber);
        blitFrame = m_data->m_frameNumber;
      }
      const unsigned int maxBufferCount = 4;
      unsigned int neededBufferCount = m_data->m_frameNumber - blitFrame + 1;

      if (neededBufferCount > maxBufferCount) {
        neededBufferCount = maxBufferCount;
        blitFrame = m_data->m_frameNumber + 1 - maxBufferCount;
      }

      if (neededBufferCount > fbos.size()) {
        blitFrame = std::min<unsigned int>(m_data->m_frameNumber, m_data->m_frameNumber + 1 - fbos.size());
        m_data->resizeOffScreenFrameBuffers(neededBufferCount);
      }

      if (blitFrame != m_data->m_frameNumber) {
        fbos[blitFrame % fbos.size()].setTargetBind(FrameBuffer::BIND_READ);
        m_data->m_driverGL->pushFrameBuffer(fbos[blitFrame % fbos.size()]);
      }

      // Push window frame buffer
      m_data->m_finalFrameBuffer.setTargetBind(FrameBuffer::BIND_DRAW);
      m_data->m_driverGL->pushFrameBuffer(m_data->m_finalFrameBuffer);

      // Blit individual areas from the buffer we are going to swap to the final frame buffer
      for(size_t i = 0; i < m_data->m_window->areaCount(); i++) {
        const Luminous::MultiHead::Area & area = m_data->m_window->area(i);
        blit(area.viewport(), area.viewport());
      }

      m_data->m_driverGL->popFrameBuffer();

      if (blitFrame != m_data->m_frameNumber) {
        fbos[blitFrame % fbos.size()].setTargetBind(FrameBuffer::BIND_DEFAULT);
        m_data->m_driverGL->popFrameBuffer();
      }
    }

    // Pop our auxiliary frame buffer if we have used it
    if(m_data->m_useOffScreenFrameBuffer) {
      m_data->m_driverGL->popFrameBuffer();
      m_data->m_frameBufferStack.pop();
    }

    //Actual drawing
    flush();
    m_data->m_bufferIndex = (m_data->m_bufferIndex + 1) % RenderContext::Internal::BUFFERSETS;

    m_data->m_driver.postFrame();
    /// @todo how do we generate this properly? Should we somehow linearize the depth buffer?
    m_data->m_automaticDepthDiff = -1.0f / std::max(m_data->m_renderCalls.top(), 10000);
    assert(m_data->m_renderCalls.size() == 1);
    m_data->m_renderCalls.top() = 0;


    // Pop opacity
    assert(m_data->m_opacityStack.size() == 1);
    m_data->m_opacityStack.pop();

    // Pop the default target
    m_data->m_driverGL->popFrameBuffer();
    m_data->m_frameBufferStack.pop();
    assert(m_data->m_frameBufferStack.empty());

    assert(stackSize() == 1);

    assert(m_data->m_clipStacks.size() == 1);

    popClipStack();

    // Call glFinish if configured to minimize latency at the expense of
    // throughput.
    const bool useGLFinish = m_data->m_window->screen()->useGlFinish();
    if(useGLFinish) {
      // On AMD / Linux / Multi-GPU setup, this is absolute requirement.
      // If we wouldn't call glFinish, it seems that calling swapBuffers
      // just adds just added OpenGL commands to a queue, and actually
      // executes those when there is enough stuff on the queue. With
      // really small application, that might mean ~100 frame latency,
      // on huge applications, maybe 1-2 frames.
      // On NVIDIA / Linux this makes rendering slower, but also seems
      // to remove ~1-2 frame latency. This was seen with
      // Experimental/LatencyTest. It is unknown how this affects the
      // speed and frame latency in other platforms / setups.
      //
      // This also makes Luminous::RenderContext::Internal::BUFFERSETS
      // useless, there doesn't seem to be any difference in speed when
      // BUFFERSETS is changed to something else than 1.
      m_data->m_driverGL->opengl().glFinish();
    }
  }

  void RenderContext::beginArea()
  {
    assert(stackSize() == 1);
    assert(transform() == Nimble::Matrix4::IDENTITY);
  }

  void RenderContext::endArea()
  {
    assert(stackSize() == 1);
    assert(transform() == Nimble::Matrix4::IDENTITY);
  }

  void RenderContext::initPostProcess(Luminous::PostProcessFilters & filters)
  {
    m_data->m_postProcessFilters = &filters;

    // Add color correction filter if any of the areas have a profile defined
    for(size_t i = 0; i < m_data->m_window->areaCount(); ++i) {

      if(m_data->m_window->isAreaSoftwareColorCorrected(static_cast<int>(i))) {
        Radiant::info("Enabling software color correction for area %lu", i);

        // Check if filter already exists
        if(!m_data->m_postProcessChain.hasFilterType<ColorCorrectionFilter>()) {

          auto filter = std::make_shared<Luminous::ColorCorrectionFilter>();
          filter->setOrder(PostProcessChain::Color_Correction);

          auto context = std::make_shared<Luminous::PostProcessContext>(filter);

          context->initialize(*this, contextSize());
          m_data->m_postProcessChain.insert(context);
        }
      }
    }
  }

  void RenderContext::postProcess()
  {
    PostProcessChain & chain = m_data->m_postProcessChain;
    size_t numFilters = chain.numEnabledFilters();

    if(numFilters == 0)
      return;

    const Nimble::Recti viewport(Nimble::Vector2i(0, 0), contextSize().cast<int>());

    // Copy off-screen buffers to use as the source of the first filter.
    // This is done because the off-screen target contains multisampled depth
    // and color buffers and usually filters are only interested in resolved
    // color data. By blitting to an FBO that only contains a non-multisampled
    // color buffer (default) the multisample resolution happens automatically.
    PostProcessContextPtr first = *chain.begin();
    first->frameBuffer().setTargetBind(FrameBuffer::BIND_DRAW);
    {
      Luminous::FrameBufferGuard g(*this, first->frameBuffer());
      blit(viewport, viewport, CLEARMASK_COLOR_DEPTH_STENCIL);
    }
    first->frameBuffer().setTargetBind(FrameBuffer::BIND_DEFAULT);

    if(numFilters > 100) {
      Radiant::warning("Using over 100 post processing filters.");
    }

    assert(m_data->m_window);

    // Apply filters in filter chain
    for(PostProcessChain::FilterIterator it(chain.begin()), next(it);
        it != chain.end() && next++ != chain.end(); ++it) {

      const PostProcessContextPtr ppf = *it;
      assert(ppf && ppf->enabled());

      // Note: if isLast is true, next is invalid
      bool isLast = (next == chain.end());

      // If this is the last filter, use the default frame buffer,
      // otherwise use the auxiliary off-screen frame buffer of the next filter
      const FrameBuffer & frameBuffer = isLast ?
            m_data->defaultFrameBuffer() :
            next->frameBuffer();

      // Push the next auxilary frame buffer
      Luminous::FrameBufferGuard bufferGuard(*this, frameBuffer);

      // Run each area through the filter
      for(unsigned j = 0; j < m_data->m_window->areaCount(); j++) {
        const MultiHead::Area & area = m_data->m_window->area(j);

        m_data->m_driver.setViewport(viewport);
        m_data->m_driver.setScissor(area.viewport());

        // Sets the current area to be rendered
        setWindowArea(m_data->m_window, &area);

        ppf->doFilter(*this);
      }
    }
  }

  /// @todo handle 90/180 degree MultiHead Area rotations
  void RenderContext::processFilter(Luminous::PostProcessFilterPtr filter, Nimble::Rect filterRect)
  {
    // filterRect is the area we are applying this filter. Transform it to gfx coordinates.
    filterRect.transform(transform3());

    Nimble::Rect areaBounds = area()->graphicsBounds();

    // This is the visible part of filterRect in the current area
    Nimble::Recti visibleAreaRect = areaBounds.intersection(filterRect).cast<int>();

    // Nothing to do?
    if (visibleAreaRect.width() <= 0 || visibleAreaRect.height() <= 0)
      return;

    Nimble::Rect windowBounds = window()->graphicsBounds();
    Nimble::Recti visibleWindowRect = windowBounds.intersection(filterRect).cast<int>();
    // In the post processing framework each context (window) has its own FBO
    // for each filter. Optimize the FBO size by using the visible part
    // of the window (union of all visible areas).
    Nimble::Size fboSize = visibleWindowRect.size();

    auto filterCtx = m_data->m_postProcessChain.get(filter);

    // Create the context for the filter if necessary
    if(!filterCtx) {
      filterCtx = std::make_shared<PostProcessContext>(filter);
      filterCtx->initialize(*this, fboSize);

      m_data->m_postProcessChain.insert(filterCtx);
    } else {
      filterCtx->resize(fboSize);
    }

    assert(!m_data->m_frameBufferStack.empty());

    // Blit from current frame buffer to filter's auxiliary frame buffer
    filterCtx->frameBuffer().setTargetBind(FrameBuffer::BIND_DRAW);
    {
      Luminous::FrameBufferGuard bufferGuard(*this, filterCtx->frameBuffer());
      Nimble::Recti src(Nimble::Vector2i(visibleAreaRect.low().x - windowBounds.low().x,
                                         window()->height() + windowBounds.low().y - visibleAreaRect.high().y),
                        visibleAreaRect.size());
      Nimble::Recti dest(Nimble::Vector2i(visibleAreaRect.low().x - visibleWindowRect.low().x,
                                          visibleWindowRect.height() - (visibleAreaRect.high().y - visibleWindowRect.low().y)),
                         visibleAreaRect.size());

      // dest is in the FBO coordinates (see fboSize), src in window coordinates
      blit(src, dest, CLEARMASK_COLOR_DEPTH_STENCIL);
    }

    filterCtx->frameBuffer().setTargetBind(FrameBuffer::BIND_DEFAULT);

    // Finally calculate a texture matrix that can be used in a fragment shader.
    // If you render filterRect to screen with normal UV coordinates from 0 to 1,
    // this matrix can be used to access this area's part of the window FBO texture
    // correctly
    auto diff = filterRect.low() - visibleWindowRect.low().cast<float>();

    // (0, 1) translation and scale (1, -1) are flipping the Y axis
    Nimble::Matrix3f uvm = Nimble::Matrix3f::makeTranslation(0, 1) *
        Nimble::Matrix3f::makeScale(filterRect.width() / visibleWindowRect.width(),
                                    -filterRect.height() / visibleWindowRect.height()) *
        Nimble::Matrix3f::makeTranslation(diff.x/filterRect.width(), diff.y/filterRect.height());

    filterCtx->doFilter(*this, uvm);
  }

  void RenderContext::initialize()
  {
    m_data->initialize();

    m_data->m_supports_GL_ATI_meminfo = isOpenGLExtensionSupported("GL_ATI_meminfo");
    m_data->m_supports_GL_NVX_gpu_memory_info = isOpenGLExtensionSupported("GL_NVX_gpu_memory_info");
    int maxSize = 1024;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    m_data->m_maxTextureSize = maxSize;
  }

  void RenderContext::pushOpacity(float opacity)
  {
    auto value = 1.f;

    if(!m_data->m_opacityStack.empty())
      value = m_data->m_opacityStack.top();

    m_data->m_opacityStack.push(value * opacity);
  }

  void RenderContext::popOpacity()
  {
    assert(!m_data->m_opacityStack.empty());
    m_data->m_opacityStack.pop();
  }

  float RenderContext::opacity() const
  {
    assert(!m_data->m_opacityStack.empty());
    return m_data->m_opacityStack.top();
  }

  void RenderContext::setOpacity(float opacity)
  {
    assert(!m_data->m_opacityStack.empty());
    m_data->m_opacityStack.top() = opacity;
  }

  void RenderContext::setDefaultState()
  {
    m_data->m_driverGL->setDefaultState();
  }

  void RenderContext::pushScissorRect(const Nimble::Recti & scissorArea)
  {
    m_data->m_scissorStack.push(scissorArea);

    m_data->m_driver.setScissor(scissorArea);
  }

  void RenderContext::popScissorRect()
  {
    assert(!m_data->m_scissorStack.empty());
    m_data->m_scissorStack.pop();

    if(!m_data->m_scissorStack.empty()) {
      const Nimble::Recti & oldArea = currentScissorArea();
      m_data->m_driver.setScissor(oldArea);
    }
  }

  const Nimble::Recti & RenderContext::currentScissorArea() const
  {
    assert(!m_data->m_scissorStack.empty());
    return m_data->m_scissorStack.top();
  }

  void RenderContext::blit(const Nimble::Recti & src, const Nimble::Recti & dst,
                           ClearMask mask, Texture::Filter filter)
  {
    m_data->m_driver.blit(src, dst, mask, filter);
  }

  void RenderContext::setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer)
  {
    m_data->m_driverGL->setRenderBuffers(colorBuffer, depthBuffer, stencilBuffer);
  }

  void RenderContext::setBlendMode(const BlendMode & mode)
  {
    m_data->m_driverGL->setBlendMode(mode);
  }

  void RenderContext::setDepthMode(const DepthMode & mode)
  {
    m_data->m_driverGL->setDepthMode(mode);
  }

  void RenderContext::setStencilMode(const StencilMode & mode)
  {
    m_data->m_stencilMode = mode;
    m_data->m_driverGL->setStencilMode(mode);
  }

  StencilMode RenderContext::stencilMode() const
  {
    return m_data->m_stencilMode;
  }

  void RenderContext::setCullMode(const CullMode& mode)
  {
    m_data->m_driverGL->setCullMode(mode);
  }

  void RenderContext::setDrawBuffers(const std::vector<GLenum> & buffers)
  {
    m_data->m_driverGL->setDrawBuffers(buffers);
  }

  void RenderContext::setDefaultDrawBuffers()
  {
    std::vector<GLenum> buffers;
    buffers.push_back(GL_BACK_LEFT);

    setDrawBuffers(buffers);
  }

  void RenderContext::setFrontFace(FaceWinding winding)
  {
    m_data->m_driverGL->setFrontFace(winding);
  }

  void RenderContext::enableClipPlanes(const QList<int> & planes)
  {
    m_data->m_driverGL->enableClipDistance(planes);
  }

  void RenderContext::disableClipPlanes(const QList<int> & planes)
  {
    m_data->m_driverGL->disableClipDistance(planes);
  }
  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  CustomOpenGL::CustomOpenGL(RenderContext & r, bool reset)
    : m_r(r)
    , m_reset(reset)
  {
    // First, flush the current deferred render queues
    r.flush();

    if(reset) {
      r.glPointSize(1.f);
      r.glLineWidth(1.f);
      r.glUseProgram(0);
      r.glDisable(GL_DEPTH_TEST);
      r.glBindRenderbuffer(GL_RENDERBUFFER, 0);
      r.glBindFramebuffer(GL_FRAMEBUFFER, 0);
      GLERROR("CustomOpenGL::CustomOpenGL");
    }
  }

  CustomOpenGL::~CustomOpenGL()
  {
    GLERROR("CustomOpenGL::~CustomOpenGL");
    if (m_reset)
      m_r.setDefaultState();
  }


}
