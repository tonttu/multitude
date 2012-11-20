#ifndef COLORCORRECTIONFILTER_HPP
#define COLORCORRECTIONFILTER_HPP

#include <Luminous/PostProcessFilter.hpp>

namespace Luminous
{
  class LUMINOUS_API ColorCorrectionFilter : public Luminous::PostProcessFilter
  {
  public:
    ColorCorrectionFilter();
    virtual ~ColorCorrectionFilter();

    /// Returns the RGBCube used for the color correction. By default the RGBCube
    /// from the currently active area is used.
    virtual const Luminous::RGBCube * rgbCube() const;

    /// Retrieves the RGBCube for the current area from the render context
    void begin(Luminous::RenderContext & rc) OVERRIDE;

    /// Returns the style used for rendering color corrected images
    Luminous::Style style() const;

  private:
    class D;
    D * m_d;
  };
}
#endif // COLORCORRECTIONFILTER_HPP
