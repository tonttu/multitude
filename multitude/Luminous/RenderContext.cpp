/* COPYRIGHT
 */

#include "RenderContext.hpp"

// #include "Dum"
#include "Error.hpp"
#include "Texture.hpp"
#include "FramebufferObject.hpp"
//#include "RenderTarget.hpp"

#include "Utils.hpp"
#include "VertexHolder.hpp"
#include "GLSLProgramObject.hpp"

// Luminous v2
#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/HardwareBuffer.hpp"

#include <Nimble/Matrix4.hpp>

#include <Radiant/Mutex.hpp>
#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Thread.hpp>

#include <strings.h>

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
    r.pushViewTransform();
    r.setViewTransform(Nimble::Matrix4::ortho3D(0, m_tex.width(), 0, m_tex.height(), -1, 1));
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

  class RenderContext::Internal
  {
  public:
    enum { MAX_TEXTURES = 64 };

    Internal(unsigned int threadIndex, RenderDriver & renderDriver, const Luminous::MultiHead::Window * win)
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
        , m_driver(renderDriver)
        , m_threadIndex(threadIndex)
    {
      m_viewTransform.identity();
      m_viewTransformStack.push_back(m_viewTransform);
      m_attribs.resize(10000);
      m_attribs.clear();

      m_verts.resize(10000);
      m_verts.clear();

      bzero(m_textures, sizeof(m_textures));
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
#ifndef LUMINOUS_OPENGLES
        const char * circ_vert_shader = ""\
            "uniform mat4 matrix;"\
            "varying vec2 pos;"\
            "void main(void) {"\
            "  pos = gl_Vertex.xy; "\
            "  mat4 transform = gl_ProjectionMatrix * matrix;"\
            "  gl_Position = transform * gl_Vertex;"\
            "  gl_ClipVertex = gl_ModelViewMatrix * matrix * gl_Vertex;"
            "  gl_FrontColor = gl_Color;"\
            "}";
        const char * circ_frag_shader = ""\
            "varying vec2 pos;"\
            "uniform float border_start;"\
            "void main(void) {"\
            "  float r = length(pos);"\
            "  gl_FragColor = gl_Color;"\
            "  gl_FragColor.w *= smoothstep(1.00, border_start, r);"\
            "}";

        m_circle_shader_old.reset(new GLSLProgramObject());
        m_circle_shader_old->loadStrings(circ_vert_shader, circ_frag_shader);

        m_polyline_shader.reset(new GLSLProgramObject());
        const char * polyline_frag = SHADER(
            varying vec2 p1;
            varying vec2 p2;
            varying vec2 vertexcoord;
            uniform float width;
            void main() {
              gl_FragColor = gl_Color;
              vec2 pp = p2-p1;
              float t = ((vertexcoord.x-p1.x)*(p2.x-p1.x)+(vertexcoord.y-p1.y)*(p2.y-p1.y))/dot(pp,pp);
              t = clamp(t, 0.0, 1.0);
              vec2 point_on_line = p1+t*(p2-p1);
              float dist = length(vertexcoord-point_on_line);
              gl_FragColor.w *= clamp(width-dist, 0.0, 1.0);
            }
          );

          const char * polyline_vert = SHADER(
              attribute vec2 coord;
              attribute vec2 coord2;
              uniform float width;
              varying vec2 p1;
              varying vec2 p2;
              varying vec2 vertexcoord;
              void main() {
                p1 = coord;
                p2 = coord2;
                vertexcoord = gl_Vertex.xy;
                gl_Position = gl_ProjectionMatrix * gl_Vertex;
                gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;
                gl_FrontColor = gl_Color;
              }
            );
        m_polyline_shader->loadStrings(polyline_vert, polyline_frag);
#endif

        /*
        Radiant::ResourceLocator::instance().addPath
            ("/Users/tommi/cornerstone/share/MultiTouch/");
            */

        GLSLProgramObject * basic =
            GLSLProgramObject::fromFiles(locateStandardShader("basic_tex.vs").toUtf8().data(),
                                         locateStandardShader("basic_tex.fs").toUtf8().data());
        if(!basic)
          fatal("Could not load basic shader for rendering");
        m_basic_shader.reset(basic);

        GLSLProgramObject * arc =
            GLSLProgramObject::fromFiles(locateStandardShader("arc_tex.vs").toUtf8().data(),
                                         locateStandardShader("arc_tex.fs").toUtf8().data());
        if(!arc)
          fatal("Could not load arc shader for rendering");
        m_arc_shader.reset(arc);

        GLSLProgramObject * circle =
            GLSLProgramObject::fromFiles(locateStandardShader("circle_tex.vs").toUtf8().data(),
                                         locateStandardShader("circle_tex.fs").toUtf8().data());
        if(!circle)
          fatal("Could not load circle shader for rendering");
        m_circle_shader.reset(circle);

        m_viewFBO.reset(new Luminous::Framebuffer());

        m_emptyTexture.reset(Texture2D::fromBytes(GL_RGB, 32, 32, 0,
                                                  Luminous::PixelFormat::rgbUByte(), false));
        info("RenderContext::Internal # init ok");
      }

      bzero(m_textures, sizeof(m_textures));
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

    void drawCircle(RenderContext & r, Nimble::Vector2f center, float radius,
                                   const float * rgba) {

      const Matrix3f& m = r.transform();
      const float tx = center.x;
      const float ty = center.y;

      static const GLfloat rect_vertices[] = {
        -1.0, -1.0,
        1.0, -1.0,
        1.0, 1.0,
        -1.0, 1.0
      };

      // translate(tx, ty) & scale(radius)
      Matrix4f t(m[0][0]*radius, m[0][1]*radius, 0, m[0][2] + m[0][1]*ty+m[0][0]*tx,
                 m[1][0]*radius, m[1][1]*radius, 0, m[1][2] + m[1][1]*ty+m[1][0]*tx,
                 0         , 0         , 1, 0,
                 m[2][0]*radius, m[2][1]*radius, 0, m[2][2]  + m[2][1]*ty+m[2][0]*tx);

      if(rgba)
        glColor4fv(rgba);

      m_circle_shader_old->bind();

      // uniform scaling assumed, should work fine with "reasonable" non-uniform scaling
      float totalRadius = m.extractScale() * radius;
      float border = Nimble::Math::Min(1.0f, totalRadius-2.0f);
      m_circle_shader_old->setUniformFloat("border_start", (totalRadius-border)/totalRadius);
      GLint matrixLoc = m_circle_shader_old->getUniformLoc("matrix");
      glUniformMatrix4fv(matrixLoc, 1, GL_TRUE, t.data());

      // using a VBO with 4 vertices is actually slower than this
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(2, GL_FLOAT, 0, rect_vertices);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDisableClientState(GL_VERTEX_ARRAY);
      m_circle_shader_old->unbind();
    }

    void drawPolyLine(RenderContext& r, const Nimble::Vector2f * vertices, int n,
                      float width, const float * rgba);

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

    std::shared_ptr<Luminous::GLSLProgramObject> m_circle_shader_old;
    std::shared_ptr<Luminous::GLSLProgramObject> m_polyline_shader;
    std::shared_ptr<Luminous::GLSLProgramObject> m_basic_shader;
    std::shared_ptr<Luminous::GLSLProgramObject> m_arc_shader;
    std::shared_ptr<Luminous::GLSLProgramObject> m_circle_shader;

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

    std::vector<Nimble::Matrix4> m_viewTransformStack;

    Nimble::Matrix4 m_viewTransform;

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

    Luminous::RenderDriver & m_driver;
    unsigned int m_threadIndex;
  };

  void RenderContext::Internal::drawPolyLine(RenderContext& r, const Nimble::Vector2f * vertices, int n,
                                             float width, const float * rgba)
  {
    if(n < 2)
      return;

    width *= r.scale() * 0.5f;
    width += 1; // for antialiasing

    const Matrix3 & m = r.transform();
    Vector2f cprev;
    Vector2f cnow = m.project(vertices[0]);
    Vector2f cnext;
    Vector2f avg;
    Vector2f dirNext;
    Vector2f dirPrev;

    int nextIdx = 1;
    while ((vertices[nextIdx]-cnow).lengthSqr() < 9.0f && nextIdx < n-1) {
      nextIdx++;
    }

    cnext = m.project(vertices[nextIdx]);
    dirNext = cnext - cnow;
    dirNext.normalize();
    avg = dirNext.perpendicular();

    if (avg.length() < 1e-5) {
      avg.make(1,0);
    } else {
      avg.normalize();
    }
    avg *= width;

    m_verts.clear();
    m_verts.push_back(cnow + avg);
    m_verts.push_back(cnow - avg);

    m_attribs.clear();

    m_attribs.push_back(cnow);
    m_attribs.push_back(cnow);
    m_attribs.push_back(cnow);
    m_attribs.push_back(cnow);

    for (int i = nextIdx; i < n; ) {
      nextIdx = i+1;
      cprev = cnow;
      cnow = cnext;

      // at least 3 pixels gap between vertices
      while (nextIdx < n-1 && (vertices[nextIdx]-cnow).lengthSqr() < 9.0f) {
        nextIdx++;
      }
      if (nextIdx > n-1) {
        cnext = 2.0f*cnow - cprev;
      } else {
        cnext = m.project(vertices[nextIdx]);
      }

      dirPrev = dirNext;
      dirNext = cnext - cnow;

      if (dirNext.length() < 1e-5f) {
        dirNext = dirPrev;
      } else {
        dirNext.normalize();
      }

      avg = (dirPrev + dirNext).perpendicular();
      avg.normalize();

      float dp = Math::Clamp(dot(avg, dirPrev.perpendicular()), 1e-2f, 1.0f);
      avg /= dp;
      avg *= width;
      m_verts.push_back(cnow-avg);
      m_verts.push_back(cnow+avg);

      m_verts.push_back(cnow+avg);
      m_verts.push_back(cnow-avg);

      m_attribs.push_back(cnow);
      m_attribs.push_back(cnow);
      m_attribs.push_back(cnow);
      m_attribs.push_back(cnow);

      i = nextIdx;
    }

    GLuint loc = m_polyline_shader->getAttribLoc("coord");
    glEnableVertexAttribArray(loc);
    GLuint loc2 = m_polyline_shader->getAttribLoc("coord2");
    glEnableVertexAttribArray(loc2);

    m_polyline_shader->bind();
    m_polyline_shader->setUniformFloat("width", width);

    glColor4fv(rgba);
    glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<GLfloat *>(&m_verts[0]));
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLfloat *>(&m_attribs[0]));
    glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLfloat *>(&m_attribs[4]));
    glEnableClientState(GL_VERTEX_ARRAY);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei) m_verts.size());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableVertexAttribArray(loc);
    glDisableVertexAttribArray(loc2);
    m_polyline_shader->unbind();
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  RenderContext::RenderContext(unsigned int threadIndex, Luminous::RenderDriver & driver, const Luminous::MultiHead::Window * win)
      : Transformer(),
      m_data(new Internal(threadIndex, driver, win))
  {
    resetTransform();
    m_data->m_recursionDepth = 0;

    // Make sure the clip stack is empty
    while(!m_data->m_clipStack.empty())
      m_data->m_clipStack.pop_back();
  }

  RenderContext::~RenderContext()
  {
    debugLuminous("Closing OpenGL context. Rendered %lu things in %lu frames, %lu things per frame",
         m_data->m_renderCount, m_data->m_frameCount,
         m_data->m_renderCount / Nimble::Math::Max(m_data->m_frameCount, (unsigned long) 1));
    delete m_data;
  }

  void RenderContext::setWindow(const Luminous::MultiHead::Window * window,
                                const Luminous::MultiHead::Area * area)
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

  QString RenderContext::locateStandardShader(const QString & filename)
  {
    QString pathname;
#ifdef LUMINOUS_OPENGL_FULL

    const char * shaderpath = getenv("CORNERSTONE_CUSTOM_SHADER_PATH");

#ifdef RADIANT_OSX
    /* OSX has broken shaders, so lets just go with custom shaders by default. */
    if(!shaderpath)
      shaderpath = "../MultiTouch/GL21OSXShaders/";
#endif

    if(shaderpath) {
      QString tmp(shaderpath);
      tmp += filename;
      tmp = Radiant::ResourceLocator::instance().locate(tmp);
      if(!tmp.isEmpty()) {
        return tmp;
      }
    }

    // pathname = "../MultiTouch/GL20Shaders/";
    pathname = "../MultiTouch/ES20Shaders/";
#else
    pathname = "../MultiTouch/ES20Shaders/";
#endif
    pathname += filename;

    return Radiant::ResourceLocator::instance().locate(pathname);
  }

  void RenderContext::prepare()
  {
    Utils::glCheck("RenderContext::prepare # 1");

    resetTransform();
    m_data->initialize();


    // Make sure the clip stack is empty
    while(!m_data->m_clipStack.empty())
      m_data->m_clipStack.pop_back();

    restart();


    Utils::glCheck("RenderContext::prepare # 2");
  }

  void RenderContext::finish()
  {
    flush();
    bindProgram(0);
  }

  void RenderContext::pushViewTransform()
  {
    m_data->m_viewTransformStack.push_back(m_data->m_viewTransform);
    if(m_data->m_viewTransformStack.size() > 200) {
      error("RenderContext::pushViewTransform # stack extremely deep (%d)",
            (int) m_data->m_viewTransformStack.size());
    }
  }

  void RenderContext::popViewTransform()
  {
    if(!m_data->m_viewTransformStack.empty()) {
      flush();
      m_data->m_viewTransformStack.pop_back();
      if(!m_data->m_viewTransformStack.empty()) {
        m_data->m_viewTransform = m_data->m_viewTransformStack.back();
      }
    }
    else {
      error("RenderContext::popViewTransform # Stack empty");
    }
  }

  void RenderContext::setViewTransform(const Nimble::Matrix4 & m)
  {
    // Radiant::info("NEW View matrix = %s", Radiant::FixedStr256(m, 5).str());
    flush();

    m_data->m_viewTransform = m;
  }

  const Nimble::Matrix4 & RenderContext::viewTransform() const
  {
    return m_data->m_viewTransform;
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
    pushViewTransform();
    setViewTransform(Nimble::Matrix4::ortho3D(0, minimumsize.x, 0, minimumsize.y, -1, 1));
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

  void RenderContext::drawLineRect(const Nimble::Rectf & r,
                                   float thickness, const float * rgba)
  {
    thickness *= 0.5f;

    Vector2 v1(thickness, thickness);

    Nimble::Rectf inside(r.low() + v1, r.high() - v1);
    Nimble::Rectf outside(r.low() - v1, r.high() + v1);

    Utils::glRectWithHoleAA(outside, inside, transform(), rgba);
  }


  void RenderContext::drawRect(const Nimble::Rectf & rect, const float * rgba)
  {
    Utils::glTexRectAA(rect.size(),
                       transform() * Matrix3::translate2D(rect.low()), rgba);
  }


  void RenderContext::drawCircle(Nimble::Vector2f center, float radius,
                                 const float * rgba, int segments) {
    if (segments < 0) {
      drawCircleImpl(center, radius, rgba);
    } else {
      drawCircleWithSegments(center, radius, rgba, segments);
    }
  }

  void RenderContext::drawArc(Nimble::Vector2f center, float radius,
                              float fromRadians, float toRadians,
                              float width, float blendWidth, const float * color,
                              int linesegments)
  {
    width *= 0.5f;

    float delta = (toRadians - fromRadians) / linesegments;

    float tanFactor = tan(delta);
    float radFactor = 1.f - cos(delta);

    float r = color[0];
    float g = color[1];
    float b = color[2];
    float a = color[3];

    float radii[4] = {
      radius - width - blendWidth, radius - width,
      radius + width, radius + width + blendWidth
    };

    Nimble::Vector2 p[4];
    for(int k = 0; k < 4; k++)
      p[k] = center + radii[k] * Nimble::Vector2f(cos(fromRadians), sin(fromRadians));

    for(size_t i = 0; i < (size_t) linesegments; i++) {

      Nimble::Vector2 v[4];

      for(int k = 0; k < 4; k++) {
        v[k] = p[k];

        Nimble::Vector2f t(-(p[k].y - center.y), p[k].x - center.x);
        p[k] += tanFactor * t;
        Nimble::Vector2f r = center - p[k];
        p[k] += radFactor * r;
      }

      glBegin(GL_QUAD_STRIP);

      glColor4f(r, g, b, 0.f);

      glVertex2fv(transform().project(v[0]).data());
      glVertex2fv(transform().project(p[0]).data());

      glColor4f(r, g, b, a);

      glVertex2fv(transform().project(v[1]).data());
      glVertex2fv(transform().project(p[1]).data());

      glVertex2fv(transform().project(v[2]).data());
      glVertex2fv(transform().project(p[2]).data());

      glColor4f(r, g, b, 0.f);

      glVertex2fv(transform().project(v[3]).data());
      glVertex2fv(transform().project(p[3]).data());

      glEnd();
    }
  }

  void RenderContext::drawArc(Nimble::Vector2f center, float radius, float width,
                              float fromRadians, float toRadians,
                              const Luminous::Style & style)
  {
    if(style.program()) {
      style.program()->bind();
    }
    else {
      m_data->m_arc_shader->bind();
    }

    RenderPacket & rp = * m_data->m_renderPacket;
    rp.setProgram(&*m_data->m_arc_shader);
    rp.setPacketRenderFunction(ArcVertex::render);

    float w2 = width * 0.5f;
    radius += w2;
    float innderRelative = 1.0f - width / radius;

    ArcVertex v;
    v.m_color = style.color();
    v.m_useTexture = style.texturing();
    v.m_objectTransform = transform().transposed();
    v.m_arcParams.make(innderRelative, fromRadians, toRadians);

    v.m_location[2] = center.x;
    v.m_location[3] = center.y;

    v.m_location[0] = -radius;
    v.m_location[1] = -radius;
    v.m_texCoord = style.texCoords().low();
    v.m_objCoord.make(-1, -1);

    rp.addFirstVertex(v);

    v.m_location[0] = radius;
    v.m_location[1] = -radius;
    v.m_texCoord = style.texCoords().highLow();
    v.m_objCoord.make(1, -1);
    rp.addVertex(v);

    v.m_location[0] = -radius;
    v.m_location[1] = radius;
    v.m_texCoord = style.texCoords().lowHigh();
    v.m_objCoord.make(-1, 1);
    rp.addVertex(v);

    v.m_location[0] = radius;
    v.m_location[1] = radius;
    v.m_texCoord = style.texCoords().high();
    v.m_objCoord.make(1, 1);
    rp.addLastVertex(v);
  }

  void RenderContext::drawCircle(Nimble::Vector2f center, float radius, const Luminous::Style & style)
  {
    flush();
    m_data->m_circle_shader->bind();

    RenderPacket & rp = * m_data->m_renderPacket;
    rp.setProgram(&*m_data->m_circle_shader);
    rp.setPacketRenderFunction(CircleVertex::render);

    CircleVertex va;
    va.m_color = style.color();
    va.m_useTexture = style.texturing();
    va.m_objectTransform = transform().transposed();

    va.m_location = center - Nimble::Vector2(radius, radius);
    va.m_texCoord = style.texCoords().low();
    va.m_objCoord.make(0, 0);

    rp.addFirstVertex(va);

    va.m_location = center + Nimble::Vector2(radius, -radius);
    va.m_texCoord = style.texCoords().highLow();
    va.m_objCoord.make(1, 0);
    rp.addVertex(va);

    va.m_location = center + Nimble::Vector2(-radius, radius);
    va.m_texCoord = style.texCoords().lowHigh();
    va.m_objCoord.make(0, 1);
    rp.addVertex(va);

    va.m_location = center + Nimble::Vector2(radius, radius);
    va.m_texCoord = style.texCoords().high();
    va.m_objCoord.make(1, 1);
    rp.addLastVertex(va);
    flush();
  }

  void RenderContext::drawWedge(Nimble::Vector2f center, float radius1,
                                float radius2, float fromRadians, float toRadians,
                                float width, float blendWidth, const float *rgba,
                                int segments)
  {
    // Draw two arcs
    drawArc(center, radius1, fromRadians, toRadians, width, blendWidth, rgba, segments);
    drawArc(center, radius2, fromRadians, toRadians, width, blendWidth, rgba, segments);

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

    drawLine(p0, p1, width, rgba);
    drawLine(p2, p3, width, rgba);
  }

  void RenderContext::drawCircleImpl(Nimble::Vector2f center, float radius,
                                 const float * rgba) {
    m_data->drawCircle(*this, center, radius, rgba);
  }

  void RenderContext::addRenderCounter()
  {
    m_data->m_renderCount++;
  }

  void RenderContext::drawCircleWithSegments(Nimble::Vector2f center, float radius,
                                 const float * rgba, int segments)
  {
    if(segments < 0) {
      float realRad = radius * transform().extractScale();
      segments = Math::Clamp((int) realRad * 2, 6, 60);
    }

    Nimble::Matrix3 m(transform() * Nimble::Matrix3::translate2D(center));

    Utils::glSolidSoftCircle(m, radius, 1.0f, segments, rgba);
  }

  void RenderContext::drawPolyLine(const Nimble::Vector2f * vertices, int n,
                                   float width, const float * rgba)
  {
    m_data->drawPolyLine(*this, vertices, n, width, rgba);
  }

  void RenderContext::drawLine(Nimble::Vector2f p1, Nimble::Vector2f p2,
                               float width, const float * rgba)
  {
    Nimble::Vector2f vs[2] = { p1, p2 };
    drawPolyLine(vs, 2, width, rgba);
  }

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

    drawPolyLine(&points[0], (int) points.size(), width, rgba);
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
    drawPolyLine(&points[0], (int) points.size(), width, rgba);
  }


  void RenderContext::drawTexRect(const Nimble::Rect & area, const float * rgba,
                                  const Nimble::Rect & texUV)
  {
    const Nimble::Matrix3 & m = transform();

    Nimble::Vector2 v[] = {
      m.project(area.low()),
      m.project(area.highLow()),
      m.project(area.high()),
      m.project(area.lowHigh())
    };

    if(rgba)
      glColor4fv(rgba);

    const Vector2 & low = texUV.low();
    const Vector2 & high = texUV.high();

    const GLfloat texCoords[] = {
      low.x, low.y,
      high.x, low.y,
      high.x, high.y,
      low.x, high.y
    };

#if 1
    // This fails when some other OpenGL features are used (FBOs, VBOs)
    glEnable(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_COORD_ARRAY);

    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<GLfloat*>(v));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisable(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_COORD_ARRAY);
