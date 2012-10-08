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

    void apply(Luminous::RenderContext & rc) OVERRIDE;

  private:
    class D;
    D * m_d;
  };
}
#endif // COLORCORRECTIONFILTER_HPP
