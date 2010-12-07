#include "Path.hpp"

namespace Luminous
{

  Path::Path():
      m_strokeWidth(1.f)
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

  bool Path::computeContour(Contour & out)
  {
    size_t coordCount = 0;

    Nimble::Vector2f pen(0.f, 0.f);

    out.push_back(pen);

    for(size_t s = 0, d = 0; s < m_segments.size(); s++, d += coordCount) {
        PathSegment cmd = m_segments[s];

        //coordCount = coordsPerCommand(cmd);

        // Output pen location
        //out.push_back(pen);

        // Unpack coordinates from data
        //for(size_t i = 0; i < coordCount; i++)
          //out.push_back(m_data[d + i]);

        // Handle segment
        switch(cmd) {
        case LINE_TO:
          out.push_back(m_data[d]);
          break;
        default:
          assert(0);
        }
    }

    return true;
  }


  void Path::draw(Luminous::RenderContext &r)
  {
    // Compute contour
    Contour c;
    computeContour(c);

    // Tesselate stroke
    SillyVB vb;
    tesselateStroke(c, vb);

    // Transform
    for(size_t i = 0; i < vb.size(); i++)
      vb[i] = r.project(vb[i]);

    // Draw
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vb.data());
    glDrawArrays(GL_TRIANGLES, 0, vb.size());
    glDisableClientState(GL_VERTEX_ARRAY);
  }

  void Path::tesselateStroke(const Contour & contour, SillyVB & vb) const
  {
    assert(contour.size() > 1);

    Nimble::Vector2f dPrev(0.f, 0.f);

    for(size_t i1 = 0; i1 < contour.size() - 1; i1++) {

      size_t i2 = i1 + 1;

      Nimble::Vector2f v1 = contour[i1];
      Nimble::Vector2f v2 = contour[i2];

      // Direction vector
      Nimble::Vector2f d = v2 - v1;

      float len = d.length();
      if(len == 0.f)
        d = dPrev;
      else
        d /= len;

      // Perpendicular vector
      Nimble::Vector2f t(-d.y, d.x);
      t *= m_strokeWidth / 2.f;

      Nimble::Vector2f l1 = v1 + t;
      Nimble::Vector2f r1 = v1 - t;

      Nimble::Vector2f l2 = v2 + t;
      Nimble::Vector2f r2 = v2 - t;

      vb.push_back(l2);
      vb.push_back(l1);
      vb.push_back(r1);

      vb.push_back(r1);
      vb.push_back(r2);
      vb.push_back(l2);
    }
  }
}