#else
    glBegin(GL_QUADS);

    for(int i = 0; i < 4; i++) {
      glTexCoord2fv(&texCoords[i * 2]);
      glVertex2fv(v[i].data());
    }

    glEnd();
#endif
  }


  void RenderContext::drawTexRect(const Nimble::Rect & area, const float * rgba,
                                  const Nimble::Rect * texUV, int uvCount)
  {
    if(rgba)
      glColor4fv(rgba);

    const Nimble::Matrix3 & m = transform();

    glBegin(GL_QUADS);

    for(int i = 0; i < uvCount; i++) {
      glMultiTexCoord2fv(GL_TEXTURE0 + i, texUV[i].low().data());
    }
    glVertex2fv(m.project(area.low()).data());

    for(int i = 0; i < uvCount; i++) {
      glMultiTexCoord2fv(GL_TEXTURE0 + i, texUV[i].highLow().data());
    }
    glVertex2fv(m.project(area.highLow()).data());

    for(int i = 0; i < uvCount; i++) {
      glMultiTexCoord2fv(GL_TEXTURE0 + i, texUV[i].high().data());
    }
    glVertex2fv(m.project(area.high()).data());

    for(int i = 0; i < uvCount; i++) {
      glMultiTexCoord2fv(GL_TEXTURE0 + i, texUV[i].lowHigh().data());
    }
    glVertex2fv(m.project(area.lowHigh()).data());

    glEnd();
  }

  void RenderContext::drawTexRect(Nimble::Vector2 size, const float * rgba)
  {
    drawTexRect(size, rgba, Nimble::Rect(0, 0, 1, 1));
  }

  void RenderContext::drawTexRect(Nimble::Vector2 size, const float * rgba,
                                  const Nimble::Rect & texUV)
  {
    Style style;
    style.setColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    style.setTexturing(1);
    style.setTexCoords(texUV);
    drawRect(Nimble::Rect(Nimble::Vector2(0,0), size), style);
  }

  void RenderContext::drawStyledRect(Nimble::Vector2 size, const Luminous::Style & style)
  {
    drawRect(Nimble::Rect(Nimble::Vector2(0,0), size), style);
  }

  void RenderContext::drawTexRect(Nimble::Vector2 size, const float * rgba,
                                  Nimble::Vector2 texUV)
  {
    drawTexRect(size, rgba, Rect(Vector2(0,0), texUV));
  }

  void RenderContext::drawTexRect(const Nimble::Rect & area, const float * rgba)
  {
    pushTransformRightMul(Nimble::Matrix3::translate2D(area.low()));
    drawTexRect(area.span(), rgba);
    popTransform();
  }

  Nimble::Vector4 proj(const Nimble::Matrix4 & m4, const Nimble::Matrix3 & m3,
                       Nimble::Vector2 v)
  {
    Nimble::Vector3 v3(v.x, v.y, 1);
    v3 = m3 * v3;
    Nimble::Vector4 v4(v3.x, v3.y, 0, v3.z);
    return m4 * v4;
  }

  void RenderContext::drawRect(const Nimble::Rect & area, const Style & style)
  {
    if(style.program()) {
      style.program()->bind();
    }
    else {
      m_data->m_basic_shader->bind();
    }

    RenderPacket & rp = * m_data->m_renderPacket;
    rp.setProgram(m_data->m_program);
    rp.setPacketRenderFunction(RectVertex::render);

    RectVertex va;
    va.m_color = style.color();
    va.m_useTexture = style.texturing();
    va.m_objectTransform = transform().transposed();

    va.m_location = area.low();
    va.m_texCoord = style.texCoords().low();

    rp.addFirstVertex(va);

    va.m_location = area.highLow();
    va.m_texCoord = style.texCoords().highLow();
    rp.addVertex(va);

    va.m_location = area.lowHigh();
    va.m_texCoord = style.texCoords().lowHigh();
    rp.addVertex(va);

    va.m_location = area.high();
    va.m_texCoord = style.texCoords().high();
    rp.addLastVertex(va);

    flush();
  }

  void RenderContext::drawRectWithHole(const Nimble::Rect & area,
                                       const Nimble::Rect & hole,
                                       const Luminous::Style & style)
  {
    if(style.program()) {
      style.program()->bind();
    }
    else {
      m_data->m_basic_shader->bind();
    }

    RenderPacket & rp = * m_data->m_renderPacket;

    rp.setProgram(m_data->m_program);
    rp.setPacketRenderFunction(RectVertex::render);

    RectVertex va;

    va.m_color = style.color();
    va.m_useTexture = false;
    va.m_location = area.low();
    va.m_objectTransform = transform().transposed();

    rp.addFirstVertex(va);

    va.m_location = hole.low();
    rp.addVertex(va);

    va.m_location = area.highLow();
    rp.addVertex(va);
    va.m_location = hole.highLow();
    rp.addVertex(va);

    va.m_location = area.high();
    rp.addVertex(va);
    va.m_location = hole.high();
    rp.addVertex(va);

    va.m_location = area.lowHigh();
    rp.addVertex(va);
    va.m_location = hole.lowHigh();
    rp.addVertex(va);

    va.m_location = area.low();
    rp.addVertex(va);
    va.m_location = hole.low();
    rp.addLastVertex(va);
  }

  void RenderContext::drawLine(const Nimble::Vector2 & p1, const Nimble::Vector2 & p2,
                               float width, const Luminous::Style & fill)
  {
    Nimble::Vector2 dir = p2 - p1;
    float l = dir.length();
    if(l < 1.0e-6f)
      return;

    dir /= l;

    Nimble::Vector2 perp = dir.perpendicular() * (width * 0.500001f);

    Nimble::Vector2 corners[4] = {
      p1 + perp,
      p1 - perp,
      p2 - perp,
      p2 + perp
    };

    drawQuad(corners, fill);
  }

  void RenderContext::drawLineStrip(const Nimble::Vector2 * vertices, size_t npoints,
                                    float width, const Luminous::Style & fill)
  {
    /* This is a very brutal line-strip implementation, that would need to be fixed. */

    if(npoints < 2)
      return;

    npoints--;

    // Nimble::Vector2 prevdir = vertices[1] - vertices[0];
    // prevdir.normalize();

    for(size_t i = 0; i < npoints; i++) {
      drawLine(vertices[i], vertices[i+1], width, fill);
      // Nimble::Vector2 dir = vertices[i+1] - vertices[1];
    }
  }


  void RenderContext::drawLineStrip(const std::vector<Nimble::Vector2> & vertices,
                                    float width, const Luminous::Style & fill)
  {
    drawLineStrip( & vertices[0], vertices.size(), width, fill);
  }

  void RenderContext::drawQuad(const Nimble::Vector2 * corners, const Luminous::Style & style)
  {
    m_data->m_basic_shader->bind();

    RenderPacket & rp = * m_data->m_renderPacket;
    rp.setProgram(m_data->m_basic_shader.get());
    rp.setPacketRenderFunction(RectVertex::render);

    RectVertex va;
    va.m_color = style.color();
    va.m_useTexture = style.texturing();
    va.m_objectTransform = transform().transposed();

    va.m_location = corners[0];
    va.m_texCoord = style.texCoords().lowHigh();

    rp.addFirstVertex(va);

    va.m_location = corners[1];
    va.m_texCoord = style.texCoords().high();
    rp.addVertex(va);

    va.m_location = corners[3];
    va.m_texCoord = style.texCoords().low();
    rp.addVertex(va);

    va.m_location = corners[2];
    va.m_texCoord = style.texCoords().highLow();
    rp.addLastVertex(va);
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
    //Radiant::info("RenderContext::useCurrentBlendMode # %s",
    // blendFuncNames()[m_data->m_blendFunc]);

    if(m_data->m_blendFunc == BLEND_NONE) {
      glDisable(GL_BLEND);
      return;
    }

    glEnable(GL_BLEND);

    if(m_data->m_blendFunc == BLEND_USUAL)
      Utils::glUsualBlend();
    else if(m_data->m_blendFunc == BLEND_ADDITIVE)
      Utils::glAdditiveBlend();
    else if(m_data->m_blendFunc == BLEND_SUBTRACTIVE)
      Utils::glSubtractiveBlend();
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

  // Doesn't work under windows where pthread_t (id_t) is a struct
  //typedef std::map<Thread::id_t, RenderContext *> ResourceMap;
  class TGLRes
  {
  public:
    TGLRes() : m_context(0) {}
    RenderContext       * m_context;
  };

  typedef std::map<Radiant::Thread::id_t, TGLRes> ResourceMap;

  static ResourceMap __resources;
  static Mutex __mutex;

  void RenderContext::setThreadContext(RenderContext * rsc)
  {
    Guard g(__mutex);
    TGLRes tmp;
    tmp.m_context = rsc;
    __resources[Radiant::Thread::myThreadId()] = tmp;
  }

  RenderContext * RenderContext::getThreadContext()
  {
    Guard g(__mutex);

    ResourceMap::iterator it = __resources.find(Radiant::Thread::myThreadId());

    if(it == __resources.end()) {
      debug("No OpenGL resources for current thread");
      return 0;
    }

    return (*it).second.m_context;
  }

  void RenderContext::bindTexture(GLenum textureType, GLenum textureUnit,
                                    GLuint textureId)
  {
    Utils::glCheck("RenderContext::bindTexture # 1");

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

    Utils::glCheck("RenderContext::bindTexture # 2");
  }

  void RenderContext::bindBuffer(GLenum type, GLuint id)
  {
    if(type == GL_ARRAY_BUFFER) {

      if(m_data->m_vbo != id) {

        m_data->m_vbo = id;
        glBindBuffer(type, id);
      }
    }
  }

  void RenderContext::bindProgram(GLSLProgramObject * program)
  {
    // Radiant::info("RenderContext::bindProgram # %p", program);
    Utils::glCheck("RenderContext::bindProgram # 1");

    if(m_data->m_program != program) {
      flush();
      if(program)
        glUseProgram(program->m_handle);
      else
        glUseProgram(0);
      m_data->m_program = program;
    }

    Utils::glCheck("RenderContext::bindProgram # 2");
  }

  void RenderContext::bindDefaultProgram()
  {
    bindProgram(&*m_data->m_basic_shader);
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
    m_data->m_basic_shader->bind();
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

  /// Start collecting render commands
  void RenderContext::beginCommands()
  {
  }

  /// Finish collecting render commands
  void RenderContext::endCommands()
  {
    /// Clear the state for the next set of commands
/*
    m_data->m_driver.clearState(threadIndex());

    bindDefaultProgram();
*/
  }

  void RenderContext::setTexture(const QString & name, std::shared_ptr<Luminous::Texture2> texture)
  {

  }

  void RenderContext::setVertexBinding(const std::shared_ptr<VertexAttributeBinding> & binding)
  {
    // Bind the VAO: Binds all the associated vertex buffers and sets the appropriate vertex attributes
    m_data->m_driver.setVertexBinding(threadIndex(), *binding);
  }

  void RenderContext::setShaderProgram(const std::shared_ptr<ShaderProgram> & program)
  {
    m_data->m_driver.setShaderProgram(threadIndex(), *program);

    // Try and bind the MultiTouch common parameters
    /// @todo this could be a piece of code with a uniform block that's always included in the shader program
    m_data->m_driver.setShaderConstant(threadIndex(), "mt_projMatrix", m_data->m_viewTransform);
  }

  void RenderContext::draw(PrimitiveType primType, unsigned int offset, unsigned int vertexCount)
  {
    m_data->m_driver.draw(primType, offset, vertexCount);
  }

  unsigned int RenderContext::threadIndex() const
  {
    return m_data->m_threadIndex;
  }

  // Create all the setters for shader constants
#define SETSHADERCONSTANT(TYPE) \
  template<> LUMINOUS_API bool RenderContext::setShaderConstant(const QString & name, const TYPE & value) \
  { \
    return m_data->m_driver.setShaderConstant(threadIndex(), name, value); \
  }
  SETSHADERCONSTANT(int);
  SETSHADERCONSTANT(float);
  SETSHADERCONSTANT(Nimble::Vector2i);
  SETSHADERCONSTANT(Nimble::Vector3i);
  SETSHADERCONSTANT(Nimble::Vector4i);
  SETSHADERCONSTANT(Nimble::Vector2f);
  SETSHADERCONSTANT(Nimble::Vector3f);
  SETSHADERCONSTANT(Nimble::Vector4f);
  SETSHADERCONSTANT(Nimble::Matrix2f);
  SETSHADERCONSTANT(Nimble::Matrix3f);
  SETSHADERCONSTANT(Nimble::Matrix4f);
#undef SETSHADERCONSTANT

  // Manual conversion: Radiant::Color > Nimble::Vector4f
  template<> LUMINOUS_API bool RenderContext::setShaderConstant(const QString & name, const Radiant::Color & value)
  {
    return m_data->m_driver.setShaderConstant(threadIndex(), name, static_cast<Nimble::Vector4f>(value));
  }
}

