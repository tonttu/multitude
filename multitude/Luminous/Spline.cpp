/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Luminous/Spline.hpp"
#include "Luminous/RenderContext.hpp"
#include "Luminous/Program.hpp"
#include "Luminous/VertexDescription.hpp"

#include <Nimble/Splines.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Vector3.hpp>
#include <Nimble/Vector4.hpp>

namespace
{
  class Point
  {
  public:
    Point()
      : m_location(0, 0)
      , m_range(0, 10000)
      , m_color(0, 0, 0, 0)
      , m_width(0)
    {
    }

    Nimble::Vector2   m_location;
    /* valid time range when this point should be visible, [m_range.x, m_range.y) */
    Nimble::Vector2f  m_range;
    Radiant::Color    m_color;
    float             m_width;
  };

  class Vertex
  {
  public:
    Vertex()
      : location(0, 0)
      , range(0, 10000)
      , color(0, 0, 0, 0)
    {
    }

    Nimble::Vector2f location;
    Nimble::Vector2f range;
    Nimble::Vector4f color;
  };

  struct UniformBlock : public Luminous::BasicUniformBlock
  {
    float time;
  };
}

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

namespace Luminous {

  class Spline::D
  {
  public:
    struct Path
    {
      std::vector<Point> points;
      Nimble::Interpolating curve;
      Nimble::Rectf bounds;
    };

    D()
      : m_redoLocation()
      , m_endTime(0)
      , m_mingap(2.0f)
      , m_maxgap(3.0f)
      , m_generation(0)
    {
      /// @todo should share these among instances of the spline   
      m_shader.loadShader("Luminous/GLSL150/spline.fs", Luminous::Shader::Fragment);
      m_shader.loadShader("Luminous/GLSL150/spline.vs", Luminous::Shader::Vertex);

      m_descr.addAttribute<Nimble::Vector2f>("vertex_position");
      m_descr.addAttribute<Nimble::Vector2f>("vertex_range");
      m_descr.addAttribute<Nimble::Vector4f>("vertex_color");
      m_shader.setVertexDescription(m_descr);
      m_shader.setTranslucency(true);
    }

    void clear()
    {
      m_paths.clear();
      m_vertices.clear();
      m_bounds.clear();
      m_active = -1;
      m_redoLocation.pathIndex = 0;
      m_redoLocation.nextPointIndex = 0;
      ++m_generation;
    }

    void addPoint(const Point & p);
    void endPath();
    void erase(const Nimble::Rectangle &eraser, float time, bool permanent);

    Point interpolate(const Path & path, float index) const;

    void recalculate();
    void recalculate(const Path & path);

    void render(Luminous::RenderContext & r, float time) const;

    int undoRedo(int points);

  public:
    /// timestamp -> vertices
    std::map<float, std::size_t> m_index;
    std::vector<Vertex> m_vertices;
    std::vector<Path> m_paths;
    int m_active = -1;

    struct {
      int pathIndex;
      int nextPointIndex;
    } m_redoLocation;
    float m_endTime;
    float m_mingap;
    float m_maxgap;

    Nimble::Rect m_bounds;
    
    std::size_t m_generation;

    Luminous::VertexDescription m_descr;
    Luminous::Program m_shader;
  };

  void Spline::D::addPoint(const Point & p)
  {
    /// @todo if the last added point is too far away from this one, we should
    ///       use curve to calculate some immediate points (for erasing)
    m_bounds.expand(p.m_location);

    if(m_active < 0 || m_active >= m_paths.size()) {
      m_paths.resize(m_paths.size()+1);
      m_active = m_paths.size() -1;
    }

    auto & path = m_paths[m_active];
    path.points.push_back(p);
    path.curve.add(p.m_location);
    path.bounds.expand(p.m_location);
    m_endTime = std::max(m_endTime, p.m_range.x);
    m_redoLocation.pathIndex = m_paths.size() - 1;
    m_redoLocation.nextPointIndex = path.points.size();
  }

  void Spline::D::endPath()
  {
    m_active = -1;
  }

