#include "Spline.hpp"

#include "ContextVariable.hpp"
#include "RenderContext.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"

#include <Nimble/Splines.hpp>

namespace
{
  static Luminous::Shader s_shader;

  class Point
  {
  public:
    Point()
    {
      m_location.clear();
      m_color.clear();
      m_width = 0.0f;
    }

    Nimble::Vector2 m_location;
    Nimble::Vector4 m_color;
    float           m_width;
  };

  class Vertex
  {
  public:

    Vertex()
    {
      m_location.clear();
      m_color.clear();
    }

    Nimble::Vector2 m_location;
    Nimble::Vector4 m_color;
  };

}

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

namespace Luminous {

  class Spline::D
  {
  public:

    D() : m_generation(0) {}

    void clear()
    {
      m_points.clear();
      m_vertices.clear();
      m_curve.clear();
      m_bounds.clear();
    }

    void addPoint(const Point & p);

    Point interpolate(float index) const;

    void recalculate();

    void render(Luminous::RenderContext &) const;


    std::vector<Point> m_points;
    std::vector<Vertex> m_vertices;

    Nimble::Interpolating m_curve;
    Nimble::Rect m_bounds;

    ContextVariableT<VertexBuffer> m_vbo;
    int m_generation;
  };

  void Spline::D::addPoint(const Point & p)
  {
    if(m_points.empty()) {
      m_bounds.set(p.m_location);
    }
    else {
      m_bounds.expand(p.m_location);
    }

    m_points.push_back(p);
    m_curve.add(p.m_location);
  }

  Point Spline::D::interpolate(float index) const
  {
    assert(Nimble::Math::isFinite(index));

    Point res;

    int indexi = (int) index;

    if(indexi >= ((int) m_points.size()) - 1) {
      if(m_points.empty())
        return res;
      else
        return m_points[m_points.size() - 1];
    }

    const Point & p1 = m_points.at(indexi);
    const Point & p2 = m_points.at(indexi + 1);

    float w2 = index - (float) indexi;
    float w1 = 1.0f - w2;

    res.m_width = p1.m_width * w1 + p2.m_width * w2;
    res.m_color = p1.m_color * w1 + p2.m_color * w2;
    res.m_location = p1.m_location * w1 + p2.m_location * w2;

    return res;
  }

