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
#include "Texture.hpp"
#include "FramebufferObject.hpp"

#include "Utils.hpp"
#include "GLSLProgramObject.hpp"

#include <strings.h>

#define DEFAULT_RECURSION_LIMIT 4

namespace Luminous
{

  using namespace Nimble;
  using namespace Radiant;
  using namespace Luminous;

  class RenderContext::FBOPackage : public GLResource
  {
  public:
    friend class FBOHolder;


    FBOPackage() : m_users(0) {}
    virtual ~FBOPackage() {}

    void setSize(Nimble::Vector2i size)
    {
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

    void activate()
    {
      m_fbo.attachTexture2D(&m_tex, Luminous::COLOR0, 0);
      m_fbo.check();
    }

    int userCount() const { return m_users; }

    Luminous::Framebuffer   m_fbo;
    Luminous::Renderbuffer  m_rbo;
    Luminous::Texture2D     m_tex;
    int m_users;
  };

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  RenderContext::FBOHolder::FBOHolder()
      : m_context(0),
      m_package(0),
      m_texUV(1,1)
  {
  }

  RenderContext::FBOHolder::FBOHolder(RenderContext * context, FBOPackage * package)
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

      m_package = 0;
      m_context = 0;
    }
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  class RenderContext::Internal
  {
  public:
    enum { FBO_STACK_SIZE = 100 };

    Internal()
        : m_recursionLimit(DEFAULT_RECURSION_LIMIT),
        m_recursionDepth(0),
        m_fboStackIndex(-1)
    {
      bzero(m_fboStack, sizeof(m_fboStack));
    }

    void pushFBO(FBOPackage * fbo)
    {
      m_fboStackIndex++;
      assert(m_fboStackIndex < FBO_STACK_SIZE);
      m_fboStack[m_fboStackIndex] = fbo;
    }

    FBOPackage * popFBO(FBOPackage * fbo)
    {
      assert(fbo == m_fboStack[m_fboStackIndex]);
      m_fboStackIndex--;

      if(m_fboStackIndex >= 0)
        return m_fboStack[m_fboStackIndex];

      return 0;
    }

    size_t m_recursionLimit;
    size_t m_recursionDepth;

    std::stack<Nimble::Rectangle> m_clipStack;

    typedef std::list<Radiant::RefPtr<FBOPackage> > FBOPackages;

    FBOPackages m_fbos;


    FBOPackage * m_fboStack[FBO_STACK_SIZE];
    int m_fboStackIndex;
    // temporarilly having screen size to make it work for lod and AA.
    Vector2i m_screenSize;
  };

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  RenderContext::RenderContext(Luminous::GLResources * resources)
      : Transformer(),
      m_resources(resources),
      m_data(new Internal)
  {
    resetTransform();
    m_data->m_recursionDepth = 0;

    // Make sure the clip stack is empty
    while(!m_data->m_clipStack.empty())
      m_data->m_clipStack.pop();
  }

  RenderContext::~RenderContext()
  {
    delete m_data;
  }

  void RenderContext::prepare()
  {
    resetTransform();
    m_data->m_recursionDepth = 0;

    // Make sure the clip stack is empty
    while(!m_data->m_clipStack.empty())
      m_data->m_clipStack.pop();


    static bool once = true;
    if(once) {
      once = false;
      const char * circ_vert_shader = ""\
          "uniform mat4 matrix;"\
          "varying vec2 pos;"\
          "void main(void) {"\
          "  pos = gl_Vertex; "\
          "  mat4 transform = gl_ProjectionMatrix * matrix;"\
          "  gl_Position = transform * gl_Vertex;"\
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

      m_circle_shader = new GLSLProgramObject();
      m_circle_shader->loadStrings(circ_vert_shader, circ_frag_shader);

      m_polyline_shader = new GLSLProgramObject();
      const char * polyline_frag = ""\
                          "varying float p;"\
                          "uniform float border_start;"\
                          "void main() {"\
                          "gl_FragColor = gl_Color;"\
                          "gl_FragColor.w *= smoothstep(1.0, border_start, abs(p));"\
                          "}";
      const char * polyline_vert = "attribute float coord;"\
                          "uniform float border_start;"\
                          "varying float p;"\
                          "void main() {"\
                          "p = coord;"\
                          "gl_Position = gl_ProjectionMatrix * gl_Vertex;"\
                          "gl_FrontColor = gl_Color;"\
                          "}";
      m_polyline_shader->loadStrings(polyline_vert, polyline_frag);
    }
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
    m_data->m_clipStack.push(r);
  }

  void RenderContext::popClipRect()
  {
    m_data->m_clipStack.pop();
  }

  bool RenderContext::isVisible(const Nimble::Rectangle & area)
  {
    if(m_data->m_clipStack.empty())
      return true;
    else
      return m_data->m_clipStack.top().intersects(area);
  }

  const Nimble::Rectangle & RenderContext::visibleArea() const
  {
    return m_data->m_clipStack.top();
  }