  void Spline::D::erase(const Nimble::Rectangle & eraser, float time, bool permanent)
  {
    if(!eraser.intersects(m_bounds))
      return;

    bool changed = false;
    for(std::size_t i = 0; i < m_paths.size(); ++i) {
      if(!eraser.intersects(m_paths[i].bounds))
        continue;

      std::vector<Point> & points = m_paths[i].points;

      if(permanent) {
        int validPoints = 0;
        for(std::size_t j = 0; j < points.size(); ++j) {
          Point & p = points[j];
          if(p.m_range.y > 0.0f) {
            if(eraser.isInside(p.m_location)) {
              p.m_range.y = 0;
              changed = true;
            } else ++validPoints;
          }
        }
        if(validPoints < 2) {
          if (m_active == i)
            m_active = -1;
          else if (m_active == m_paths.size() -1)
            m_active = i;
          std::swap(m_paths[i], m_paths.back());
          m_paths.resize(m_paths.size()-1);
          --i;
        }
      } else {
        for(std::size_t j = 0; j < points.size(); ++j) {
          Point & p = points[j];
          if(p.m_range.x <= time && p.m_range.y > time && eraser.isInside(p.m_location)) {
            p.m_range.y = time;
            changed = true;
          }
        }
      }
    }
    /// @todo could just change m_vertices directly? or make some kind of light-version of recalculate?
    if(changed)
      recalculate();
  }

  Point Spline::D::interpolate(const Path & path, float index) const
  {
    assert(Nimble::Math::isFinite(index));

    Point res;

    int indexi = (int) index;

    if(indexi >= ((int) path.points.size()) - 1) {
      if(path.points.empty())
        return res;
      else
        return path.points.back();
    }

    const Point & p1 = path.points.at(indexi);
    const Point & p2 = path.points.at(indexi + 1);

    float w2 = index - (float) indexi;
    float w1 = 1.0f - w2;

    res.m_width = p1.m_width * w1 + p2.m_width * w2;
    res.m_color = p1.m_color * w1 + p2.m_color * w2;
    res.m_range = p1.m_range * w1 + p2.m_range * w2;
    res.m_location = p1.m_location * w1 + p2.m_location * w2;

    return res;
  }

  void Spline::D::recalculate()
  {
    m_vertices.clear();
    m_index.clear();
    m_generation++;

    for(std::size_t i = 0; i < m_paths.size(); ++i)
      recalculate(m_paths[i]);
  }

