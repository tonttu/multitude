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

#include "CPUFont.hpp"

#include "GPUFont.hpp"

#include <Luminous/GLResource.hpp>

namespace Poetic
{

  GPUFont * CPUFont::getGPUFont() 
  {
    Luminous::GLResources * glr = Luminous::GLResources::getThreadResources();
    Luminous::GLResource * gf = glr->getResource(this);

    if(gf) {
      GPUFont * font = dynamic_cast<GPUFont *> (gf);
      assert(font);
      return font;
    }

    // puts("CPUFontBase::getGPUFont # New GPU font");
    
    GPUFont * font = createGPUFont();
    assert(font != 0);
    
    glr->addResource(this, font);

    // m_gpuFonts.push_back(font);
    
    return font;
  }

}

