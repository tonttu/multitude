#include "BezierSplineFitter.hpp"

#include <Radiant/Color.hpp>

namespace Luminous
{
  /// Generates a triangle strip approximation of a cubic Bezier spline
  class LUMINOUS_API BezierSplineTesselator
  {
  public:
    struct Vertex
    {
      Nimble::Vector2f location;
      Radiant::ColorPMA color;
    };

  public:
    /// @param maxCurveError Maximum error from the stroke center to the actual
    ///        Bezier curve, smaller values look better but also generate more vertices
    /// @param maxRoundCapError Maximum error when rendering round caps or round
    ///        joins between non-continuous curves. In most cases this should be
    ///        the same as maxCurveError, but for debugging reasons this can also be
    ///        adjusted separately.
    BezierSplineTesselator(float maxCurveError, float maxRoundCapError);
    ~BezierSplineTesselator();

    /// Tesselates the whole Bezier spline to a triangle strip
    void tesselate(const std::vector<BezierNode> & nodes, const Radiant::ColorPMA & color);

    const std::vector<Vertex> & triangleStrip() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
