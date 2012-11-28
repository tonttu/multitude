#ifndef COLORCORRECTIONFILTER_HPP
#define COLORCORRECTIONFILTER_HPP

#include <Luminous/PostProcessContext.hpp>

namespace Luminous
{

  /// This class implements the built-in color-correction in Cornerstone. It is
  /// implemented as a post-processing filter.
  class LUMINOUS_API ColorCorrectionFilter : public Luminous::PostProcessContext
  {
  public:
    ColorCorrectionFilter();
    virtual ~ColorCorrectionFilter();

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
