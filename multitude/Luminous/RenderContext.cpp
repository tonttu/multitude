/* COPYRIGHT
 */

#include "RenderContext.hpp"

#include "OpenGL/Error.hpp"
#include "Texture.hpp"
#include "FramebufferObject.hpp"

#include "Utils.hpp"
#include "RenderCommand.hpp"
#include "GLSLProgramObject.hpp"

#include <Nimble/ClipStack.hpp>

// Luminous v2
#include "Luminous/VertexArray.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/Buffer.hpp"
#include "Luminous/OpenGL/RenderDriverGL.hpp"
#include "Luminous/Text/SimpleTextLayout.hpp"

#include <Nimble/Matrix4.hpp>

#include <Radiant/Mutex.hpp>
#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Thread.hpp>

#include <strings.h>
#include <tuple>
#include <vector>

#define DEFAULT_RECURSION_LIMIT 8
#define SHADER(str) #str

namespace Luminous
{
  using namespace Nimble;
  using namespace Radiant;


  RenderContext::FBOPackage::~FBOPackage()
  {}

  void RenderContext::FBOPackage::setSize(Nimble::Vector2i size)

  {
    if(size == m_tex.size())
      return;

    GLint textureId = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureId);

    m_tex.bind();
    m_tex.setWidth(size.x);
    m_tex.setHeight(size.y);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // <- essential on Nvidia