  void Spline::D::recalculate(const Path & path)
  {
    if(path.points.size() < 2) {
      return;
    }

    float step = 0.1f;
    const float len = path.curve.size();
    std::vector<Point> points;

    const float mingapSqr = m_mingap * m_mingap;
    // erasing works much better if there is a maximum gap
    const float maxgapSqr = m_maxgap * m_maxgap;

    for(float t = 0.f; t < len - 1; t += step) {
      int ii = static_cast<int>(t);
      float t2 = t - ii;
      Nimble::Vector2 lov = path.curve.getPoint(ii, t2);

      if (points.size() >= 2) {
        Nimble::Vector2 p1 = (lov - points[points.size()-2].m_location);
        Nimble::Vector2 p2 = (lov - points[points.size()-1].m_location);
        const float p1len2 = p1.lengthSqr();
        p1.normalize();
        p2.normalize();
        // 3 degrees
        if ((dot(p1, p2) > 0.99862953475457383 || p1len2 < mingapSqr) && (p1len2 < maxgapSqr)) {
          points.pop_back();
        }
      }

      Point p = interpolate(path, t);
      p.m_location = lov;

      points.push_back(p);
    }

    const int n = (int) points.size();

    Nimble::Vector2f cprev;
    Nimble::Vector2f cnow = points[0].m_location;
    Nimble::Vector2f cnext;
    Nimble::Vector2f avg;
    Nimble::Vector2f dirNext;
    Nimble::Vector2f dirPrev;

    cnext = points[1].m_location;
    dirNext = cnext - cnow;
    dirNext.normalize();
    avg = dirNext.perpendicular();

    if (avg.length() < 1e-5) {
      avg.make(1,0);
    } else {
      avg.normalize();
    }

    avg *= points[0].m_width * 0.5f;

    Vertex v;
    v.color = points[0].m_color;
    v.location = cnow - avg;
    v.range = points[0].m_range;

    if(!m_vertices.empty()) {
      // degenerated triangle
      m_vertices.push_back(m_vertices.back());
      m_vertices.push_back(v);
    }

    m_vertices.push_back(v);

    v.location = cnow + avg;
    m_vertices.push_back(v);

    for (int i = 1; i < n; ++i) {
      const Point & p = points[i];

      cprev = cnow;
      cnow = cnext;

      if (i+1 > n-1) {
        cnext = 2.0f*cnow - cprev;
      } else {
        cnext = points[i+1].m_location;
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

      float dp = Nimble::Math::Clamp(dot(avg, dirPrev.perpendicular()), 0.7f, 1.0f);
      avg /= dp;
      avg *= p.m_width * 0.5f;

      v.range = p.m_range;
      v.color = p.m_color;

      v.location = cnow - avg;
      m_vertices.push_back(v);

      v.location = cnow + avg;
      m_vertices.push_back(v);

      /// Doesn't really need the index on every vertex, lets do it every 50 vertex
      /// (m_vertices should be always even here)
      if(m_vertices.size() % 50 == 0) {
        m_index[v.range.x] = m_vertices.size();
      }
    }

    m_index[v.range.x] = m_vertices.size();
  }

  void Spline::D::render(Luminous::RenderContext & r, float time) const
  {
    // Nothing to render
    if(m_vertices.empty())
      return;

    /// @todo Currently quite inefficient since it recreates the buffer all the time
    auto it = m_index.lower_bound(time);
    // Check if there even are any vertices ready at this time
    if(it == m_index.begin() && it->first > time)
      return;

    const std::size_t vertices = (it == m_index.end() ? m_vertices.size() : it->second);

    /// @todo Should we be able to overrule this with Style::Translucent
    /// @todo Guess this depends on all the vertex colors
    bool translucent =
      m_shader.translucent();

    auto b = r.render<Vertex, UniformBlock>(translucent, Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, vertices, 1.f, m_shader);
    std::copy(m_vertices.begin(), m_vertices.end(), b.vertex);

    /// @todo what color to use here?
    b.uniform->color = Nimble::Vector4f(1,1,1,r.opacity());
    b.uniform->time = time;
    b.uniform->depth = b.depth;
  }

  int Spline::D::undoRedo(int points)
  {
    int changes = 0;
    if(m_redoLocation.pathIndex >= int(m_paths.size()))
      m_redoLocation.pathIndex = int(m_paths.size()) - 1;
    if(m_redoLocation.pathIndex < 0)
      return 0;

    // undo
    while(points < 0) {
      if(m_redoLocation.nextPointIndex <= 0) {
        if(m_redoLocation.pathIndex <= 0)
          break;
        --m_redoLocation.pathIndex;
        m_redoLocation.nextPointIndex = m_paths[m_redoLocation.pathIndex].points.size();
      }
      int diff = std::min(m_redoLocation.nextPointIndex, -points);
      points += diff;
      changes += diff;
      Path & path = m_paths[m_redoLocation.pathIndex];
      for(int i = m_redoLocation.nextPointIndex - diff; i < m_redoLocation.nextPointIndex; ++i) {
        Point & point = path.points[i];
        point.m_range.y = std::min(point.m_range.y, -point.m_range.y);
      }
      m_redoLocation.nextPointIndex -= diff;
    }

    // redo
    while(points > 0) {
      if(m_redoLocation.nextPointIndex >= int(m_paths[m_redoLocation.pathIndex].points.size())) {
        if(m_redoLocation.pathIndex+2 >= int(m_paths.size()))
          break;
        ++m_redoLocation.pathIndex;
        m_redoLocation.nextPointIndex = 0;
      }
      int diff = std::min(int(m_paths[m_redoLocation.pathIndex].points.size()) -
          m_redoLocation.nextPointIndex, points);
      points -= diff;
      changes += diff;
      Path & path = m_paths[m_redoLocation.pathIndex];
      for(int i = m_redoLocation.nextPointIndex; i < m_redoLocation.nextPointIndex + diff; ++i) {
        Point & point = path.points[i];
        point.m_range.y = std::max(point.m_range.y, -point.m_range.y);
      }
      m_redoLocation.nextPointIndex += diff;
    }
    return changes;
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////


  Spline::Spline()
    : m_d(new Spline::D)
  {
  }

  Spline::~Spline()
  {
    delete m_d;
  }

  Spline::Spline(Spline && spline)
    : m_d(spline.m_d)
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

  void Spline::addControlPoint(Nimble::Vector2 point, Nimble::Vector4 color, float width, float time)
  {
    Point p;
    p.m_location = point;
    p.m_width = width;
    p.m_range[0] = std::max(time, 0.0f);
    p.m_color = color;

    if(!m_d)
      m_d = new D();
    m_d->addPoint(p);
  }

  void Spline::endPath()
  {
    if(m_d)
      m_d->endPath();
  }

  void Spline::clear()
  {
    if(m_d)
      m_d->clear();
  }

  void Spline::erase(const Nimble::Rectangle & eraser, float time)
  {
    if(m_d)
      m_d->erase(eraser, time, false);
  }

  void Spline::erasePermanent(const Nimble::Rectangle &eraser)
  {
    m_d->erase(eraser, 0.0f, true);
  }

  void Spline::render(Luminous::RenderContext & r, float time) const
  {
    m_d->render(r, time);
  }

  void Spline::setCalculationParameters(float mingap, float maxgap)
  {
    m_d->m_mingap = mingap;
    m_d->m_maxgap = maxgap;
  }

  void Spline::recalculate()
  {
    m_d->recalculate();
  }

  float Spline::beginTime() const
  {
    if(!m_d->m_paths.empty()) {
      std::vector<Point> & points = m_d->m_paths[0].points;
      if(!points.empty())
        return points[0].m_range.x;
    }
    return 0.0f;
  }

  float Spline::endTime() const
  {
    return m_d->m_endTime;
  }

  int Spline::undoRedo(int points)
  {
    int changes = m_d->undoRedo(points);
    if(changes > 0)
      recalculate();
    return changes;
  }

  size_t Spline::controlPointCount() const
  {
    if(m_d->m_active < 0 || m_d->m_active >= m_d->m_paths.size())
      return 0;
    return m_d->m_paths[m_d->m_active].points.size();
  }

  Nimble::Rect Spline::controlPointBounds() const
  {
    return m_d->m_bounds;
  }

  bool Spline::isEmpty() const
  {
    return m_d->m_paths.empty();
  }

  QDataStream & operator<<(QDataStream & out, const Spline & spline)
  {
    const qint64 hdr = 0;
    const qint64 paths = spline.m_d->m_paths.size();
    out << hdr << paths;

    for(long i = 0; i < paths; ++i) {
      const std::vector<Point> & points = spline.m_d->m_paths[i].points;
      const qint64 size = points.size();
      out << size;
      out.writeRawData(reinterpret_cast<const char*>(points.data()), size * sizeof(points[0]));
    }
    return out;
  }

  QDataStream & operator>>(QDataStream & in, Spline & spline)
  {
    spline.clear();
    qint64 hdr = 0;
    qint64 paths = 0;
    in >> hdr >> paths;

    std::vector<Point> points;
    for(long i = 0; i < paths; ++i) {
      qint64 size = 0;
      in >> size;
      if (in.status() != QDataStream::Ok) {
        spline.clear();
        return in;
      }
      points.resize(size);
      in.readRawData(reinterpret_cast<char*>(points.data()), size * sizeof(points[0]));
      if (in.status() != QDataStream::Ok) {
        spline.clear();
        return in;
      }
      for(std::size_t j = 0; j < points.size(); ++j)
        spline.m_d->addPoint(points[j]);
      spline.m_d->endPath();
    }
    spline.recalculate();
    return in;
  }
}
