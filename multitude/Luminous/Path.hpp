#ifndef LUMINOUS_PATH_HPP
#define LUMINOUS_PATH_HPP

#include "RenderContext.hpp"

#include <Nimble/Vector2.hpp>

#include <vector>

namespace Luminous
{
  typedef enum {
    CLOSE_PATH  = (0 << 1),
    MOVE_TO     = (1 << 1),
    LINE_TO     = (2 << 1)
  } PathSegment;

  class Path
  {
  private:
    // Placeholder VB for debugging
    typedef std::vector<Nimble::Vector2f> SillyVB;

    // Vertex used to store Paths
    struct Vertex
    {
      // Position
      Nimble::Vector2f pos;
      // Tangent
      Nimble::Vector2f tan;
//      float pathLen;
      uint32_t flags;
    };

    enum VertexFlags {
      START_SUBPATH       = (1 << 0),
      END_SUBPATH         = (1 << 1),
      START_SEGMENT       = (1 << 2),
      END_SEGMENT         = (1 << 3),
      CLOSE_SUBPATH       = (1 << 4),
      IMPLICIT_CLOSE_PATH = (1 << 5)
    };

    struct StrokeVertex
    {

    };

    // Helpers
    static PathSegment getPathSegment(uint8_t data) { return (PathSegment)(data & 0x1e); }

    //
    void generateLineSegments();
    void tessellateStroke(SillyVB & out);

    void addVertex(Nimble::Vector2 p, Nimble::Vector2 t, uint32_t flags);
    // Add end-path edge between v0 & v1
    void addEndPath(Nimble::Vector2 v0, Nimble::Vector2 v1, bool subpathHasGeometry, uint32_t flags);
    // Add edge between  v0 and v1
    void addEdge(Nimble::Vector2 v0, Nimble::Vector2 v1, Nimble::Vector2 t0, Nimble::Vector2 t1, uint32_t begFlags, uint32_t endFlags);
    // Add line between v0 and v1
    bool addLineTo(Nimble::Vector2 v0, Nimble::Vector2 v1, bool subpathHasGeometry);

    // Segment definitions
    std::vector<uint8_t> m_segments;
    // Segment data matching m_segments
    std::vector<Nimble::Vector2f> m_data;

    /// Width of the stroke
    float m_strokeWidth;

    // Tesselated geometry
    std::vector<Vertex> m_vertices;

  public:
    Path();

    void closePath();
    void moveTo(Nimble::Vector2f p);
    void lineTo(Nimble::Vector2f p);

    // Draw in here to avoid modifying RenderContext for now
    void drawStroke(Luminous::RenderContext & r);

    /// Returns the stroke width
    float strokeWidth() const { return m_strokeWidth; }
    /// Sets the stroke width
    void setStrokeWidth(float w) { m_strokeWidth = w; }
 };

}

#endif // PATH_HPP
