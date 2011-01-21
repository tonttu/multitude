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

  void Path::drawStroke(Luminous::RenderContext &r)
  {
    // Generate line segments
    generateLineSegments();

    // Tessellate stroke
    SillyVB vb;
    tessellateStroke(vb);



    // Transform
    //    for(size_t i = 0; i < vb.size(); i++)
    //      vb[i] = r.project(vb[i]);

    // Draw
    //    glEnableClientState(GL_VERTEX_ARRAY);
    //    glVertexPointer(2, GL_FLOAT, 0, vb.data());
    //    glDrawArrays(GL_TRIANGLES, 0, vb.size());
    //    glDisableClientState(GL_VERTEX_ARRAY);
  }

  void Path::generateLineSegments()
  {
    if(!m_vertices.empty())
      return;

    m_vertices.clear();
    size_t vertexDataIndex = 0;

    // Start of current sub-path
    Nimble::Vector2f s(0.f, 0.f);
    // Last point of previous segment
    Nimble::Vector2f o(0.f, 0.f);
    // Last internal control point (for Bezier curves)
    Nimble::Vector2f p(0.f, 0.f);

    PathSegment prevSegment = MOVE_TO;
    // Has the current sub-path generated any geometry
    bool subpathHasGeometry = false;

    for(size_t i = 0; i < m_segments.size(); i++) {

      PathSegment segment = getPathSegment(m_segments[i]);

      switch(segment) {
      case CLOSE_PATH:
        addEndPath(o, s, subpathHasGeometry, CLOSE_PATH);
        p = s;
        o = s;
        subpathHasGeometry = false;
        break;
      case MOVE_TO:
        {
          Nimble::Vector2 c = m_data[vertexDataIndex++];

          if(prevSegment != CLOSE_PATH && prevSegment != MOVE_TO)
            addEndPath(o, s, subpathHasGeometry, IMPLICIT_CLOSE_PATH);

          s = c;
          p = c;
          o = c;
          subpathHasGeometry = false;
          break;
        }
      case LINE_TO:
        {
          Nimble::Vector2 c = m_data[vertexDataIndex++];

          if(addLineTo(o, c, subpathHasGeometry))
            subpathHasGeometry = true;

          p = c;
          o = c;
          break;
        }
      default:
        assert(0);
        break;
      }

      prevSegment = segment;
    }
  }

  void Path::addVertex(Nimble::Vector2 p, Nimble::Vector2 t, uint32_t flags)
  {
    Vertex v = { p, t, flags };

    m_vertices.push_back(v);
  }

  void Path::addEndPath(Nimble::Vector2 v0, Nimble::Vector2 v1, bool subpathHasGeometry, uint32_t flags)
  {
    // Subpath contains no geometry?
    if(!subpathHasGeometry) {
      // Nothing to do?

    } else {
      // Add segment from last vertex to start of subpath

      assert(!m_vertices.empty());

      // Flag last vertex
      m_vertices.back().flags |= END_SUBPATH;

      // Compute tangent
      Nimble::Vector2 tangent = (v1 - v0).normalize();

      // If tangent is zero, use tangent from the last segment end point
      if(tangent.isZero())
        tangent = m_vertices.back().tan;

      addEdge(v0, v1, tangent, tangent, flags | START_SEGMENT, flags | END_SEGMENT);
    }
  }

  void Path::addEdge(Nimble::Vector2 v0, Nimble::Vector2 v1, Nimble::Vector2 t0, Nimble::Vector2 t1, uint32_t begFlags, uint32_t endFlags)
  {
    addVertex(v0, t0, begFlags);
    addVertex(v1, t1, endFlags);
  }

  bool Path::addLineTo(Nimble::Vector2 v0, Nimble::Vector2 v1, bool subpathHasGeometry)
  {
    // Ignore degenerate segments
    if(v0 == v1)
      return false;

    // Compute tangent
    Nimble::Vector2 tangent = (v1 - v0).normalize();

    uint32_t begFlags = START_SEGMENT;

    if(!subpathHasGeometry)
      begFlags |= START_SUBPATH;

    addEdge(v0, v1, tangent, tangent, begFlags, END_SEGMENT);

    return true;
  }

  void Path::tessellateStroke(SillyVB &out)
  {
    // Need polyline to continue
    generateLineSegments();

    if(m_vertices.empty())
      return;

    Vertex vs;

    // Walk along the path
    for(size_t i = 0; i < m_vertices.size(); i++) {
      const Vertex & v = m_vertices[i];

      if(v.flags & START_SEGMENT) {

        if(v.flags & START_SUBPATH) {

          // Save subpath start point
          vs = v;
        } else {

          if(v.flags & IMPLICIT_CLOSE_PATH) {


          } else {

          }
        }
      } else {
        // In middle of a segment

        // Interpolate segment end points
      }

      if((v.flags & END_SEGMENT) && (v.flags & CLOSE_SUBPATH)) {



      }

      // v0 = v1
    }

  }

}