    glBindTexture(GL_TEXTURE_2D, textureId);
  }

  void RenderContext::FBOPackage::attach()
  {
    m_fbo.attachTexture2D(&m_tex, GL_COLOR_ATTACHMENT0, 0);
    m_fbo.check();
  }

  void RenderContext::FBOPackage::activate(RenderContext & r)
  {
    LUMINOUS_IN_FULL_OPENGL(glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT));

    attach();
    r.pushDrawBuffer(Luminous::COLOR0, this);
    // Save and setup viewport to match the FBO
    glViewport(0, 0, m_tex.width(), m_tex.height());
    // error("RenderContext::FBOPackage::activate # unimplemented");
    r.pushViewTransform(Nimble::Matrix4::ortho3D(0, m_tex.width(), 0, m_tex.height(), -1, 1));
    /*
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(0, m_tex.width(), 0, m_tex.height(), -1, 1);
    */
  }

  void RenderContext::FBOPackage::deactivate(RenderContext & r)
  {
    Luminous::glErrorToString(__FILE__, __LINE__);
    m_fbo.unbind();
    Luminous::glErrorToString(__FILE__, __LINE__);
    glPopAttrib();

    Luminous::glErrorToString(__FILE__, __LINE__);
    r.popDrawBuffer();
    Luminous::glErrorToString(__FILE__, __LINE__);
    // Restore matrix stack
    /*
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    */
    r.popViewTransform();
    Luminous::glErrorToString(__FILE__, __LINE__);
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  RenderContext::FBOHolder::FBOHolder()
      : m_context(0),
      m_texUV(1,1)
  {
  }

  RenderContext::FBOHolder::FBOHolder(RenderContext * context, std::shared_ptr<FBOPackage> package)
      : m_context(context), m_package(package),
      m_texUV(1,1)
  {
    m_package->m_users++;
  }

  RenderContext::FBOHolder::FBOHolder(const FBOHolder & that)
  {
    FBOHolder * that2 = (FBOHolder *) & that;
    m_context = that2->m_context;
    m_package = that2->m_package;
    m_texUV   = that2->m_texUV;
    m_package->m_users++;
  }

  RenderContext::FBOHolder::~FBOHolder()
  {
    release();
  }

  /** Copies the data pointers from the argument object. */
  RenderContext::FBOHolder & RenderContext::FBOHolder::operator =
      (const RenderContext::FBOHolder & that)
  {
    release();
    FBOHolder * that2 = (FBOHolder *) & that;
    m_context = that2->m_context;
    m_package = that2->m_package;
    m_texUV   = that2->m_texUV;
    m_package->m_users++;
    return *this;
  }


  Luminous::Texture2D * RenderContext::FBOHolder::finish()
  {
    if(!m_package)
      return 0;

    Luminous::Texture2D * tex = & m_package->m_tex;

    release();

    return tex;
  }

  void RenderContext::FBOHolder::release()
  {
    if(m_package) {
      m_package->m_users--;

      if(!m_package->m_users) {
        m_context->clearTemporaryFBO(m_package);
      }

      m_package.reset();
      m_context = 0;
    }
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  struct RenderContext::SharedBuffer
  {
    SharedBuffer(Buffer::Type type) : buffer(type), reservedBytes(0) {}
    SharedBuffer(SharedBuffer && shared)
      : buffer(std::move(shared.buffer)),
        reservedBytes(shared.reservedBytes)
    {}
    SharedBuffer & operator=(SharedBuffer && shared)
    {
      buffer = std::move(shared.buffer);
      reservedBytes = shared.reservedBytes;
      return *this;
    }

    Buffer buffer;
    std::size_t reservedBytes;
  };

  class RenderContext::Internal
  {
  public:
    enum { MAX_TEXTURES = 64 };

    Internal(RenderDriver & renderDriver, const Luminous::MultiHead::Window * win)
        : m_recursionLimit(DEFAULT_RECURSION_LIMIT)
        , m_recursionDepth(0)
        , m_renderCount(0)
        , m_frameCount(0)
        , m_area(0)
        , m_window(win)
        , m_viewStackPos(-1)
        , m_boundProgram(0)
        , m_initialized(false)
        , m_program(0)
        , m_vbo(0)
        , m_uniformBufferOffsetAlignment(0)
        , m_automaticDepthDiff(-1.0f/100000.0f)
        , m_driver(renderDriver)
        , m_driverGL(dynamic_cast<RenderDriverGL*>(&renderDriver))
        , m_defaultRenderTarget(RenderTarget::WINDOW)
        , m_currentRenderTarget(0)
    {
      // Reset render call count
      m_renderCalls.push(0);

      // Initialize default render target size
      assert(win);
      m_defaultRenderTarget.setSize(Nimble::Size(win->size().x, win->size().y));

      memset(m_textures, 0, sizeof(m_textures));

      m_basicShader.loadShader("Luminous/GLSL150/basic.vs", ShaderGLSL::Vertex);
      m_basicShader.loadShader("Luminous/GLSL150/basic.fs", ShaderGLSL::Fragment);
      Luminous::VertexDescription desc;
      desc.addAttribute<Nimble::Vector3f>("vertex_position");
      m_basicShader.setVertexDescription(desc);

      m_texShader.loadShader("Luminous/GLSL150/tex.vs", ShaderGLSL::Vertex);
      m_texShader.loadShader("Luminous/GLSL150/tex.fs", ShaderGLSL::Fragment);
      desc = Luminous::VertexDescription();
      desc.addAttribute<Nimble::Vector3f>("vertex_position");
      desc.addAttribute<Nimble::Vector2>("vertex_uv");
      m_texShader.setVertexDescription(desc);

      m_fontShader.loadShader("Luminous/GLSL150/distance_field.vs", ShaderGLSL::Vertex);
      m_fontShader.loadShader("Luminous/GLSL150/distance_field.fs", ShaderGLSL::Fragment);
      desc = Luminous::VertexDescription();
      desc.addAttribute<Nimble::Vector3f>("vertex_position");
      desc.addAttribute<Nimble::Vector2>("vertex_uv");
      m_fontShader.setVertexDescription(desc);
    }

    ~Internal()
    {
    }

    void pushFBO(std::shared_ptr<RenderContext::FBOPackage> fbo)
    {
      m_fboStack.push(fbo);
    }

    std::shared_ptr<RenderContext::FBOPackage> popFBO()
    {
      m_fboStack.pop();

      return m_fboStack.empty() ? std::shared_ptr<RenderContext::FBOPackage>() : m_fboStack.top();
    }

    void initialize() {

      assert(m_window != 0);

      if(!m_initialized) {
        m_initialized = true;

        m_viewFBO.reset(new Luminous::Framebuffer());

        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniformBufferOffsetAlignment);
        if(m_uniformBufferOffsetAlignment < 1) {
          Radiant::error("RenderContext::Internal # Couldn't get GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, assuming 256");
          m_uniformBufferOffsetAlignment = 256;
        }

        info("RenderContext::Internal # init ok");
      }

      memset(m_textures, 0, sizeof(m_textures));
      m_program = 0;

      while(!m_drawBufferStack.empty())
        m_drawBufferStack.pop();
      m_drawBufferStack.push(DrawBuf(GL_BACK, 0)); // Start off by rendering to the back buffer
    }

    Nimble::Vector2f contextSize() const
    {
      if(m_window)
        return Nimble::Vector2f(m_window->size().x, m_window->size().y);

      /// @todo why not zero vector?
      return Nimble::Vector2f(10, 10);
    }

    void pushViewStack()
    {
      int w = m_window->size().x;
      int h = m_window->size().y;
      ++m_viewStackPos;
      if ((int) m_viewTextures.size() == m_viewStackPos) {
        m_viewTextures.push_back(new Luminous::Texture2D);
        Luminous::Texture2D & tex = *m_viewTextures.back();
        tex.setWidth(w);
        tex.setHeight(h);
        tex.bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
      }
      attachViewTexture();
      glPushAttrib(GL_VIEWPORT_BIT);
      glViewport(0, 0, w, h);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
    }
    void popViewStack()
    {
      glPopAttrib();
      --m_viewStackPos;
      // if wasn't last
      if (m_viewStackPos >= 0) {
        attachViewTexture();
      } else {
        unattachViewTexture();
      }
      assert(m_viewStackPos >= -1);
      glEnable(GL_TEXTURE_2D);
      m_viewTextures[m_viewStackPos+1]->bind();
    }

    void attachViewTexture()
    {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // <- essential on Nvidia

      m_viewFBO->attachTexture2D(m_viewTextures[m_viewStackPos], Luminous::COLOR0);
      m_viewFBO->check();
      // attachTexture2D should do this as a side effect already?
      glDrawBuffer(Luminous::COLOR0);
    }
    void unattachViewTexture() {
      m_viewFBO->unbind();
      glDrawBuffer(GL_BACK);
    }

    size_t m_recursionLimit;
    size_t m_recursionDepth;

    std::stack<Nimble::ClipStack> m_clipStacks;

    typedef std::list<std::shared_ptr<RenderContext::FBOPackage> > FBOPackages;

    FBOPackages m_fbos;


    std::stack<std::shared_ptr<RenderContext::FBOPackage>, std::vector<std::shared_ptr<RenderContext::FBOPackage> > > m_fboStack;

    class DrawBuf
    {
    public:
      DrawBuf() : m_fbo(0), m_dest(GL_BACK) {}
      DrawBuf(GLenum dest, RenderContext::FBOPackage * fbo) : m_fbo(fbo), m_dest(dest) {}
      RenderContext::FBOPackage * m_fbo;
      GLenum m_dest;
    };

    std::stack<DrawBuf, std::vector<DrawBuf> > m_drawBufferStack;

    unsigned long m_renderCount;
    unsigned long m_frameCount;

    const Luminous::MultiHead::Area * m_area;
    const Luminous::MultiHead::Window * m_window;
    /// fbo for views
    std::shared_ptr<Luminous::Framebuffer> m_viewFBO;
    /// fbo texture stack for views
    std::vector<Luminous::Texture2D *> m_viewTextures;
    int m_viewStackPos;

    Transformer m_viewTransformer;

    GLSLProgramObject * m_boundProgram;

    bool m_initialized;

    /// Viewports defined as x1,y1,x2,y2
    typedef std::stack<Nimble::Recti, std::vector<Nimble::Recti> > ViewportStack;
    ViewportStack m_viewportStack;
    //RenderTargetManager m_rtm;

    /// Cache for vertex array objects used in sharedbuffer rendering
    // key is <vertex buffer id, shader>
    std::map<std::tuple<RenderResource::Id, RenderResource::Id, ProgramGL*>, VertexArray> m_vertexArrayCache;

    // List of currently active textures, vbos etc.
    GLenum m_textures[MAX_TEXTURES];
    GLSLProgramObject * m_program;
    GLuint              m_vbo;

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
      BufferPool() : currentIndex(0) {}
      std::vector<SharedBuffer> buffers;
      int currentIndex;

      BufferPool(BufferPool && pool)
        : buffers(std::move(pool.buffers))
        , currentIndex(pool.currentIndex)
      {}

      BufferPool & operator=(BufferPool && pool)
      {
        buffers = std::move(pool.buffers);
        currentIndex = pool.currentIndex;
        return *this;
      }

      void flush(RenderContext & ctx)
      {
        currentIndex = 0;
        for(auto it = buffers.begin(); it != buffers.end(); ++it) {
          if (it->reservedBytes > 0) {
            ctx.unmapBuffer(it->buffer, 0, it->reservedBytes);
            it->reservedBytes = 0;
          }
        }
      }
    };

    // vertex/uniform struct size -> pool
    std::map<std::size_t, BufferPool> m_vertexBuffers;
    std::map<std::size_t, BufferPool> m_uniformBuffers;
    BufferPool m_indexBuffers;

    // Default window framebuffer
    RenderTarget m_defaultRenderTarget;
    const RenderTarget * m_currentRenderTarget;

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
         m_data->m_renderCount / Nimble::Math::Max(m_data->m_frameCount, (unsigned long) 1));
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
    return m_data->m_viewTransformer.transform4();
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

  void RenderContext::pushDrawBuffer(GLenum dest, FBOPackage * fbo)
  {
    if(m_data->m_drawBufferStack.size() > 1000) {
      error("RenderContext::pushDrawBuffer # Stack is very deep %d",
            (int) m_data->m_drawBufferStack.size());
    }

    m_data->m_drawBufferStack.push(Internal::DrawBuf(dest, fbo));
    glDrawBuffer(dest);
  }

  void RenderContext::popDrawBuffer()
  {
    if(m_data->m_drawBufferStack.empty()) {
      error("RenderContext::popDrawBuffer # empty stack");
      glDrawBuffer(GL_BACK);
      return;
    }
    m_data->m_drawBufferStack.pop();

    if(m_data->m_drawBufferStack.empty()) {
      error("RenderContext::popDrawBuffer # empty stack (phase 2)");
      glDrawBuffer(GL_BACK);
      return;
    }
    Internal::DrawBuf buf = m_data->m_drawBufferStack.top();
    // info("DrawBuffer = %d %x", (int) buf.m_dest, (int) buf.m_dest);
    if(buf.m_fbo)
      buf.m_fbo->attach();

    glDrawBuffer(buf.m_dest);
  }

  void RenderContext::drawArc(const Nimble::Vector2f & center, float radius,
                              float fromRadians, float toRadians, Luminous::Style & style, unsigned int linesegments)
  {
    if (linesegments == 0) {
      /// @todo Automagically determine the proper number of linesegments
      linesegments = 32;
    }

    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, 0, linesegments + 1,
      program, style.strokeColor(), style.strokeWidth(), style);

    float step = (toRadians - fromRadians) / linesegments;

    float angle = fromRadians;
    for (unsigned int i = 0; i <= linesegments; ++i) {
      Nimble::Vector2f c(std::cos(angle), std::sin(angle));
      b.vertex[i].location = Nimble::Vector3f(center + c * radius, 0.f);
      angle += step;
    }
  }

  void RenderContext::drawCircle(const Nimble::Vector2f & center, float radius, Luminous::Style & style, unsigned int linesegments, float fromRadians, float toRadians)
  {
    if (linesegments == 0) {
      /// @todo Automagically determine the proper number of linesegments
      linesegments = 32;
    }

    // Filler function: Generates vertices in a circle
    auto fill = [=](BasicVertex * vertices, float depth) {
      float step = (toRadians - fromRadians) / linesegments;

      // Add the rest of the fan vertices
      float angle = fromRadians;
      for (unsigned int i = 0; i <= linesegments; ++i) {
        Nimble::Vector2f c(std::cos(angle), std::sin(angle));
        vertices[i].location = Nimble::Vector3f(center + c * radius, depth);
        angle += step;
      }
    };

    // Draw fill
    if (style.fillColor().w > 0.f) {
      const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
      auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_TriangleFan, 0, linesegments + 2, program, style.fillColor(), 1.f, style);
      // Center is the first vertex in a fan
      b.vertex[0].location.make(center.x, center.y, 0.f);
      // Create the rest of the vertices
      fill(&b.vertex[1], b.depth);
    }

    // Draw stroke
    if (style.strokeWidth() > 0.f) {
      const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
      auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, 0, linesegments+1, program, style.strokeColor(), style.strokeWidth(), style);
      fill(b.vertex, b.depth);
    }
  }

  void RenderContext::drawDonut(const Nimble::Vector2f & center,
                                float majorAxisLength,
                                float minorAxisLength,
                                float width,
                                const Luminous::Style & style,
                                unsigned int linesegments,
                                float fromRadians, float toRadians)
  {
    if(linesegments == 0) {
      /// @todo automagically determine divisions?
      linesegments = 32;
    }

    bool stroke = style.strokeWidth() > 0.0f;
    bool needInnerStroke = std::min(majorAxisLength, minorAxisLength) - width > 0.0f;

    majorAxisLength -= style.strokeWidth();
    minorAxisLength -= style.strokeWidth();

    const float step = (toRadians - fromRadians) / (linesegments-1);

    float angle = fromRadians;

    float r = 0.5f * width;
    float m1 = majorAxisLength - r;
    float m2 = minorAxisLength - r;

    float strokeRatio = (r + 0.5f*style.strokeWidth()) / r;

    float maxLength = Nimble::Math::Max(majorAxisLength, minorAxisLength);
    float iSpan = 1.0f/(2.0f*r);
    Nimble::Vector2f low = center - Nimble::Vector2(maxLength, maxLength);

    RenderBuilder<BasicVertex, BasicUniformBlock> fill;
    RenderBuilder<BasicVertexUV, BasicUniformBlock> textured;
    RenderBuilder<BasicVertex, BasicUniformBlock> innerStroke;
    RenderBuilder<BasicVertex, BasicUniformBlock> outerStroke;

    /// Generate the fill builders
    bool isTextured = false;
    if (style.fillColor().w > 0.f) {
      if (style.fill().textures().empty()) {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        fill = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, 0, linesegments * 2, program, style.fillColor(), 1.f, style);
      }
      else {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        textured = drawPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, 0, linesegments * 2, program, style.fillColor(), 1.f, style);
        isTextured = true;
      }
    }

    /// Generate the stroke builders
    if(stroke) {
      const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
      if(needInnerStroke)
        innerStroke = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, 0, linesegments, program, style.strokeColor(), style.strokeWidth(), style);
      outerStroke = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, 0, linesegments, program, style.strokeColor(), style.strokeWidth(), style);
    }

    /// Generate the vertex data
    for (unsigned int i = 0; i < linesegments; ++i) {
      Nimble::Vector2f p = Nimble::Vector2(std::cos(angle), std::sin(angle));
      Nimble::Vector2f normal = Nimble::Vector2(-m1*p.y, m2*p.x).perpendicular().normalize(r);

      p.x *= m1;
      p.y *= m2;
      p += center;

      Nimble::Vector2f in = p + normal;
      Nimble::Vector2f out = p - normal;

      if (isTextured) {
        textured.vertex[2*i].location.make(in, textured.depth);
        textured.vertex[2*i+1].location.make(out, textured.depth);
        textured.vertex[i].texCoord = iSpan * (in-low);
        textured.vertex[i].texCoord = iSpan * (out-low);
      }
      else {
        fill.vertex[2*i].location.make(p + normal, fill.depth);
        fill.vertex[2*i+1].location.make(p - normal, fill.depth);
      }

      if (stroke) {
        if (needInnerStroke)
          innerStroke.vertex[i].location.make(p + strokeRatio*normal, innerStroke.depth);
        outerStroke.vertex[i].location.make(p - strokeRatio*normal, outerStroke.depth);
      }
      angle += step;
    }

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

  std::pair<void *, RenderContext::SharedBuffer *> RenderContext::sharedBuffer(
      std::size_t vertexSize, std::size_t maxVertexCount, Buffer::Type type, unsigned int & offset)
  {
    Internal::BufferPool & pool = type == Buffer::Index
        ? m_data->m_indexBuffers
        : type == Buffer::Vertex
          ? m_data->m_vertexBuffers[vertexSize]
          : m_data->m_uniformBuffers[vertexSize];

    const std::size_t requiredBytes = vertexSize * maxVertexCount;

    SharedBuffer * buffer = nullptr;
    std::size_t nextSize = 1 << 20;
    for(;;) {
      if(pool.currentIndex >= pool.buffers.size()) {
        pool.buffers.emplace_back(type);
        buffer = &pool.buffers.back();
        buffer->buffer.setData(nullptr, std::max(requiredBytes, nextSize), Buffer::StreamDraw);
        break;
      }

      buffer = &pool.buffers[pool.currentIndex];
      if(buffer->buffer.size() - buffer->reservedBytes >= requiredBytes)
        break;

      nextSize = buffer->buffer.size() << 1;
      ++pool.currentIndex;
    }

    char * data = mapBuffer<char>(buffer->buffer, Buffer::MapWrite | Buffer::MapUnsynchronized |
                                  Buffer::MapInvalidateRange | Buffer::MapFlushExplicit);
    assert(data);
    data += buffer->reservedBytes;
    offset = buffer->reservedBytes / vertexSize;
    buffer->reservedBytes += requiredBytes;
    return std::make_pair(data, buffer);
  }

  template <>
  void * RenderContext::mapBuffer<void>(const Buffer & buffer, int offset, std::size_t length,
                                        Radiant::FlagsT<Buffer::MapAccess> access)
  {
    return m_data->m_driver.mapBuffer(buffer, offset, length, access);
  }

  void RenderContext::unmapBuffer(const Buffer & buffer, int offset, std::size_t length)
  {
    m_data->m_driver.unmapBuffer(buffer, offset, length);
  }


  // Create a render command using the shared buffers
  RenderCommand & RenderContext::createRenderCommand(bool translucent,
                                                     int indexCount, int vertexCount,
                                                     std::size_t vertexSize, std::size_t uniformSize,
                                                     unsigned *& mappedIndexBuffer,
                                                     void *& mappedVertexBuffer,
                                                     void *& mappedUniformBuffer,
                                                     float & depth,
                                                     const Program & shader,
                                                     const std::map<QByteArray,const Texture *> & textures)
  {
    unsigned int indexOffset, vertexOffset, uniformOffset;

    // Align uniforms as required by OpenGL
    uniformSize = Nimble::Math::Ceil(uniformSize / float(m_data->m_uniformBufferOffsetAlignment)) *
        m_data->m_uniformBufferOffsetAlignment;

    SharedBuffer * ibuffer;
    std::tie(mappedIndexBuffer, ibuffer) = sharedBuffer<unsigned int>(indexCount, Buffer::Index, indexOffset);

    SharedBuffer * vbuffer;
    std::tie(mappedVertexBuffer, vbuffer) = sharedBuffer(vertexSize, vertexCount, Buffer::Vertex, vertexOffset);

    SharedBuffer * ubuffer;
    std::tie(mappedUniformBuffer, ubuffer) = sharedBuffer(uniformSize, 1, Buffer::Uniform, uniformOffset);

    // Get the matching vertexarray from cache or create a new one if needed
    const auto key = std::make_tuple(vbuffer->buffer.resourceId(), ibuffer->buffer.resourceId(), &handle(shader));

    auto it = m_data->m_vertexArrayCache.find(key);
    if(it == m_data->m_vertexArrayCache.end()) {
      // No array yet for this combination: Create a new vertexarray
      VertexArray vertexArray;
      vertexArray.addBinding(vbuffer->buffer, shader.vertexDescription());
      if (indexCount > 0)
        vertexArray.setIndexBuffer(ibuffer->buffer);

      it = m_data->m_vertexArrayCache.insert(std::make_pair(key, std::move(vertexArray))).first;
    }

    RenderCommand & cmd = m_data->m_driver.createRenderCommand(
          translucent, it->second, ubuffer->buffer, shader, textures);
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
        auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, 0, 4, program, style.fillColor(), 1.f, style);
        b.vertex[0].location.make(rect.low(), b.depth);
        b.vertex[1].location.make(rect.highLow(), b.depth);
        b.vertex[2].location.make(rect.lowHigh(), b.depth);
        b.vertex[3].location.make(rect.high(), b.depth);
      }
      else {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        auto b = drawPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, 0, 4, program, style.fillColor(), 1.f, style);

        b.vertex[0].location.make(rect.low(), b.depth);
        b.vertex[0].texCoord = uvs.low();

        b.vertex[1].location.make(rect.highLow(), b.depth);
        b.vertex[1].texCoord = uvs.highLow();

        b.vertex[2].location.make(rect.lowHigh(), b.depth);
        b.vertex[2].texCoord = uvs.lowHigh();

        b.vertex[3].location.make(rect.high(), b.depth);
        b.vertex[3].texCoord = uvs.high();
      }
    }

    // Draw the outline
    if (style.strokeWidth() > 0.f && style.strokeColor().w > 0.f) {
      const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
      auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, 0, 5, program, style.strokeColor(), style.strokeWidth(), style);
      b.vertex[0].location.make(rect.low(), b.depth);
      b.vertex[1].location.make(rect.highLow(), b.depth);
      b.vertex[2].location.make(rect.high(), b.depth);
      b.vertex[3].location.make(rect.lowHigh(), b.depth);
      b.vertex[4].location.make(rect.low(), b.depth);
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
        auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, 0, 10, program, style.fillColor(), 1.f, style);
        b.vertex[0].location.make(hole.low(), b.depth);
        b.vertex[1].location.make(area.low(), b.depth);
        b.vertex[2].location.make(hole.highLow(), b.depth);
        b.vertex[3].location.make(area.highLow(), b.depth);
        b.vertex[4].location.make(hole.high(), b.depth);
        b.vertex[5].location.make(area.high(), b.depth);
        b.vertex[6].location.make(hole.lowHigh(), b.depth);
        b.vertex[7].location.make(area.lowHigh(), b.depth);
        b.vertex[8].location.make(hole.low(), b.depth);
        b.vertex[9].location.make(area.low(), b.depth);
      }
      else {
        // Textured
        /// @todo calculate correct UVs for the inside ring
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        auto b = drawPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, 0, 10, program, style.fillColor(), 1.f, style);

        b.vertex[0].location.make(hole.low(), b.depth);
        b.vertex[0].texCoord.make(0,0);

        b.vertex[1].location.make(area.low(), b.depth);
        b.vertex[1].texCoord.make(0,0);

        b.vertex[2].location.make(hole.highLow(), b.depth);
        b.vertex[2].texCoord.make(0,0);

        b.vertex[3].location.make(area.highLow(), b.depth);
        b.vertex[3].texCoord.make(1,0);

        b.vertex[4].location.make(hole.high(), b.depth);
        b.vertex[4].texCoord.make(0,0);

        b.vertex[5].location.make(area.high(), 0);
        b.vertex[5].texCoord.make(1,1);

        b.vertex[6].location.make(hole.lowHigh(), b.depth);
        b.vertex[6].texCoord.make(0,0);

        b.vertex[7].location.make(area.lowHigh(), b.depth);
        b.vertex[7].texCoord.make(0,1);

        b.vertex[8].location.make(hole.low(), b.depth);
        b.vertex[8].texCoord.make(0,0);

        b.vertex[9].location.make(area.low(), b.depth);
        b.vertex[9].texCoord.make(0,0);
      }
    }

    // Draw the stroke
    if (style.strokeWidth() > 0.f && style.strokeColor().w > 0.f) {
      const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
      { // Draw innerstroke
        auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, 0, 5, program, style.strokeColor(), style.strokeWidth(), style);
        b.vertex[0].location.make(hole.low(), b.depth);
        b.vertex[1].location.make(hole.highLow(), b.depth);
        b.vertex[2].location.make(hole.high(), b.depth);
        b.vertex[3].location.make(hole.lowHigh(), b.depth);
        b.vertex[4].location.make(hole.low(), b.depth);
      }
      
      { // Draw outerstroke
        auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, 0, 5, program, style.strokeColor(), style.strokeWidth(), style);
        b.vertex[0].location.make(area.low(), b.depth);
        b.vertex[1].location.make(area.highLow(), b.depth);
        b.vertex[2].location.make(area.high(), b.depth);
        b.vertex[3].location.make(area.lowHigh(), b.depth);
        b.vertex[4].location.make(area.low(), b.depth);
      }
    }
  }

  //
  void RenderContext::drawLine(const Nimble::Vector2f & p1, const Nimble::Vector2f & p2, const Luminous::Style & style)
  {
    assert(style.strokeWidth() > 0.f);
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, 0, 2, program, style.strokeColor(), style.strokeWidth(), style);
    b.vertex[0].location.make(p1,b.depth);
    b.vertex[1].location.make(p2,b.depth);
  }

  //
  void RenderContext::drawPolyLine(const Nimble::Vector2f * points, unsigned int numPoints, const Luminous::Style & style)
  {
    assert(style.strokeWidth() > 0.f);
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, 0, numPoints, program, style.strokeColor(), style.strokeWidth(), style);
    for (size_t i = 0; i < numPoints; ++i)
      b.vertex[i].location.make(points[i], b.depth);
  }

  void RenderContext::drawPoints(const Nimble::Vector2f * points, size_t numPoints, const Luminous::Style & style)
  {
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_Point, 0, numPoints, program, style.strokeColor(), style.strokeWidth(), style);
    for (size_t i = 0; i < numPoints; ++i)
      b.vertex[i].location.make(points[i], b.depth);
  }

  void RenderContext::drawText(const TextLayout & layout, const Nimble::Vector2f & location, const Style & style)
  {
    const int maxGlyphsPerCmd = 1000;

    std::map<QByteArray, const Texture *> textures = style.fill().textures();
    DepthMode d;
    d.setFunction(DepthMode::LessEqual);

    Nimble::Matrix4f m;
    m.identity();

    Nimble::Vector2f renderLocation = layout.renderLocation();

    for (int g = 0; g < layout.groupCount(); ++g) {
      textures["tex"] = layout.texture(g);

      auto & items = layout.items(g);

      for (int i = 0; i < items.size();) {
        const int count = std::min<int>(items.size() - i, maxGlyphsPerCmd);

        auto b = render<BasicVertexUV, FontUniformBlock>(
              true, PrimitiveType_TriangleStrip, count*6 - 2, count*4, 1, fontShader(), textures);
        b.uniform->color = style.fillColor();
        b.command->blendMode = style.blendMode();
        b.command->depthMode = d;
        b.command->stencilMode = style.stencilMode();

        m.setTranslation(Nimble::Vector3f(renderLocation.x + location.x, renderLocation.y + location.y, b.depth));
        b.uniform->modelMatrix = transform4() * m;
        b.uniform->modelMatrix.transpose();

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
  }

  void RenderContext::drawText(const QString & text, const Nimble::Rectf & rect,
                               const Style & style, TextFlags flags)
  {
    if (flags == TextStatic) {
      const SimpleTextLayout & layout = SimpleTextLayout::cachedLayout(text, rect.size(), style.font(), style.textOption());
      drawText(layout, rect.low(), style);
    } else {
      SimpleTextLayout layout(text, rect.size(), style.font(), style.textOption());
      layout.generate();
      drawText(layout, rect.low(), style);
    }
  }

  Nimble::Vector2 RenderContext::contextSize() const
  {
    return m_data->contextSize();
  }

  void RenderContext::pushViewStack()
  {
    m_data->pushViewStack();
  }

  void RenderContext::popViewStack()
  {
    m_data->popViewStack();
  }

  void RenderContext::clearTemporaryFBO(std::shared_ptr<FBOPackage> fbo)
  {
    assert(fbo->userCount() == 0);

    fbo->m_fbo.unbind();

    fbo = m_data->popFBO();

    if(fbo) {
      fbo->attach();
    }
    popDrawBuffer();

    glPopAttrib();

    // Restore matrix stack
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    popTransform();
  }

  static RADIANT_TLS(RenderContext *) t_threadContext;

  void RenderContext::setThreadContext(RenderContext * rsc)
  {
    t_threadContext = rsc;
  }

  RenderContext * RenderContext::getThreadContext()
  {
    if(!t_threadContext) {
      debug("No OpenGL resources for current thread");
      return nullptr;
    }

    return t_threadContext;
  }

  void RenderContext::flush2()
  {
    m_data->m_indexBuffers.flush(*this);

    for(auto it = m_data->m_vertexBuffers.begin(); it != m_data->m_vertexBuffers.end(); ++it)
      it->second.flush(*this);
    for(auto it = m_data->m_uniformBuffers.begin(); it != m_data->m_uniformBuffers.end(); ++it)
      it->second.flush(*this);

    m_data->m_driver.flush();
  }

  void RenderContext::restart()
  {
    m_data->m_program = 0;
    m_data->m_vbo = 0;

    memset(m_data->m_textures, 0, sizeof(m_data->m_textures));
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

    glViewport(viewport.low().x, viewport.low().y, viewport.width(), viewport.height());

    glScissor(viewport.low().x, viewport.low().y, viewport.width(), viewport.height());
  }

  void RenderContext::popViewport()
  {
    m_data->m_viewportStack.pop();

    if(!m_data->m_viewportStack.empty()) {
      const Nimble::Recti & viewport = currentViewport();
      glViewport(viewport.low().x, viewport.low().y, viewport.width(), viewport.height());
    }
  }

  const Nimble::Recti & RenderContext::currentViewport() const
  {
    return m_data->m_viewportStack.top();
  }

  //////////////////////////////////////////////////////////////////////////
  // Luminousv2

  void RenderContext::setBuffer(Buffer::Type type, const Luminous::Buffer & buffer)
  {
    switch (type)
    {
    case Buffer::Vertex: m_data->m_driver.setVertexBuffer(buffer); break;
    case Buffer::Index: m_data->m_driver.setIndexBuffer(buffer); break;
    case Buffer::Uniform: m_data->m_driver.setUniformBuffer(buffer); break;
    default:
      assert(false);
      Radiant::error("RenderContext::setBuffer - Buffertype %d not implemented", type);
    }
  }

  void RenderContext::setVertexArray(const VertexArray & vertexArray)
  {
    // Bind the VAO: Binds all the associated vertex buffers and sets the appropriate vertex attributes
    m_data->m_driver.setVertexArray(vertexArray);
  }

  void RenderContext::setShaderProgram(const Program & program)
  {
    m_data->m_driver.setShaderProgram(program);
  }

  void RenderContext::draw(PrimitiveType primType, unsigned int offset, unsigned int primitives)
  {
    m_data->m_driver.draw(primType, offset, primitives);
  }

  void RenderContext::drawIndexed(PrimitiveType primType, unsigned int offset, unsigned int primitives)
  {
    m_data->m_driver.drawIndexed(primType, offset, primitives);
  }

  void RenderContext::clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil)
  {
    m_data->m_driver.clear(mask, color, depth, stencil);
  }

  // Create all the setters for shader constants
#define SETSHADERUNIFORM(TYPE) \
  template<> LUMINOUS_API bool RenderContext::setShaderUniform(const char * name, const TYPE & value) \
  { \
    return m_data->m_driver.setShaderUniform(name, value); \
  }
  SETSHADERUNIFORM(int);
  SETSHADERUNIFORM(float);
  SETSHADERUNIFORM(Nimble::Vector2i);
  SETSHADERUNIFORM(Nimble::Vector3i);
  SETSHADERUNIFORM(Nimble::Vector4i);
  SETSHADERUNIFORM(Nimble::Vector2f);
  SETSHADERUNIFORM(Nimble::Vector3f);
  SETSHADERUNIFORM(Nimble::Vector4f);
  SETSHADERUNIFORM(Nimble::Matrix2f);
  SETSHADERUNIFORM(Nimble::Matrix3f);
  SETSHADERUNIFORM(Nimble::Matrix4f);
#undef SETSHADERUNIFORM

  // Manual conversion: Radiant::Color > Nimble::Vector4f
  template<> LUMINOUS_API bool RenderContext::setShaderUniform(const char * name, const Radiant::Color & value)
  {
    return m_data->m_driver.setShaderUniform(name, static_cast<Nimble::Vector4f>(value));
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

  ProgramGL & RenderContext::handle(const Program & program)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->handle(program);
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

    // Reset the render call count for this target
    m_data->m_renderCalls.push(0);

    return RenderTargetGuard(*this);
  }

  void RenderContext::popRenderTarget()
  {
    // Restore the matrix stack
    popTransform();
    popViewTransform();

    m_data->m_renderCalls.pop();
    m_data->m_driverGL->popRenderTarget();
  }

  void RenderContext::beginFrame()
  {
    pushClipStack();

    assert(stackSize() == 1);

    m_data->m_driver.preFrame();

    // Push the default render target. Don't use the RenderContext API to avoid
    // the guard.
    assert(m_data->m_defaultRenderTarget.targetType() != RenderTarget::INVALID);
    m_data->m_driverGL->pushRenderTarget(m_data->m_defaultRenderTarget);
    m_data->m_currentRenderTarget = &m_data->m_defaultRenderTarget;

    // Push default opacity
    assert(m_data->m_opacityStack.empty());
    m_data->m_opacityStack.push(1.f);
  }

  void RenderContext::endFrame()
  {
    flush2();

    m_data->m_driver.postFrame();

    /// @todo how do we generate this properly? Should we somehow linearize the depth buffer?
    m_data->m_automaticDepthDiff = -1.0f / std::max(m_data->m_renderCalls.top(), 100000);
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
    assert(transform4() == Nimble::Matrix4::IDENTITY);
  }

  void RenderContext::endArea()
  {
    assert(stackSize() == 1);
    assert(transform4() == Nimble::Matrix4::IDENTITY);
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

}