  void RenderContext::setScreenSize(Nimble::Vector2i size)
  {
    m_data->m_screenSize = size;
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
    FBOPackage * fbo = 0;

    for(Internal::FBOPackages::iterator it = m_data->m_fbos.begin();
    it != m_data->m_fbos.end(); it++) {
      fbo = (*it).ptr();

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
      fbo = new FBOPackage();
      Vector2i useSize = minimumsize;
      if(!(flags & FBO_EXACT_SIZE))
        useSize += minimumsize / 4;
      fbo->setSize(useSize);
      m_data->m_fbos.push_back(fbo);

      ret = FBOHolder(this, fbo);
    }

    /* We now have a valid FBO, next job is to set it up for rendering.
    */

    glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

    for(int i = 0; i < 6; i++)
      glDisable(GL_CLIP_PLANE0 + i);

    ret.m_package->activate();

    // Draw into color attachment 0
    glDrawBuffer(Luminous::COLOR0);

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

  void RenderContext::drawCircleImpl(Nimble::Vector2f center, float radius,
                                 const float * rgba) {
    const Matrix3f& m = transform();
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
    if(n < 2)
      return;

    width *= scale() * 0.5f;
    width += 1; // for antialiasing

    const Matrix3 & m = transform();
    std::vector<Nimble::Vector2> vertexArr;
    Vector2f cprev;
    Vector2f cnow = m.project(vertices[0]);
    Vector2f cnext;
    Vector2f avg;
    Vector2f dirNext;
    Vector2f dirPrev;

    int nextIdx = 1;
    while ((vertices[nextIdx]-cprev).length() < 3.0f && nextIdx < n-1) {
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


    vertexArr.push_back(cnow - avg);
    vertexArr.push_back(cnow + avg);

    for (int i = nextIdx; i < n; ) {
      nextIdx = i+1;
      cprev = cnow;      
      cnow = cnext;

      // at least 3 pixels gap between vertices
      while (nextIdx < n-1 && (vertices[nextIdx]-cnow).length() < 3.0f) {
        nextIdx++;
      }
      if (nextIdx > n-1) {
        cnext = (cnow - cprev) + cnow;
      } else {
        cnext = m.project(vertices[nextIdx]);
      }

      dirPrev = dirNext;
      dirNext = cnext - cnow;
      
      float max = Math::Max(1-(dirNext.length()/40.0f), 0.1f);
      
      if (dirNext.length() < 1e-5f) {
        dirNext = dirPrev;
      } else {
        dirNext.normalize();
      }

      Vector2 avg = (dirPrev + dirNext).perpendicular();
      avg.normalize();

      float dp = dot(avg, dirPrev.perpendicular());
      /// @todo: use bevel join if angle too small
      if (dp > 0.1f)
        avg /= Math::Max(dp, max);
      avg *= width;
      vertexArr.push_back(cnow-avg);
      vertexArr.push_back(cnow+avg);

      i = nextIdx;
    }

    GLuint loc = m_polyline_shader->getAttribLoc("coord");
    glEnableVertexAttribArray(loc);
    m_polyline_shader->bind();
    m_polyline_shader->setUniformFloat("border_start", 1.0f-1.0f/(width-0.5f));

    GLfloat * attribs = new GLfloat[vertexArr.size()];
    for (unsigned int i=0; i < vertexArr.size(); i++) {
      attribs[i] = i%2 == 0 ? 1.0f : -1.0f;
    }
    glColor4fv(rgba);    
    glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<GLfloat *>(&vertexArr[0]));
    glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, 0, attribs);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexArr.size());
    glDisableClientState(GL_VERTEX_ARRAY);
    m_polyline_shader->unbind();
    glDisableVertexAttribArray(loc);
    delete[] attribs;    
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

    glEnable(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_COORD_ARRAY);

    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<GLfloat*>(v));
    glDrawArrays(GL_QUADS, 0, 4);

    glDisable(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_COORD_ARRAY);

  }

  void RenderContext::drawTexRect(Nimble::Vector2 size, const float * rgba,
                                  Nimble::Vector2 texUV)
  {
    drawTexRect(size, rgba, Rect(Vector2(0,0), texUV));
  }

  void RenderContext::setBlendFunc(BlendFunc f)
  {
    if(f == BLEND_NONE) {
      glDisable(GL_BLEND);
      return;
    }

    glEnable(GL_BLEND);

    if(f == BLEND_USUAL)
      Utils::glUsualBlend();
    else if(f == BLEND_ADDITIVE)
      Utils::glAdditiveBlend();
    else if(f == BLEND_SUBTRACTIVE) {
      Utils::glSubtractiveBlend();
    }

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

  void RenderContext::clearTemporaryFBO(FBOPackage * fbo)
  {
    assert(fbo->userCount() == 0);

    fbo->m_fbo.unbind();

    fbo = m_data->popFBO(fbo);

    if(fbo) {
      fbo->activate();
    }
    else {
      // info("Back to back-buffer");
      glDrawBuffer(GL_BACK);
    }

    glPopAttrib();

    // Restore matrix stack
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    popTransform();
  }


}

