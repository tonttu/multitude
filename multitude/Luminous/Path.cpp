#include "Path.hpp"

namespace Luminous
{

  Path::Path()
  {
  }

  void Path::closePath()
  {
    m_segments.push_back(CLOSE_PATH);
    //m_data.push_back(Nimble::Vector2f(0, 0));
  }

  void Path::lineTo(Nimble::Vector2f p)
  {
    m_segments.push_back(LINE_TO);
    m_data.push_back(p);
  }

  void Path::moveTo(Nimble::Vector2f p)
  {
    m_segments.push_back(MOVE_TO);
    m_data.push_back(p);
  }

  void Path::tesselate(Triangles &out)
  {

  }

  ////////// Tesselation //////////

  float Path::area(const Countour &c)
  {
    size_t n = c.size();
    float area = 0.f;

    for(size_t p = n - 1, q = 0; q < n; p = q++)
      area += c[p].x * c[q].y - c[q].x * c[p].y;

    return area;
  }

  bool Path::insideTriangle(Nimble::Vector2 a, Nimble::Vector2 b, Nimble::Vector2 c, Nimble::Vector2 p)
  {
    Nimble::Vector2 ap = p - a;
    Nimble::Vector2 bp = p - b;
    Nimble::Vector2 cp = p - c;

    float aCP = a.x * bp.y - a.y * bp.x;
    float bCP = b.x * cp.y - b.y * cp.x;
    float cCP = c.x * ap.y - c.y * ap.x;

    return ((aCP >= 0.f) && (bCP >= 0.f) && (cCP >= 0.f));
  }

  bool Path::snip(const Countour &c, int u, int v, int w, int n, std::vector<int> & V)
  {
     Nimble::Vector2 a = contour[v[u]];
     Nimble::Vector2 b = contour[V[v]];
     Nimble::Vector2 c = contour[V[w]];

     if ( Nimble::Math::TOLERANCE > (((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x))) )
       return false;

     for (int p = 0; p < n; p++)
     {
       if( (p == u) || (p == v) || (p == w) )
         continue;

       Nimble::Vector2 p = contour[V[p]];

       if (insideTriangle(a, b, c, p))
         return false;
     }

     return true;
  }

  void Path::generateContour(Contour &c) const
  {
    c.resize(m_segments.size());

    for(size_t i = 0; i < m_segments.size(); i++) {

    }
  }

  bool Path::tesselate(Countour &out) const
  {
      size_t n = m_segments.size() + 1;
      if(n < 3)
        return false;

      std::vector<int> V(n);

      // CCW polygons in V
      if(0.f < area())

  }

}
