#ifndef LUMINOUS_PATH_HPP
#define LUMINOUS_PATH_HPP

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
  public:
    Path();

    typedef std::vector<Nimble::Vector2f> Countour;

    void closePath();
    void moveTo(Nimble::Vector2f p);
    void lineTo(Nimble::Vector2f p);

    bool tesselate(Countour & out);

  private:
    // Tesselation stuff
    void generateContour(Contour & c) const;

    static float area(const Countour & c);
    static bool insideTriangle(Nimble::Vector2 a, Nimble::Vector2 b, Nimble::Vector2 c, Nimble::Vector2 p);
    static bool snip(const Countour & c, int u, int v, int w, int n, int * V);

    // Segment definitions
    std::vector<PathSegment> m_segments;
    // Segment data matching m_segments
    std::vector<Nimble::Vector2f> m_data;
  };

}

#endif // PATH_HPP
