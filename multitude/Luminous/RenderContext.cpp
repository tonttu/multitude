/* COPYRIGHT
 */

#include "RenderContext.hpp"

#include "OpenGL/Error.hpp"
#include "Texture.hpp"
#include "FramebufferObject.hpp"

#include "Utils.hpp"
#include "VertexHolder.hpp"
#include "GLSLProgramObject.hpp"

// Luminous v2
#include "Luminous/VertexArray.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/Buffer.hpp"
#include "Luminous/OpenGL/RenderDriverGL.hpp"

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
        , m_renderPacket(0)
        , m_renderCount(0)
        , m_frameCount(0)
        , m_area(0)
        , m_window(win)
        , m_viewStackPos(-1)
        , m_boundProgram(0)
        , m_initialized(false)
        , m_blendFunc(RenderContext::BLEND_USUAL)
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
      m_defaultRenderTarget.setSize(QSize(win->size().x, win->size().y));

      m_attribs.resize(10000);
      m_attribs.clear();

      m_verts.resize(10000);
      m_verts.clear();

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
    }

    ~Internal()
    {
      delete m_renderPacket;
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

        m_emptyTexture.reset(Texture2D::fromBytes(GL_RGB, 32, 32, 0,
                                                  Luminous::PixelFormat::rgbUByte(), false));

        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniformBufferOffsetAlignment);
        if(m_uniformBufferOffsetAlignment < 1) {
          Radiant::error("RenderContext::Internal # Couldn't get GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, assuming 256");
          m_uniformBufferOffsetAlignment = 256;
        }

        info("RenderContext::Internal # init ok");
      }

      memset(m_textures, 0, sizeof(m_textures));
      m_program = 0;

      if(!m_renderPacket)
        m_renderPacket = new RenderPacket();

      while(!m_drawBufferStack.empty())
        m_drawBufferStack.pop();
      m_drawBufferStack.push(DrawBuf(GL_BACK, 0)); // Start off by rendering to the back buffer

      m_emptyTexture->bind(GL_TEXTURE0);
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

    std::vector<Nimble::Rectangle> m_clipStack;

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

    /*
    class Vertex
    {
    public:
      Vertex() { bzero(this, sizeof (*this)); }
      Nimble::Vector2 m_location;
      Nimble::Vector4 m_color;
      Nimble::Vector2 m_texCoord;
      float           m_useTexture;
    };

    std::vector<Vertex> m_vertices;
    */

    RenderPacket * m_renderPacket;

    unsigned long m_renderCount;
    unsigned long m_frameCount;

    const Luminous::MultiHead::Area * m_area;
    const Luminous::MultiHead::Window * m_window;
    /// fbo for views
    std::shared_ptr<Luminous::Framebuffer> m_viewFBO;
    /// fbo texture stack for views
    std::vector<Luminous::Texture2D *> m_viewTextures;
    int m_viewStackPos;
    std::vector<Nimble::Vector2> m_attribs;
    std::vector<Nimble::Vector2> m_verts;
    std::vector<Nimble::Vector2> m_texcoords;
    std::vector<Nimble::Vector4> m_colors;

    Transformer m_viewTransformer;

    GLSLProgramObject * m_boundProgram;

    bool m_initialized;

    BlendFunc m_blendFunc;

    /// Viewports defined as x1,y1,x2,y2
    typedef std::stack<Nimble::Recti, std::vector<Nimble::Recti> > ViewportStack;
    ViewportStack m_viewportStack;
    //RenderTargetManager m_rtm;

    // List of currently active textures, vbos etc.
    GLenum m_textures[MAX_TEXTURES];
    GLSLProgramObject * m_program;
    GLuint              m_vbo;
    std::shared_ptr<Texture2D> m_emptyTexture;

    int m_uniformBufferOffsetAlignment;

    float m_automaticDepthDiff;
    // Stack of render call counts
    std::stack<int> m_renderCalls;

    Program m_basicShader;
    Program m_texShader;

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
    RenderTarget * m_currentRenderTarget;
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

  void RenderContext::pushClipRect(const Nimble::Rectangle & r)
  {
//      Radiant::info("RenderContext::pushClipRect # (%f,%f) (%f,%f)", r.center().x, r.center().y, r.size().x, r.size().y);

    m_data->m_clipStack.push_back(r);
  }

  void RenderContext::popClipRect()
  {
    m_data->m_clipStack.pop_back();
  }

  const std::vector<Nimble::Rectangle> & RenderContext::clipStack() const
  {
    return m_data->m_clipStack;
  }

  bool RenderContext::isVisible(const Nimble::Rectangle & area)
  {
    // Radiant::info("RenderContext::isVisible # area (%f,%f) (%f,%f)",
    // area.center().x, area.center().y, area.size().x, area.size().y);

      if(m_data->m_clipStack.empty()) {
        debugLuminous("\tclip stack is empty");
        return true;
      } else {

          /* Since we have no proper clipping algorithm, we compare against
             every clip rectangle in the stack*/
          bool inside = true;

          // Why does const_reverse_iterator not work on OSX :(
          for(std::vector<Nimble::Rectangle>::reverse_iterator it =
              m_data->m_clipStack.rbegin(); it != m_data->m_clipStack.rend(); it++) {
            inside &= (*it).intersects(area);
          }

          /*
          const Nimble::Rectangle & clipArea = m_data->m_clipStack.top();

          bool inside = m_data->m_clipStack.top().intersects(area);

          Radiant::info("\tclip area (%f,%f) (%f,%f) : inside %d",
          clipArea.center().x, clipArea.center().y, clipArea.size().x,
          clipArea.size().y, inside);
          */

          return inside;
      }
  }
