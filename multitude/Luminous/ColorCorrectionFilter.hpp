/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef COLORCORRECTIONFILTER_HPP
#define COLORCORRECTIONFILTER_HPP

#include <Luminous/PostProcessFilter.hpp>

namespace Luminous
{

  /// This class implements the built-in color-correction in Cornerstone. It is
  /// implemented as a post-processing filter.
  class LUMINOUS_API ColorCorrectionFilter : public Luminous::PostProcessFilter
  {
  public:

    ColorCorrectionFilter();
    virtual ~ColorCorrectionFilter();

    void filter(Luminous::RenderContext & rc,
                Luminous::PostProcessContext & ctx,
                Luminous::Style style) const;    


  private:
    class D;
    D * m_d;
  };
}
#endif // COLORCORRECTIONFILTER_HPP