  void Spline::D::recalculate()
  {
    m_vertices.clear();
    m_generation++;

    if(m_points.size() < 2) {
      return;
    }

    float step = 0.1f;
    const float len = m_curve.size();
    std::vector<Point> points;

    for(float t = 0.f; t < len - 1; t += step) {
      int ii = static_cast<int>(t);
      float t2 = t - ii;
      Nimble::Vector2 lov = m_curve.getPoint(ii, t2);

      if (points.size() >= 2) {
        Nimble::Vector2 p1 = (lov - points[points.size()-2].m_location);
        Nimble::Vector2 p2 = (lov - points[points.size()-1].m_location);
        p1.normalize();
        p2.normalize();
        // 3 degrees
        if (dot(p1, p2) > 0.99862953475457383) {
          points.pop_back();
        }
      }

      Point p = interpolate(t);
      p.m_location = lov;

      points.push_back(p);
    }

    std::vector<Point> & vertices = points;

    const float aapad = 1; // for antialiasing
    const float mingapSqr = 3.0f * 3.0f;

    const Point * p = & vertices[0];
    const int n = (int) vertices.size();

    Vector2f cprev;
    Vector2f cnow = p->m_location;
    Vector2f cnext;
    Vector2f avg;
    Vector2f dirNext;
    Vector2f dirPrev;

    int nextIdx = 1;
    while ((vertices[nextIdx].m_location - cnow).length() < mingapSqr && nextIdx < n-1) {
      nextIdx++;
    }

    cnext = vertices[nextIdx].m_location;
    dirNext = cnext - cnow;
    dirNext.normalize();
    avg = dirNext.perpendicular();

    if (avg.length() < 1e-5) {
      avg.make(1,0);
    } else {
      avg.normalize();
    }

    avg *= p->m_width * 0.5f;

    m_vertices.clear();

    Vertex v;
    v.m_color = p->m_color;
    v.m_location = cnow + avg;

    m_vertices.push_back(v);

    v.m_location = cnow - avg;
    m_vertices.push_back(v);

    for (int i = nextIdx; i < n; ) {
      nextIdx = i + 1;
      cprev = cnow;
      cnow = cnext;

      while (nextIdx < n-1 && (vertices[nextIdx].m_location - cnow).lengthSqr() < mingapSqr) {
        nextIdx++;
      }
      if (nextIdx > n-1) {
        cnext = 2.0f*cnow - cprev;
      } else {
        cnext = vertices[nextIdx].m_location;
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

      float dp = Nimble::Math::Clamp(dot(avg, dirPrev.perpendicular()), 1e-2f, 1.0f);
      avg /= dp;
      avg *= p->m_width * 0.5f;

      v.m_color = p->m_color;

      v.m_location = cnow - avg;
      m_vertices.push_back(v);

      v.m_location = cnow + avg;
      m_vertices.push_back(v);

      p = & vertices[nextIdx];
      i = nextIdx;
    }
  }

  void Spline::D::render(Luminous::RenderContext & r) const
  {
    if(m_vertices.empty())
      return;

    r.flush();

    s_shader.bind();

    bool created = false;
    VertexBuffer & vbo = m_vbo.ref(&r, & created);

    if(vbo.generation() != m_generation) {
      // Load new vertex data to the GPU
      vbo.fill( & m_vertices[0], m_vertices.size() * sizeof(Vertex), VertexBuffer::STATIC_DRAW);
      vbo.setGeneration(m_generation);
    }

    vbo.bind();

    GLSLProgramObject * prog = s_shader.bind();

    assert(prog != nullptr);

    prog->bind();
    prog->setUniformMatrix4("view_transform", r.viewTransform());
    prog->setUniformMatrix3("object_transform", r.transform());


    int aloc = prog->getAttribLoc("location");
    int acol = prog->getAttribLoc("color");

    assert(aloc >= 0 && acol >= 0);

    Vertex vr;
    const int vsize = sizeof(vr);

    VertexAttribArrayStep ls(aloc, 2, GL_FLOAT, vsize, offsetBytes(vr.m_location, vr));
    VertexAttribArrayStep cs(acol, 4, GL_FLOAT, vsize, offsetBytes(vr.m_color, vr));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, GLsizei(m_vertices.size()));

    // Radiant::info("Spline::D::render # %d", (int) m_vertices.size());
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////


  Spline::Spline()
    : m_d(nullptr)
  {
    {
      // Load the shader for rendering
      static Radiant::Mutex mutex;
      Radiant::Guard g(mutex);

      if(!s_shader.isDefined()) {
        QString fspath = RenderContext::locateStandardShader("spline.fs");
        QString vspath = RenderContext::locateStandardShader("spline.vs");
        s_shader.loadFragmentShader(fspath);
        s_shader.loadVertexShader(vspath);

        assert(s_shader.isDefined());
      }
    }
  }

  Spline::~Spline()
  {
    delete m_d;
  }

  Spline::Spline(Spline && spline) : m_d(spline.m_d)
  {
    spline.m_d = nullptr;
  }

  Spline & Spline::operator=(Spline && spline)
  {
    delete m_d;
    m_d = spline.m_d;
    spline.m_d = nullptr;
    return *this;
  }

  void Spline::addControlPoint(Nimble::Vector2 point, Nimble::Vector4 color, float width)
  {
    Point p;
    p.m_location = point;
    p.m_color = color;
    p.m_width = width;

    if(!m_d)
      m_d = new D();
    m_d->addPoint(p);
  }

  void Spline::clear()
  {
    if(m_d)
      m_d->clear();
  }

  void Spline::render(Luminous::RenderContext & r) const
  {
    if(m_d)
      m_d->render(r);
  }

  void Spline::recalculate()
  {
    if(m_d)
      m_d->recalculate();
  }
}
