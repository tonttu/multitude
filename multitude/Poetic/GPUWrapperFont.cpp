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
#include "GPUWrapperFont.hpp"

namespace Poetic
{

  GPUWrapperFont::GPUWrapperFont(GPUManagedFont * gmf, CPUWrapperFont * cf)
    : m_gmf(gmf),
    m_cf(cf)
  {}

  GPUWrapperFont::~GPUWrapperFont()
  {}

  CPUFont * GPUWrapperFont::cpuFont() 
  {
    return m_cf;
  }

  void GPUWrapperFont::internalRender(const char * str, int n, const Nimble::Matrix3 & transform)
  {
    m_gmf->render(str, n, m_cf->faceSize(),
                  transform, m_cf->minimumRenderSize());
  }

  void GPUWrapperFont::internalRender(const wchar_t * str, int n, const Nimble::Matrix3 & transform)
  {
    m_gmf->render(str, n, m_cf->faceSize(),
                  transform, m_cf->minimumRenderSize());
  }

}

