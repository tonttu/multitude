#include "Spline.hpp"

#include "ContextVariable.hpp"
#include "RenderContext.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"

#include <Nimble/Splines.hpp>

namespace Luminous {

  static Shader s_shader;

  class Spline::Point
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

  class Spline::Vertex
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

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

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

    void update();

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

  void Spline::D::update()
  {
    m_vertices.clear();
    m_generation++;

    if(m_points.size() < 2) {
      return;
    }

    /*
    float step = 0.25f;
    const float len = m_curve.size();
    std::vector<Point> points;

    for(float t = 0.f; t < len - 1; t += step) {
      int ii = static_cast<int>(t);
      float t2 = t - ii;
      Vector2 point = m_curve.getPoint(ii, t2);

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

    */

    std::vector<Point> & vertices = m_points;

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

    avg *= p->m_width;

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
      avg *= p->m_width;

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
    if(created) {

    }

    if(vbo.generation() != m_generation) {
      // Load new vertex data to the GPU
      vbo.fill( & m_vertices[0], m_vertices.size() * sizeof(Vertex), VertexBuffer::STATIC_DRAW);
    }

    vbo.setGeneration(m_generation);

    vbo.bind();
  }


  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////


  Spline::Spline()
    : m_d(new D())
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
      }
    }
  }

  Spline::~Spline()
  {
    delete m_d;
  }

  void Spline::addControlPoint(Nimble::Vector2 point, Nimble::Vector4 color, float width)
  {
    Point p;
    p.m_location = point;
    p.m_color = color;
    p.m_width = width;

    m_d->addPoint(p);
  }

  void Spline::clear()
  {
    m_d->clear();
  }

  void Spline::render(Luminous::RenderContext & r) const
  {
    m_d->render(r);
  }

}