/*
  const Nimble::Rectangle & RenderContext::visibleArea() const
  {
    return m_data->m_clipStack.back();
  }
*/

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


  RenderContext::FBOHolder RenderContext::getTemporaryFBO
      (Nimble::Vector2f basicsize, float scaling, uint32_t flags)
  {
    Nimble::Vector2f r = basicsize * scaling;

    Nimble::Vector2i minimumsize(r.x, r.y);

    /* First we try to find a reasonable available FBO, that is not more than
       100% too large.
    */

    long maxpixels = 2 * minimumsize.x * minimumsize.y;

    FBOHolder ret;
    std::shared_ptr<FBOPackage> fbo;

    for(Internal::FBOPackages::iterator it = m_data->m_fbos.begin();
    it != m_data->m_fbos.end(); it++) {
      fbo = *it;

      if(flags & FBO_EXACT_SIZE) {
        if(fbo->userCount() ||
           fbo->m_tex.width() != minimumsize.x ||
           fbo->m_tex.height() != minimumsize.y)
          continue;
      }
      else if(fbo->userCount() ||
              fbo->m_tex.width() < minimumsize.x ||
              fbo->m_tex.height() < minimumsize.y ||
              fbo->m_tex.pixelCount() > maxpixels)
        continue;

      ret = FBOHolder(this, fbo);
      break;
    }

    if(!ret.m_package) {
      // Nothing available, we need to create a new FBOPackage
      // info("Creating a new FBOPackage");
      fbo.reset(new FBOPackage());
      Vector2i useSize = minimumsize;
      if(!(flags & FBO_EXACT_SIZE))
        useSize += minimumsize / 4;
      fbo->setSize(useSize);
      m_data->m_fbos.push_back(std::shared_ptr<FBOPackage>(fbo));

      ret = FBOHolder(this, fbo);
    }

    /* We now have a valid FBO, next job is to set it up for rendering.
    */

    glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

    for(int i = 0; i < 6; i++)
      glDisable(GL_CLIP_PLANE0 + i);

    ret.m_package->attach();

    // Draw into color attachment 0
    pushDrawBuffer(Luminous::COLOR0, &*ret.m_package);

    // Save and setup viewport to match the FBO
    glViewport(0, 0, fbo->m_tex.width(), fbo->m_tex.height());
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, minimumsize.x, minimumsize.y);

    // Save matrix stack
    /*
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(0, minimumsize.x, 0, minimumsize.y, -1, 1);
    */
    pushViewTransform(Nimble::Matrix4::ortho3D(0, minimumsize.x, 0, minimumsize.y, -1, 1));

    m_data->pushFBO(ret.m_package);

    // Lets adjust the matrix stack to take into account the new
    // reality:
    pushTransform(Nimble::Matrix3::scale2D(
        minimumsize.x / basicsize.x,
        minimumsize.y / basicsize.y));

    ret.m_texUV.make(minimumsize.x / (float) fbo->m_tex.width(),
                     minimumsize.y / (float) fbo->m_tex.height());

    // info("texuv = %f %f", ret.m_texUV.x, ret.m_texUV.y);

    return ret;
  }

  void RenderContext::drawArc(Nimble::Vector2f center, float radius,
                              float fromRadians, float toRadians, Luminous::Style & style, unsigned int linesegments)
  {
    if (linesegments == 0) {
      /// @todo Automagically determine the proper number of linesegments
      linesegments = 32;
    }
    std::vector<Nimble::Vector2f> vertices(linesegments + 1);
    float step = (toRadians - fromRadians) / linesegments;

    float angle = fromRadians;
    for (unsigned int i = 0; i <= linesegments; ++i) {
      Nimble::Vector2f c(std::cos(angle), std::sin(angle));
      vertices[i] = center + c * radius;
      angle += step;
    }

    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, vertices.data(), vertices.size(), 
      program, style.strokeColor(), style.strokeWidth());
  }

  void RenderContext::drawCircle(Nimble::Vector2f center, float radius, Luminous::Style & style, unsigned int linesegments)
  {
    if (linesegments == 0) {
      /// @todo Automagically determine the proper number of linesegments
      linesegments = 32;
    }

    std::vector<Nimble::Vector2f> vertices(linesegments + 2);
    float step = Nimble::Math::TWO_PI / linesegments;

    // Center is the first vertex in a fan
    vertices[0] = center;

    // Add the rest of the fan vertices
    float angle = 0.f;
    for (unsigned int i = 0; i <= linesegments; ++i) {
      Nimble::Vector2f c(std::cos(angle), std::sin(angle));
      vertices[1 + i] = center + c * radius;
      angle += step;
    }

    //vertices[linesegments+1] = vertices[1];

    // Draw fill
    if (style.fillColor().w > 0.f) {
      if (style.fill().textures().empty()) {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_TriangleFan, vertices.data(), vertices.size(), program, style.fillColor());
      } else
      {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        /// @todo generate UVs
        std::vector<Nimble::Vector2f> uvs;
        drawTexPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PrimitiveType_TriangleFan, vertices.data(), uvs.data(), vertices.size(),
          program, style.fill().textures(), style.fillColor());
      }
    }

    // Draw stroke
    if (style.strokeWidth() > 0.f) {
      const Program & program = (style.strokeProgram() ? *style.strokeProgram() : texShader());
      drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, &vertices[1], linesegments+1, program, style.strokeColor(), style.strokeWidth());
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
    /// @todo these look a bit crappy as the blending doesn't match the arcs properly
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

  /*
  void RenderContext::drawCurve(Vector2* controlPoints, float width, const float * rgba) {

    struct Subdivider {
      std::vector<Vector2> & points;

      void subdivide(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, int level=0) {
        Vector2 p12 = 0.5f*(p1+p2);
        Vector2 p23 = 0.5f*(p2+p3);
        Vector2 p34 = 0.5f*(p3+p4);
        Vector2 p123 = 0.5f*(p12+p23);
        Vector2 p234 = 0.5f*(p23+p34);
        Vector2 p1234 = 0.5f*(p123 + p234);

        ///@todo could do collinearity detection
        if (level != 0 && (level > 20 ||
                           fabs( (p1234 - 0.5f*(p1+p4)).lengthSqr() ) < 1e-1f)) {
          //points.push_back(p1);
          //points.push_back(p4);
          points.push_back(p23);
        } else {
          subdivide(p1, p12, p123, p1234, level+1);
          subdivide(p1234, p234, p34, p4, level+1);
        }
      }
    };
    std::vector<Nimble::Vector2f> points;
    points.push_back(controlPoints[0]);

    Subdivider sub = {points};
    sub.subdivide( controlPoints[0], controlPoints[1], controlPoints[2], controlPoints[3] );
    points.push_back(controlPoints[3]);

    Style s;
    s.setFillColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    drawLineStrip(points.data(), (int) points.size(), width, s);
  }

  void RenderContext::drawSpline(Nimble::Interpolating & s,
                                 float width, const float * rgba, float step)
  {
    const float len = s.size();

    if (len < 2)
      return;
    std::vector<Vector2> points;

    for(float t = 0.f; t < len - 1; t += step) {
      int ii = static_cast<int>(t);
      float t2 = t - ii;
      Vector2 point = s.getPoint(ii, t2);

      if (points.size() >= 2) {
          Vector2 p1 = (point - points[points.size()-2]);
          Vector2 p2 = (point - points[points.size()-1]);
          p1.normalize();
          p2.normalize();
          // 3 degrees
          if (dot(p1, p2) > 0.99862953475457383) {
              points.pop_back();
          }

      }
      points.push_back(point);
    }

    Style style;
    style.setFillColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    drawLineStrip(points.data(), (int) points.size(), width, style);
  }
  */

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

    char * data = mapBuffer<char>(buffer->buffer, Buffer::MapWrite |
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

  RenderCommand & RenderContext::createRenderCommand(bool translucent,
                                                     int indexCount, int vertexCount,
                                                     std::size_t vertexSize, std::size_t uniformSize,
                                                     unsigned *& mappedIndexBuffer,
                                                     void *& mappedVertexBuffer,
                                                     void *& mappedUniformBuffer,
                                                     float & depth,
                                                     const Program & shader,
                                                     const std::map<QByteArray,Texture *> & textures)
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

    RenderCommand & cmd = m_data->m_driver.createRenderCommand(
          translucent, vbuffer->buffer, ibuffer->buffer, ubuffer->buffer, shader, textures);
    cmd.primitiveCount = indexCount;
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
    const Nimble::Vector2f corners[] = { rect.low(), rect.highLow(), rect.lowHigh(), rect.high() };

    if (style.fillColor().w > 0.f) {
      if(style.fill().textures().empty()) {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, corners, 4, program, style.fillColor());
      }
      else {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        static const Nimble::Vector2f uvs[] = { Nimble::Vector2f(0,0), Nimble::Vector2f(1,0), Nimble::Vector2f(0,1), Nimble::Vector2f(1,1) };
        drawTexPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, corners, uvs, 4, program, style.fill().textures(), style.fillColor());
      }
    }

    // Draw the outline
    if (style.strokeWidth() > 0.f && style.strokeColor().w > 0.f) {
      const Nimble::Vector2f outline[] = { rect.low(), rect.highLow(), rect.high(), rect.lowHigh(), rect.low() };
      const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
      drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, outline, 5, program, style.strokeColor(), style.strokeWidth());
    }
  }

  //
  void RenderContext::drawRectWithHole(const Nimble::Rect & area,
                                       const Nimble::Rect & hole,
                                       const Luminous::Style & style)
  {
    const Nimble::Vector2f vertices[] = {
      hole.low(), area.low(),
      hole.highLow(), area.highLow(),
      hole.high(), area.high(),
      hole.lowHigh(), area.lowHigh(),
      hole.low(), area.low()
    };

    if (style.fillColor().w > 0.f) {
      if (style.fill().textures().empty()) {
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, vertices, 10, program, style.fillColor());
      } else
      {
        /// @todo calculate correct UVs for the inside ring
        const Nimble::Vector2f uvs[] = {
          Nimble::Vector2f(0,0), Nimble::Vector2f(0,0),
          Nimble::Vector2f(0,0), Nimble::Vector2f(1,0),
          Nimble::Vector2f(0,0), Nimble::Vector2f(1,1),
          Nimble::Vector2f(0,0), Nimble::Vector2f(0,1),
          Nimble::Vector2f(0,0), Nimble::Vector2f(0,0),
        };
        const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
        drawTexPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PrimitiveType_TriangleStrip, vertices, uvs, 10, program, style.fill().textures(), style.fillColor());
      }
    }

    // Draw the stroke
    if (style.strokeWidth() > 0.f && style.strokeColor().w > 0.f) {
      const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
      const Nimble::Vector2f innerStroke[] = { hole.low(), hole.highLow(), hole.high(), hole.lowHigh(), hole.low() };
      const Nimble::Vector2f outerStroke[] = { hole.low(), hole.highLow(), hole.high(), hole.lowHigh(), hole.low() };
      drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, innerStroke, 5, program, style.strokeColor(), style.strokeWidth());
      drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, outerStroke, 5, program, style.strokeColor(), style.strokeWidth());
    }
  }

  //
  void RenderContext::drawLine(const Nimble::Vector2 & p1, const Nimble::Vector2 & p2, const Luminous::Style & style)
  {
    const Nimble::Vector2f vertices[] = { p1, p2 };
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, vertices, 2, program, style.strokeColor(), style.strokeWidth());
  }

  //
  void RenderContext::drawPolyLine(const Nimble::Vector2 * points, unsigned int numPoints, const Luminous::Style & style)
  {
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_LineStrip, points, numPoints, program, style.strokeColor(), style.strokeWidth());
  }

  void RenderContext::drawPoints(const Nimble::Vector2f * points, size_t numPoints, const Luminous::Style & style)
  {
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PrimitiveType_Point, points, numPoints, program, style.strokeColor(), style.strokeWidth());
  }

  Nimble::Vector2 RenderContext::contextSize() const
  {
    return m_data->contextSize();
  }

  void RenderContext::setBlendFunc(BlendFunc f)
  {
    m_data->m_blendFunc = f;

    useCurrentBlendMode();
  }

  void RenderContext::useCurrentBlendMode()
  {
    //m_data->m_driver->setBlendFunc(m_data->m_blendFunc);
  }

  const char ** RenderContext::blendFuncNames()
  {
    static const char * names [] = {
      "usual",
      "none",
      "additive",
      "subtractive"
    };

    return names;
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

  void RenderContext::bindTexture(GLenum textureType, GLenum textureUnit,
                                    GLuint textureId)
  {
    //Utils::glCheck("RenderContext::bindTexture # 1");

    unsigned textureIndex = textureUnit - GL_TEXTURE0;

    assert(textureIndex < Internal::MAX_TEXTURES);

    if(m_data->m_textures[textureIndex] == textureId) {
      return;
    }

    if(m_data->m_textures[textureIndex]) {
      flush();
    }

    m_data->m_textures[textureIndex] = textureId;

    glActiveTexture(textureUnit);
    glBindTexture(textureType, textureId);

    //Utils::glCheck("RenderContext::bindTexture # 2");
  }

  void RenderContext::bindBuffer(GLenum type, GLuint id)
  {
    /// @todo wtf is this? you can only bind one vertex buffer at a time and never any index buffers?
    if(type == GL_ARRAY_BUFFER) {

      if(m_data->m_vbo != id) {

        m_data->m_vbo = id;
      }
    }
    glBindBuffer(type, id);
  }

  void RenderContext::bindProgram(GLSLProgramObject * program)
  {
    // Radiant::info("RenderContext::bindProgram # %p", program);
    //Utils::glCheck("RenderContext::bindProgram # 1");

    if(m_data->m_program != program) {
      flush();
      if(program)
        glUseProgram(program->m_handle);
      else
        glUseProgram(0);
      m_data->m_program = program;
    }

    //Utils::glCheck("RenderContext::bindProgram # 2");
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

  void RenderContext::flush()
  {
    RenderPacket * rp = m_data->m_renderPacket;

    if(!rp)
      return;

    if(rp->empty())
      return;

    RenderPacket::RenderFunction rf = rp->renderFunction();

    assert(rf != 0);

    (*rf)(*this, *rp);

    rp->setPacketRenderFunction(0);
    rp->setProgram(0);
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


  VertexAttribArrayStep::VertexAttribArrayStep(int pos, int elems, GLenum type, GLboolean normalized, size_t stride,
                        size_t offset)
                          : m_pos(pos)
  {
    if(pos >= 0)
    {
      glEnableVertexAttribArray(pos);
      glVertexAttribPointer(pos, elems, type, normalized,
                            (GLsizei)stride, (char*)0 + offset);
    }
    else
    {
      Radiant::warning("Luminous::VertexAttribArrayStep: trying to enable an attribute array with invalid index\n");
    }
  }

  VertexAttribArrayStep::VertexAttribArrayStep(GLSLProgramObject & prog, const char * attribname,
                                               int elems, GLenum type, GLboolean normalized, size_t stride,
                                               size_t offset, const char * userstr)
    : m_pos(prog.getAttribLoc(attribname))
  {
    if(m_pos >= 0)
    {
      glEnableVertexAttribArray(m_pos);
      glVertexAttribPointer(m_pos, elems, type, normalized,
                            (GLsizei)stride, (char*)0 + offset);
    }
    else
    {
      Radiant::warning("Luminous::VertexAttribArrayStep: trying to enable an attribute "
                       "array with invalid name \"%s\", in \"%s\", from \"%s\"",
                       attribname, prog.label().toLocal8Bit().data(), userstr);
    }
  }

  VertexAttribArrayStep::~VertexAttribArrayStep ()
  {
    glDisableVertexAttribArray(m_pos);
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

  Program & RenderContext::basicShader()
  {
    return m_data->m_basicShader;
  }

  Program & RenderContext::texShader()
  {
    return m_data->m_texShader;
  }

  TextureGL & RenderContext::handle(Texture & texture)
  {
    assert(m_data->m_driverGL);
    return m_data->m_driverGL->handle(texture);
  }

  RenderTargetGuard RenderContext::pushRenderTarget(RenderTarget &target)
  {
    m_data->m_driverGL->pushRenderTarget(target);

    m_data->m_currentRenderTarget = &target;

    // Push new projection matrix
    pushViewTransform(Nimble::Matrix4::ortho3D(0.f, target.size().width(), target.size().height(), 0.f, -1.f, 1.f));

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
    assert(stackSize() == 1);

    // Push the default render target. Don't use the RenderContext API to avoid
    // the guard.
    m_data->m_driverGL->pushRenderTarget(m_data->m_defaultRenderTarget);
    m_data->m_currentRenderTarget = &m_data->m_defaultRenderTarget;

    m_data->m_driver.preFrame();
  }

  void RenderContext::endFrame()
  {
    flush2();

    m_data->m_driver.postFrame();

    /// @todo how do we generate this properly? Should we somehow linearize the depth buffer?
    m_data->m_automaticDepthDiff = -1.0f / std::max(m_data->m_renderCalls.top(), 100000);
    assert(m_data->m_renderCalls.size() == 1);
    m_data->m_renderCalls.top() = 0;

    // Pop the default target
    m_data->m_driverGL->popRenderTarget();

    assert(stackSize() == 1);
  }

  void RenderContext::beginArea()
  {
    assert(stackSize() == 1);
    assert(transform4() == Nimble::Matrix4::IDENTITY);
    assert(clipStack().empty());
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

}
