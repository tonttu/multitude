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

#include "RenderContext.hpp"

#include "Error.hpp"
#include "GLContext.hpp"
#include "Texture.hpp"
#include "FramebufferObject.hpp"

#include "Utils.hpp"
#include "GLSLProgramObject.hpp"

#include <strings.h>

#define DEFAULT_RECURSION_LIMIT 8

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

    m_tex.bind();
    m_tex.setWidth(size.x);
    m_tex.setHeight(size.y);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // <- essential on Nvidia
  }

  void RenderContext::FBOPackage::attach()
  {
    m_fbo.attachTexture2D(&m_tex, Luminous::COLOR0, 0);
    m_fbo.check();
  }

  void RenderContext::FBOPackage::activate(RenderContext & r)
  {
    glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

    attach();
    r.pushDrawBuffer(Luminous::COLOR0, this);
    // Save and setup viewport to match the FBO
    glViewport(0, 0, m_tex.width(), m_tex.height());

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, m_tex.width(), 0, m_tex.height());
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
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
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

    Internal(const Luminous::MultiHead::Window * win)
        : m_recursionLimit(DEFAULT_RECURSION_LIMIT),
        m_recursionDepth(0),
        m_renderCount(0),
        m_frameCount(0),
        m_window(win),
        m_viewStackPos(-1),
        m_glContext(new GLDummyContext),
        m_initialized(false),
        m_blendFunc(BLEND_USUAL)
    {

      m_attribs.resize(10000);
      m_attribs.clear();

      m_verts.resize(10000);
      m_verts.clear();
    }

    ~Internal()
    {
      delete m_glContext;
    }

    void pushFBO(std::shared_ptr<FBOPackage> fbo)
    {
      m_fboStack.push(fbo);
    }

    std::shared_ptr<FBOPackage> popFBO(std::shared_ptr<FBOPackage> fbo)
    {
      assert(fbo == m_fboStack.top());
      m_fboStack.pop();

      return m_fboStack.empty() ? std::shared_ptr<FBOPackage>() : m_fboStack.top();
    }

    void initialize() {

      assert(m_window != 0);

      if(!m_initialized) {
        m_initialized = true;
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

        m_circle_shader.reset(new GLSLProgramObject());
        m_circle_shader->loadStrings(circ_vert_shader, circ_frag_shader);

        m_polyline_shader.reset(new GLSLProgramObject());
        const char * polyline_frag = ""
                            " varying vec2 p1;\n"\
                            " varying vec2 p2;\n"\
                            "varying vec2 vertexcoord;\n"\
                            "uniform float width;\n"\
                            "void main() {\n"\
                            "gl_FragColor = gl_Color;\n"\
                            "vec2 pp = p2-p1;\n"\
                            "float t = ((vertexcoord.x-p1.x)*(p2.x-p1.x)+(vertexcoord.y-p1.y)*(p2.y-p1.y))/dot(pp,pp);\n"\
                            "t = clamp(t, 0.0, 1.0);\n"\
                            "vec2 point_on_line = p1+t*(p2-p1);\n"\
                            "float dist = length(vertexcoord-point_on_line);\n"\
                            "gl_FragColor.w *= clamp(width-dist, 0.0, 1.0);\n"\
                            "}";

        const char * polyline_vert = "\n"
                            "attribute vec2 coord;\n"\
                            "attribute vec2 coord2;\n"\
                            "uniform float width;\n"\
                            " varying vec2 p1;\n"\
                            " varying vec2 p2;\n"\
                            "varying vec2 vertexcoord;\n"\
                            "void main() {\n"\
                            "p1 = coord;\n"\
                            "p2 = coord2;\n"\
                            "vertexcoord = gl_Vertex.xy;\n"\
                            "gl_Position = gl_ProjectionMatrix * gl_Vertex;\n"\
                            "gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n"\
                            "gl_FrontColor = gl_Color;\n"\
                            "}\n";
        m_polyline_shader->loadStrings(polyline_vert, polyline_frag);

        m_viewFBO.reset(new Luminous::Framebuffer());
      }

      while(!m_drawBufferStack.empty())
        m_drawBufferStack.pop();
      m_drawBufferStack.push(DrawBuf(GL_BACK, 0)); // Start off by rendering to the back buffer
    }
    Nimble::Vector2 contextSize() const
    {
      return m_window->size();
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

      m_circle_shader->bind();

      // uniform scaling assumed, should work fine with "reasonable" non-uniform scaling
      float totalRadius = m.extractScale() * radius;
      float border = Nimble::Math::Min(1.0f, totalRadius-2.0f);
      m_circle_shader->setUniformFloat("border_start", (totalRadius-border)/totalRadius);
      GLint matrixLoc = m_circle_shader->getUniformLoc("matrix");
      glUniformMatrix4fv(matrixLoc, 1, GL_TRUE, t.data());

      // using a VBO with 4 vertices is actually slower than this
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(2, GL_FLOAT, 0, rect_vertices);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDisableClientState(GL_VERTEX_ARRAY);
      m_circle_shader->unbind();
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

    //std::stack<Nimble::Rectangle> m_clipStack;
    std::vector<Nimble::Rectangle> m_clipStack;

    typedef std::list<std::shared_ptr<FBOPackage> > FBOPackages;

    FBOPackages m_fbos;


    std::stack<std::shared_ptr<FBOPackage> > m_fboStack;

    class DrawBuf
    {
    public:
      DrawBuf() : m_fbo(0), m_dest(GL_BACK) {}
      DrawBuf(GLenum dest, FBOPackage * fbo) : m_fbo(fbo), m_dest(dest) {}
      FBOPackage * m_fbo;
      GLenum m_dest;
    };

    std::stack<DrawBuf> m_drawBufferStack;

    unsigned long m_renderCount;
    unsigned long m_frameCount;

    std::shared_ptr<Luminous::GLSLProgramObject> m_circle_shader;
    std::shared_ptr<Luminous::GLSLProgramObject> m_polyline_shader;

    const Luminous::MultiHead::Window * m_window;
    /// fbo for views
    std::shared_ptr<Luminous::Framebuffer> m_viewFBO;
    /// fbo texture stack for views
    std::vector<Luminous::Texture2D *> m_viewTextures;
    int m_viewStackPos;
    std::vector<Nimble::Vector2> m_attribs;
    std::vector<Nimble::Vector2> m_verts;

    Luminous::GLContext * m_glContext;

    bool m_initialized;

    BlendFunc m_blendFunc;
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

    glDrawArrays(GL_QUADS, 0, (GLsizei) m_verts.size());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableVertexAttribArray(loc);
    glDisableVertexAttribArray(loc2);
    m_polyline_shader->unbind();
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  RenderContext::RenderContext(Luminous::GLResources * resources, const Luminous::MultiHead::Window * win)
      : Transformer(),
      m_resources(resources),
      m_data(new Internal(win))
  {
    resetTransform();
    m_data->m_recursionDepth = 0;

    // Make sure the clip stack is empty
    while(!m_data->m_clipStack.empty())
      m_data->m_clipStack.pop_back();
  }

  RenderContext::~RenderContext()
  {
    debug("Closing OpenGL context. Rendered %lu things in %lu frames, %lu things per frame",
         m_data->m_renderCount, m_data->m_frameCount,
         m_data->m_renderCount / Nimble::Math::Max(m_data->m_frameCount, (unsigned long) 1));
    delete m_data;
  }

  void RenderContext::setWindow(const Luminous::MultiHead::Window * window)
  {
    m_data->m_window = window;
  }

  void RenderContext::prepare()
  {

    resetTransform();
    m_data->initialize();
    m_data->m_recursionDepth = 0;
    m_data->m_frameCount++;

    // Make sure the clip stack is empty
    while(!m_data->m_clipStack.empty())
      m_data->m_clipStack.pop_back();

  }

  void RenderContext::finish()
  {
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

  bool RenderContext::isVisible(const Nimble::Rectangle & area)
  {
//      Radiant::info("RenderContext::isVisible # area (%f,%f) (%f,%f)", area.center().x, area.center().y, area.size().x, area.size().y);

      if(m_data->m_clipStack.empty()) {
        Radiant::debug("\tclip stack is empty");
        return true;
      } else {

          // Since we have no proper clipping algorithm, we compare against every clip rectangle in the stack
          bool inside = true;

          // Why does const_reverse_iterator not work on OSX :(
          for(std::vector<Nimble::Rectangle>::reverse_iterator it = m_data->m_clipStack.rbegin(); it != m_data->m_clipStack.rend(); it++) {
            inside &= (*it).intersects(area);
          }

          /*
          const Nimble::Rectangle & clipArea = m_data->m_clipStack.top();

          bool inside = m_data->m_clipStack.top().intersects(area);

          Radiant::info("\tclip area (%f,%f) (%f,%f) : inside %d", clipArea.center().x, clipArea.center().y, clipArea.size().x, clipArea.size().y, inside);
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
    Nimble::Vector2i minimumsize = basicsize * scaling;

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
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, minimumsize.x, 0, minimumsize.y);

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

  void RenderContext::drawArc(Nimble::Vector2f center, float radius, float fromRadians, float toRadians, float width, float blendWidth, const float * color, int linesegments)
  {
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

  void RenderContext::drawWedge(Nimble::Vector2f center, float radius1, float radius2, float fromRadians, float toRadians, float width, float blendWidth, const float *rgba, int segments)
  {
    // Draw two arcs
    drawArc(center, radius1, fromRadians, toRadians, width, blendWidth, rgba, segments);
    drawArc(center, radius2, fromRadians, toRadians, width, blendWidth, rgba, segments);

    // Draw sector edges
    /// @todo these look a bit crappy as the blending doesn't match the arcs properly
    Nimble::Vector2f p0 = center + radius1 * Nimble::Vector2f(cos(fromRadians), sin(fromRadians));
    Nimble::Vector2f p1 = center + radius2 * Nimble::Vector2f(cos(fromRadians), sin(fromRadians));

    Nimble::Vector2f p2 = center + radius1 * Nimble::Vector2f(cos(toRadians), sin(toRadians));
    Nimble::Vector2f p3 = center + radius2 * Nimble::Vector2f(cos(toRadians), sin(toRadians));

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
        if (level != 0 && (level > 20 || fabs( (p1234 - 0.5f*(p1+p4)).lengthSqr() ) < 1e-1f)) {
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

  void RenderContext::drawSpline(Nimble::Interpolating & s, float width, const float * rgba, float step)
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

#if 0
    // This fails when some other OpenGL features are used (FBOs, VBOs)
    glEnable(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_COORD_ARRAY);

    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<GLfloat*>(v));
    glDrawArrays(GL_QUADS, 0, 4);

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
    const Nimble::Matrix3 & m = transform();

    Nimble::Vector2 v[] = {
      m.project(0, 0),
      m.project(size.x, 0),
      m.project(size.x, size.y),
      m.project(0, size.y)
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

#if 0
    // This fails when some other OpenGL features are used (FBOs, VBOs)
    glEnable(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_COORD_ARRAY);

    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<GLfloat*>(v));
    glDrawArrays(GL_QUADS, 0, 4);

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
    //Radiant::info("RenderContext::useCurrentBlendMode # %s", blendFuncNames()[m_data->m_blendFunc]);

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

    fbo = m_data->popFBO(fbo);

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

  void RenderContext::setGLContext(Luminous::GLContext * ctx)
  {
    delete m_data->m_glContext;
    m_data->m_glContext = ctx;
  }

  Luminous::GLContext * RenderContext::glContext()
  {
    return m_data->m_glContext;
  }

}

