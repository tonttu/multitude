/* COPYRIGHT
 */

#include "RenderContext.hpp"

#include "Error.hpp"
#include "Texture.hpp"

#include "RenderCommand.hpp"

#include <Nimble/ClipStack.hpp>

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
    enum { MAX_TEXTURES = 64, BUFFERSETS = 2 };

    Internal(RenderDriver & renderDriver, const Luminous::MultiHead::Window * win)
        : m_recursionLimit(DEFAULT_RECURSION_LIMIT)
        , m_recursionDepth(0)
        , m_renderCount(0)
        , m_frameCount(0)
        , m_area(0)
        , m_window(win)
        , m_initialized(false)
        , m_uniformBufferOffsetAlignment(0)
        , m_automaticDepthDiff(-1.0f/100000.0f)
        , m_driver(renderDriver)
        , m_driverGL(dynamic_cast<RenderDriverGL*>(&renderDriver))
        , m_bufferIndex(0)
        , m_defaultRenderTarget(RenderTarget::WINDOW)
        , m_defaultOffScreenRenderTarget(RenderTarget::NORMAL)
        , m_currentRenderTarget(0)
        , m_postProcessFilters(0)
    {
      // Reset render call count
      m_renderCalls.push(0);

      // Initialize default render target size
      assert(win);
      m_defaultRenderTarget.setSize(Nimble::Size(win->size().x, win->size().y));

      // Set initial data for an off-screen render target
      // The hardware resource is not created if this is actually never bound
      m_defaultOffScreenRenderTarget.setSize(Nimble::Size(win->size().x, win->size().y));
      m_defaultOffScreenRenderTarget.setSamples(win->antiAliasingSamples());

      m_defaultOffScreenRenderTarget.createRenderBufferAttachment(GL_COLOR_ATTACHMENT0, GL_RGBA);
      m_defaultOffScreenRenderTarget.createRenderBufferAttachment(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT);

      m_basicShader.loadShader("Luminous/GLSL150/basic.vs", ShaderGLSL::Vertex);
      m_basicShader.loadShader("Luminous/GLSL150/basic.fs", ShaderGLSL::Fragment);
      Luminous::VertexDescription desc;
      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      m_basicShader.setVertexDescription(desc);

      m_texShader.loadShader("Luminous/GLSL150/tex.vs", ShaderGLSL::Vertex);
      m_texShader.loadShader("Luminous/GLSL150/tex.fs", ShaderGLSL::Fragment);
      desc = Luminous::VertexDescription();
      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      desc.addAttribute<Nimble::Vector2>("vertex_uv");
      m_texShader.setVertexDescription(desc);

      m_fontShader.loadShader("Luminous/GLSL150/distance_field.vs", ShaderGLSL::Vertex);
      m_fontShader.loadShader("Luminous/GLSL150/distance_field.fs", ShaderGLSL::Fragment);
      desc = Luminous::VertexDescription();
      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      desc.addAttribute<Nimble::Vector2>("vertex_uv");
      desc.addAttribute<float>("vertex_invsize");
      m_fontShader.setVertexDescription(desc);
      m_fontShader.setSampleShading(1.0f);
    }

    ~Internal()
    {
    }


    void initialize() {

      assert(m_window != 0);

      if(!m_initialized) {
        m_initialized = true;

        m_uniformBufferOffsetAlignment = m_driver.uniformBufferOffsetAlignment();

        Radiant::info("RenderContext::Internal # init ok");
      }
    }

    Nimble::Vector2f contextSize() const
    {
      if(m_window)
        return Nimble::Vector2f(m_window->size().x, m_window->size().y);

      /// @todo why not zero vector?
      return Nimble::Vector2f(10, 10);
    }

    void createPostProcessFilters(RenderContext & rc, const Luminous::PostProcessFilters & filters)
    {
      for(Luminous::PostProcessFilters::const_iterator it = filters.begin(); it != filters.end(); ++it) {

        if(m_postProcessChain.contains(*it))
          continue;

        // Create a new context for the filter
        auto context = std::make_shared<PostProcessContext>(*it);

        if(context) {
          // By default resizes new render targets to current context size
          context->initialize(rc);
          m_postProcessChain.insert(context);
        }
      }
    }

    RenderTarget & defaultRenderTarget()
    {
      return m_window->directRendering() ?
            m_defaultRenderTarget :
            m_defaultOffScreenRenderTarget;
    }

    size_t m_recursionLimit;
    size_t m_recursionDepth;

    std::stack<Nimble::ClipStack> m_clipStacks;
 
    unsigned long m_renderCount;
    unsigned long m_frameCount;

    const Luminous::MultiHead::Area * m_area;
    const Luminous::MultiHead::Window * m_window;

    Transformer m_viewTransformer;

    bool m_initialized;

    // Viewports defined as x1,y1,x2,y2
    typedef std::stack<Nimble::Recti, std::vector<Nimble::Recti> > ViewportStack;
    ViewportStack m_viewportStack;

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
    Program m_fontShader;

    Luminous::RenderDriver & m_driver;
    Luminous::RenderDriverGL * m_driverGL;

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
            ctx.unmapBuffer(it->buffer, it->type, 0, it->reservedBytes);
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

    // Default window framebuffer
    RenderTarget m_defaultRenderTarget;
    RenderTarget m_defaultOffScreenRenderTarget;
    const RenderTarget * m_currentRenderTarget;

    // owned by Application
    const Luminous::PostProcessFilters * m_postProcessFilters;
    PostProcessChain m_postProcessChain;

    std::stack<float> m_opacityStack;
  };

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  RenderContext::RenderContext(Luminous::RenderDriver & driver, const Luminous::MultiHead::Window * win)
      : Transformer(),
      m_data(new Internal(driver, win))
  {
    resetTransform();
    m_data->m_recursionDepth = 0;
  }

  RenderContext::~RenderContext()
  {
    debugLuminous("Closing OpenGL context. Rendered %lu things in %lu frames, %lu things per frame",
         m_data->m_renderCount, m_data->m_frameCount,
         m_data->m_renderCount / std::max(m_data->m_frameCount, (unsigned long) 1));
    delete m_data;
  }

  void RenderContext::setArea(const Luminous::MultiHead::Area * area)
  {
    m_data->m_area = area;
    m_data->m_window = area->window();
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

  const RenderTarget & RenderContext::currentRenderTarget() const
  {
    assert(m_data->m_currentRenderTarget);

    return *m_data->m_currentRenderTarget;
  }

  void RenderContext::setRecursionLimit(size_t limit)
  {
    m_data->m_recursionLimit = limit;
  }

  size_t RenderContext::recursionLimit() const
  {
    return m_data->m_recursionLimit;
  }

  void RenderContext::setRecursionDepth(size_t rd)
  {
    m_data->m_recursionDepth = rd;
  }

  size_t RenderContext::recursionDepth() const
  {
    return m_data->m_recursionDepth;
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

  bool RenderContext::isVisible(const Nimble::Rectangle & area)
  {
    if(m_data->m_clipStacks.empty())
      return true;

    return m_data->m_clipStacks.top().isVisible(area);
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
      program, style.strokeColor(), style.strokeWidth(), style);

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
      /// @todo Automagically determine the proper number of linesegments
      linesegments = 32;
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
    if (style.fillColor().w > 0.f) {
      const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
      auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_FAN, 0, linesegments + 2, program, style.fillColor(), 1.f, style);
      // Center is the first vertex in a fan
      b.vertex[0].location = center;
      // Create the rest of the vertices
      fill(&b.vertex[1]);
    }

    // Draw stroke
    if(style.strokeWidth() > 0.f && style.strokeColor().alpha() > 0.f) {
      Luminous::Style s = style;
      s.stroke() = Stroke();
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
                                float fromRadians, float toRadians)
  {
    if(linesegments == 0) {
      /// @todo automagically determine divisions?
      linesegments = 32;
    }

    float rotation = axis.angle();

    // Ellipse parameters
    float a = axis.length();
    float b = otherAxisLength;

    pushTransformRightMul(Nimble::Matrix3::makeTranslation(center) * Nimble::Matrix3::makeRotation(rotation));

    bool isFilled = style.fillColor().alpha() > 0.f;
    bool stroke = style.strokeWidth() > 0.0f;

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
      if (style.fill().textures().empty()) {
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
        textured.vertex[2*i].texCoord = iSpan * (in-low);
        textured.vertex[2*i+1].texCoord = iSpan * (out-low);
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
    popTransform();
  }

  void RenderContext::drawWedge(const Nimble::Vector2f & center, float radius1,
                                float radius2, float fromRadians, float toRadians,
                                Style & style,
                                int segments)
  {
    // @todo Create fill geometry

    // Draw two arcs
    drawArc(center, radius1, fromRadians, toRadians, style, segments);
    drawArc(center, radius2, fromRadians, toRadians, style, segments);

    // Draw sector edges
    /// @todo these look a bit crappy as the blending doesn't match  the arcs properly
    Nimble::Vector2f p0 =
        center + radius1 * Nimble::Vector2f(cos(fromRadians), sin(fromRadians));
    Nimble::Vector2f p1 =
        center + radius2 * Nimble::Vector2f(cos(fromRadians), sin(fromRadians));

    Nimble::Vector2f p2 =
        center + radius1 * Nimble::Vector2f(cos(toRadians), sin(toRadians));
    Nimble::Vector2f p3 =
        center + radius2 * Nimble::Vector2f(cos(toRadians), sin(toRadians));

    drawLine(p0, p1, style);
    drawLine(p2, p3, style);
  }

  void RenderContext::addRenderCounter()
  {
    m_data->m_renderCount++;
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
      std::size_t vertexSize, std::size_t vertexCount, Buffer::Type type)
  {
    int bufferIndex = m_data->m_bufferIndex;
    Internal::BufferPool & pool = type == Buffer::INDEX
        ? m_data->m_indexBuffers[bufferIndex]
        : type == Buffer::VERTEX
          ? m_data->m_vertexBuffers[bufferIndex][vertexSize]
          : m_data->m_uniformBuffers[bufferIndex][vertexSize];

    const std::size_t requiredBytes = vertexSize * vertexCount;

    SharedBuffer * buffer = nullptr;
    std::size_t nextSize = 1 << 20;
    for(int i = 0; ;++i) {
      if(i >= int(pool.buffers.size())) {
        pool.buffers.emplace_back(type);
        buffer = &pool.buffers.back();
        buffer->buffer.setData(nullptr, std::max(requiredBytes, nextSize), Buffer::STREAM_DRAW);
        /// Fix the generation so it doesn't get automatically overwritten by an upload()
        buffer->buffer.setGeneration(0);
        break;
      }

      buffer = &pool.buffers[i];
      if(buffer->buffer.size() - buffer->reservedBytes >= requiredBytes)
        break;

      nextSize <<= 1;
    }
    return buffer;
  }

  std::pair<void *, RenderContext::SharedBuffer *> RenderContext::sharedBuffer(
      std::size_t vertexSize, std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset)
  {
    SharedBuffer * buffer = findAvailableBuffer(vertexSize, maxVertexCount, type);

    char * data = mapBuffer<char>(buffer->buffer, type, Buffer::MAP_WRITE |
                                  Buffer::MAP_INVALIDATE_RANGE | Buffer::MAP_FLUSH_EXPLICIT);
    assert(data);
    data += buffer->reservedBytes;
    offset = buffer->reservedBytes / vertexSize;
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
    unsigned int indexOffset, vertexOffset, uniformOffset;

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
      char * data = mapBuffer<char>(ibuffer->buffer, Buffer::INDEX, Buffer::MAP_WRITE |
                                    Buffer::MAP_INVALIDATE_RANGE | Buffer::MAP_FLUSH_EXPLICIT);
      mappedIndexBuffer = reinterpret_cast<unsigned int*>(data + ibuffer->reservedBytes);
      indexOffset = ibuffer->reservedBytes / sizeof(unsigned int);
      ibuffer->reservedBytes += sizeof(unsigned int)*indexCount;
    }

    cmd.primitiveCount = ( indexCount > 0 ? indexCount : vertexCount );
    cmd.indexed = (indexCount > 0);
    cmd.indexOffset = indexOffset;
    cmd.vertexOffset = vertexOffset;
    cmd.uniformOffsetBytes = uniformOffset * uniformSize;
    cmd.uniformSizeBytes = uniformSize;

    depth = 0.99999f + m_data->m_automaticDepthDiff * m_data->m_renderCalls.top();
    ++(m_data->m_renderCalls.top());

    return cmd;
  }

  /// Drawing utility commands
  //////////////////////////////////////////////////////////////////////////

  //
  void RenderContext::drawRect(const Nimble::Vector2f & min, const Nimble::Vector2f & max, const Style & style)
  {
    drawRect(Nimble::Rect(min, max), style);
  }

  void RenderContext::drawRect(const Nimble::Rectf & rect, const Style & style)
  {
    drawRect(rect, Nimble::Rect(0, 0, 1, 1), style);
  }

  void RenderContext::drawRect(const Nimble::Rectf & rect, const Nimble::Rectf & uvs, const Style & style)
  {
    if (style.fillColor().w > 0.f) {
      if(style.fill().textures().empty()) {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 4, program, style.fillColor(), 1.f, style);
        b.vertex[0].location = rect.low();
        b.vertex[1].location = rect.highLow();
        b.vertex[2].location = rect.lowHigh();
        b.vertex[3].location = rect.high();
      }
      else {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        auto b = drawPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 4, program, style.fillColor(), 1.f, style);

        b.vertex[0].location = rect.low();
        b.vertex[0].texCoord = uvs.low();

        b.vertex[1].location = rect.highLow();
        b.vertex[1].texCoord = uvs.highLow();

        b.vertex[2].location = rect.lowHigh();
        b.vertex[2].texCoord = uvs.lowHigh();

        b.vertex[3].location = rect.high();
        b.vertex[3].texCoord = uvs.high();
      }
    }

    // Draw the outline
    if (style.strokeWidth() > 0.f && style.strokeColor().w > 0.f) {

      Luminous::Style s = style;
      s.stroke() = Stroke();
      s.setFillColor(style.strokeColor());
      if(style.strokeProgram())
        s.setFillProgram(*style.strokeProgram());
      else
        s.setDefaultFillProgram();

      Nimble::Rect outer = rect;
      Nimble::Rect inner = rect;
      outer.increaseSize(0.5f*style.strokeWidth());
      inner.smaller(0.5f*style.strokeWidth());

      drawRectWithHole(outer, inner, s);
      /*
      const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
      auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_LINE_STRIP, 0, 5, program, style.strokeColor(), style.strokeWidth(), style);
      b.vertex[0].location.make(rect.low(), b.depth);
      b.vertex[1].location.make(rect.highLow(), b.depth);
      b.vertex[2].location.make(rect.high(), b.depth);
      b.vertex[3].location.make(rect.lowHigh(), b.depth);
      b.vertex[4].location.make(rect.low(), b.depth);
      */
    }
  }

  void RenderContext::drawQuad(const Nimble::Vector2 *vertices, const Nimble::Vector2 *uvs, const Style &style)
  {
    if (style.fillColor().w > 0.f) {
      if(style.fill().textures().empty()) {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 4, program, style.fillColor(), 1.f, style);
        b.vertex[0].location = vertices[0];
        b.vertex[1].location = vertices[1];
        b.vertex[2].location = vertices[2];
        b.vertex[3].location = vertices[3];
      }
      else {
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
    if (style.strokeWidth() > 0.f && style.strokeColor().w > 0.f) {
      Radiant::warning("RenderContext::drawQuad # Stroke is not implemented");
    }
  }

  //
  void RenderContext::drawRectWithHole(const Nimble::Rectf & area,
                                       const Nimble::Rectf & hole,
                                       const Luminous::Style & style)
  {
    if (style.fillColor().w > 0.f) {
      if (style.fill().textures().empty()) {
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
    if (style.strokeWidth() > 0.f && style.strokeColor().w > 0.f) {
      Luminous::Style s = style;
      s.stroke() = Stroke();
      s.setFillColor(style.strokeColor());
      if(style.strokeProgram())
        s.setFillProgram(*style.strokeProgram());
      else
        s.setDefaultFillProgram();

      Nimble::Rect outer = area;
      Nimble::Rect inner = area;
      outer.increaseSize(0.5f*style.strokeWidth());
      inner.smaller(0.5f*style.strokeWidth());
      drawRectWithHole(outer, inner, s);

      outer = hole;
      inner = hole;
      outer.increaseSize(0.5f*style.strokeWidth());
      inner.smaller(0.5f*style.strokeWidth());
      drawRectWithHole(outer, inner, s);
    }
  }

  //
  void RenderContext::drawLine(const Nimble::Vector2f & p1, const Nimble::Vector2f & p2, const Luminous::Style & style)
  {
    assert(style.strokeWidth() > 0.f);
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_LINE, 0, 2, program, style.strokeColor(), style.strokeWidth(), style);
    b.vertex[0].location = p1;
    b.vertex[1].location = p2;
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
    s.stroke() = Stroke();

    // Fill is an affine transform of a circle
    pushTransformRightMul(Nimble::Matrix3::makeTranslation(center) * m);
    drawCircle(Nimble::Vector2(0, 0), 1.0f, s, lineSegments, fromRadians, toRadians);
    popTransform();

    // Stroke should be of constant width, so use drawDonut for the outline
    if(style.strokeColor().alpha() > 0.f && style.strokeWidth() > 0.f) {
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

  //
  void RenderContext::drawPolyLine(const Nimble::Vector2f * points, unsigned int numPoints, const Luminous::Style & style)
  {
    assert(style.strokeWidth() > 0.f);
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    /// @todo Can't rely on supported line sizes. Should just make triangle strips for values > 1
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_LINE_STRIP, 0, numPoints, program, style.strokeColor(), style.strokeWidth(), style);
    for (size_t i = 0; i < numPoints; ++i)
      b.vertex[i].location = points[i];
  }

  void RenderContext::drawPoints(const Nimble::Vector2f * points, size_t numPoints, const Luminous::Style & style)
  {
    /// @todo Can't rely on supported point sizes. Should this just call drawCircle instead for values > 1
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_POINT, 0, numPoints, program, style.strokeColor(), style.strokeWidth(), style);
    for (size_t i = 0; i < numPoints; ++i)
      b.vertex[i].location = points[i];
  }

  void RenderContext::drawText(const TextLayout & layout, const Nimble::Vector2f & location,
                               const Nimble::Rectf & viewRect, const TextStyle & style)
  {
    const Nimble::Matrix4f model = transform();

    FontUniformBlock uniform;
    uniform.invscale = 1.0f / Nimble::Vector2f(model[1][0], model[1][1]).length() / style.textSharpness();
    uniform.split = 0.0f;

    /// @todo how to calculate these?
    const float magic = 175.f;
    const float edge = 0.5f - style.fontRenderWidth() / magic;
    const float strokeWidth = std::min(1.0f, style.strokeWidth() / magic);

    if (style.dropShadowColor().alpha() > 0.0f) {
      uniform.colorIn = uniform.colorOut = style.dropShadowColor();
      uniform.colorIn.w *= opacity();
      uniform.colorOut.w *= opacity();
      const float blur = style.dropShadowBlur();
      //uniform.outline.make(edge - (blur + strokeWidth) * 0.5f, edge + (blur - strokeWidth) * 0.5f);
      uniform.outline.make(edge - blur * 0.5f - strokeWidth, edge + blur * 0.5f - strokeWidth);
      drawTextImpl(layout, location, style.dropShadowOffset(), viewRect, style, uniform, fontShader(), model);
    }

    if (style.glow() > 0.0f) {
      uniform.colorIn = uniform.colorOut = style.glowColor();
      uniform.colorIn.w *= opacity();
      uniform.colorOut.w *= opacity();
      uniform.outline.make(edge * (1.0f - style.glow()), edge);
      drawTextImpl(layout, location, Nimble::Vector2f(0, 0), viewRect, style, uniform, fontShader(), model);
    }

    // To remove color bleeding at the edge, ignore colorOut if there is no border
    // uniform.split = strokeWidth < 0.000001f ? 0 : edge + strokeWidth * 0.5f;
    // uniform.outline.make(edge - strokeWidth * 0.5f, edge - strokeWidth * 0.5f);
    uniform.split = strokeWidth < 0.000001f ? 0 : edge;
    uniform.outline.make(edge - strokeWidth, edge - strokeWidth);

    uniform.colorIn = style.fillColor();
    uniform.colorOut = style.strokeColor();

    uniform.colorIn.w *= opacity();
    uniform.colorOut.w *= opacity();

    drawTextImpl(layout, location, Nimble::Vector2f(0, 0), viewRect, style, uniform, fontShader(), model);
  }

  void RenderContext::drawTextImpl(const TextLayout & layout, const Nimble::Vector2f & location,
                                   const Nimble::Vector2f & renderOffset,
                                   const Nimble::Rectf & viewRect, const TextStyle & style,
                                   FontUniformBlock & uniform, const Program & program,
                                   const Nimble::Matrix4f & modelview)
  {
    const int maxGlyphsPerCmd = 1000;

    std::map<QByteArray, const Texture *> textures = style.fill().textures();
    DepthMode d;
    d.setFunction(DepthMode::LESS_EQUAL);
    setDepthMode(d);

    Nimble::Matrix4f m;
    m.identity();


    Nimble::Vector2f renderLocation = layout.renderLocation() - viewRect.low() + renderOffset;

    // Need to check here that we are using correct texture atlas
    layout.check();
    for (int g = 0; g < layout.groupCount(); ++g) {
      textures["tex"] = layout.texture(g);

      auto & items = layout.items(g);

      for (int i = 0; i < int(items.size());) {
        const int count = std::min<int>(items.size() - i, maxGlyphsPerCmd);

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
          b.uniform->clip.move(-layout.renderLocation() - renderOffset);
        }

        m.setTranslation(offset);
        b.uniform->modelMatrix = (modelview * m).transpose();

        int index = 0;

        const int first = i;
        for (const int m = count + i; i < m; ++i) {
          auto & item = items[i];
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

    // Restore depth mode
    setDepthMode(DepthMode::Default());
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

  Nimble::Vector2 RenderContext::contextSize() const
  {
    return m_data->contextSize();
  }

  static RADIANT_TLS(RenderContext *) t_threadContext;

  void RenderContext::setThreadContext(RenderContext * rsc)
  {
    t_threadContext = rsc;
  }

  RenderContext * RenderContext::getThreadContext()
  {
    if(!t_threadContext) {
      Radiant::debug("No OpenGL resources for current thread");
      return nullptr;
    }

    return t_threadContext;
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

/*
  RenderTargetObject RenderContext::pushRenderTarget(Nimble::Vector2 size, float scale) {
    return m_data->m_rtm.pushRenderTarget(size, scale);
  }

  Luminous::Texture2D & RenderContext::popRenderTarget(RenderTargetObject & trt) {
    return m_data->m_rtm.popRenderTarget(trt);
  }
*/
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

  BufferGL & RenderContext::handle(const Buffer & buffer)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->handle(buffer);
  }

  RenderTargetGL & RenderContext::handle(const RenderTarget & target)
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

  //////////////////////////////////////////////////////////////////////////
  // Deferred mode API
  // 
  // All these commands generate a RenderCommand that can be reordered
  //
  //////////////////////////////////////////////////////////////////////////
  void RenderContext::clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil)
  {
    m_data->m_driver.clear(mask, color, depth, stencil);
  }

  const Program & RenderContext::basicShader() const
  {
    return m_data->m_basicShader;
  }

  const Program & RenderContext::texShader() const
  {
    return m_data->m_texShader;
  }

  const Program & RenderContext::fontShader() const
  {
    return m_data->m_fontShader;
  }

  int RenderContext::uniformBufferOffsetAlignment() const
  {
    return m_data->m_uniformBufferOffsetAlignment;
  }

  RenderTargetGuard RenderContext::pushRenderTarget(const RenderTarget &target)
  {
    m_data->m_driverGL->pushRenderTarget(target);

    m_data->m_currentRenderTarget = &target;

    // Push new projection matrix
    pushViewTransform(Nimble::Matrix4::ortho3D(0.f, target.size().width(), 0.f, target.size().height(), -1.f, 1.f));

    // Reset transformation matrix to identity
    pushTransform();
    setTransform(Nimble::Matrix4::IDENTITY);

    const Nimble::Recti viewport(0, 0, target.size().width(), target.size().height());

    // Push a scissor area that is the size of the render target. This is done
    // because the currently defined scissor area might be smaller the viewport
    // defined by the render target.
    pushScissorRect(viewport);

    // Push viewport
    pushViewport(viewport);

    // Reset the render call count for this target
    m_data->m_renderCalls.push(0);

    return RenderTargetGuard(*this);
  }

  void RenderContext::popRenderTarget()
  {
    // Restore viewport
    popViewport();

    // Restore scissor area
    popScissorRect();

    // Restore the matrix stack
    popTransform();
    popViewTransform();

    m_data->m_renderCalls.pop();
    m_data->m_driverGL->popRenderTarget();
  }

  void RenderContext::beginFrame()
  {
    if(m_data->m_postProcessFilters) {
      m_data->createPostProcessFilters(*this, *m_data->m_postProcessFilters);
      // Reorders the chain is necessary
      m_data->m_postProcessChain.prepare();
    }

    pushClipStack();

    assert(stackSize() == 1);

    m_data->m_driver.preFrame();

    // Push the render target for drawing the scene. Use an off-screen render
    // target if we have post-process filters. Don't use the RenderContext API
    // to avoid the guard.
    const Luminous::RenderTarget & renderTarget =
        m_data->m_postProcessChain.numEnabledFilters() > 0 ?
          m_data->m_defaultOffScreenRenderTarget :
          m_data->defaultRenderTarget();

    assert(renderTarget.targetType() != RenderTarget::INVALID);
    m_data->m_driverGL->pushRenderTarget(renderTarget);
    m_data->m_currentRenderTarget = &renderTarget;

    // Push default opacity
    assert(m_data->m_opacityStack.empty());
    m_data->m_opacityStack.push(1.f);
  }

  void RenderContext::endFrame()
  {
    if(!m_data->m_window->directRendering()) {
      // Push window render target
      m_data->m_defaultRenderTarget.setTargetBind(RenderTarget::BIND_DRAW);
      m_data->m_driverGL->pushRenderTarget(m_data->m_defaultRenderTarget);
      m_data->m_currentRenderTarget = &m_data->m_defaultRenderTarget;

      // Blit individual areas (from currently bound FBO)
      for(size_t i = 0; i < m_data->m_window->areaCount(); i++) {
        const Luminous::MultiHead::Area & area = m_data->m_window->area(i);
        blit(area.viewport(), area.viewport());
      }
    }

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
    m_data->m_driverGL->popRenderTarget();

    assert(stackSize() == 1);

    assert(m_data->m_clipStacks.size() == 1);

    popClipStack();
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
      const MultiHead::Area & area = m_data->m_window->area(i);

      if(area.isSoftwareColorCorrection()) {
        Radiant::info("Enabling software color correction for area %lu", i);

        // Check if filter already exists
        if(!m_data->m_postProcessChain.hasFilterType<ColorCorrectionFilter>()) {

          auto filter = std::make_shared<Luminous::ColorCorrectionFilter>();
          filter->setOrder(PostProcessChain::Color_Correction);

          auto context = std::make_shared<Luminous::PostProcessContext>(filter);

          context->initialize(*this);
          m_data->m_postProcessChain.insert(context);
        }
      }
    }
  }

  void RenderContext::postProcess()
  {
    PostProcessChain & chain = m_data->m_postProcessChain;
    const unsigned numFilters = chain.numEnabledFilters();

    if(numFilters == 0)
      return;

    const Nimble::Recti viewport(Nimble::Vector2i(0, 0), contextSize().cast<int>());

    // Copy off-screen buffers to use as the source of the first filter.
    // This is done because the off-screen target contains multisampled depth
    // and color buffers and usually filters are only interested in resolved
    // color data. By blitting to an FBO that only contains a non-multisampled
    // color buffer (default) the multisample resolution happens automatically.
    PostProcessContextPtr first = *chain.begin();
    first->renderTarget().setTargetBind(RenderTarget::BIND_DRAW);
    {
      auto g = pushRenderTarget(first->renderTarget());
      blit(viewport, viewport);
    }
    first->renderTarget().setTargetBind(RenderTarget::BIND_DEFAULT);

    // Set viewport to context size
    pushViewport(viewport);
    pushScissorRect(viewport);

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

      // If this is the last filter, use the default render target,
      // otherwise use the auxiliary off-screen render target of the next filter
      const RenderTarget & renderTarget = isLast ?
            m_data->defaultRenderTarget() :
            next->renderTarget();

      // Push the next auxilary render target
      auto g = pushRenderTarget(renderTarget);

      // Run each area through the filter
      for(unsigned j = 0; j < m_data->m_window->areaCount(); j++) {
        const MultiHead::Area & area = m_data->m_window->area(j);

        m_data->m_driver.setViewport(viewport);
        m_data->m_driver.setScissor(area.viewport());

        // Sets the current area to be rendered
        setArea(&area);

        ppf->doFilter(*this);
      }
    }

    // Remember to restore the viewport
    popScissorRect();
    popViewport();
  }

  bool RenderContext::initialize()
  {
    m_data->initialize();

    return true;
  }

  RenderContext::OpacityGuard RenderContext::pushOpacity(float opacity)
  {
    auto value = 1.f;

    if(!m_data->m_opacityStack.empty())
      value = m_data->m_opacityStack.top();

    m_data->m_opacityStack.push(value * opacity);

    return OpacityGuard(*this);
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
    m_data->m_driverGL->setStencilMode(mode);
  }

  void RenderContext::setCullMode(const CullMode mode)
  {
    m_data->m_driverGL->setCullMode(mode);
  }

  void RenderContext::setFrontFace(FaceWinding winding)
  {
    m_data->m_driverGL->setFrontFace(winding);
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

#if defined(RADIANT_OSX)
# include <DummyOpenGL.hpp>
#endif

  CustomOpenGL::CustomOpenGL(RenderContext & r, bool reset)
    : m_r(r)
  {
    // First, flush the current deferred render queues
    r.flush();

    if(reset) {
      glPointSize(1.f);
      glLineWidth(1.f);
      glUseProgram(0);
      glDisable(GL_DEPTH_TEST);
      glBindRenderbuffer(GL_RENDERBUFFER, 0);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
  }

  CustomOpenGL::~CustomOpenGL()
  {
    m_r.setDefaultState();
  }


}
