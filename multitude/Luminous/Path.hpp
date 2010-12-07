#ifndef LUMINOUS_PATH_HPP
#define LUMINOUS_PATH_HPP

#include "RenderContext.hpp"

#include <Nimble/Vector2.hpp>

#include <vector>

namespace Luminous
{
  typedef enum {
    CLOSE_PATH  = 0,
    MOVE_TO     = 1,
    LINE_TO     = 2
  } PathSegment;

  class Path
  {
  public:
    Path();

    typedef std::vector<Nimble::Vector2f> Contour;
    typedef std::vector<Nimble::Vector2f> SillyVB;

    void closePath();
    void moveTo(Nimble::Vector2f p);
    void lineTo(Nimble::Vector2f p);

    // Computes the contour polyline
    bool computeContour(Contour & out);

    // Draw in here to avoid modifying RenderContext for now
    void draw(Luminous::RenderContext & r);

    float strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(float w) { m_strokeWidth = w; }

  private:
    // Generate the stroke geometry for rendering
    void tesselateStroke(const Contour & out, SillyVB & vb) const;

    // Segment definitions
    std::vector<PathSegment> m_segments;
    // Segment data matching m_segments
    std::vector<Nimble::Vector2f> m_data;

    /// Width of the stroke
    float m_strokeWidth;
  };

}

#endif // PATH_HPP
