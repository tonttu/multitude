/* COPYRIGHT
 *
 * This file is part of Poetic.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Poetic.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */
#ifndef POETIC_GPU_WRAPPER_FONT_HPP
#define POETIC_GPU_WRAPPER_FONT_HPP

#include "Export.hpp"
#include "GPUFont.hpp"
#include "CPUWrapperFont.hpp"
#include "GPUManagedFont.hpp"

#include <Luminous/GLResource.hpp>

namespace Poetic
{

  /// A wrapper class for convenience
  class POETIC_API GPUWrapperFont : public GPUFont
  {
  public:
    /// Constructs a new GPU wrapper font
    GPUWrapperFont(GPUManagedFont * gf, CPUWrapperFont * cf);
    ~GPUWrapperFont();

    CPUFont * cpuFont();

  protected:
    void internalRender(const char * str, int n, const Nimble::Matrix3 & transform);
    void internalRender(const wchar_t * str, int n, const Nimble::Matrix3 & transform);

  private:
    GPUManagedFont * m_gmf;
    CPUWrapperFont * m_cf;
  };

}

#endif
