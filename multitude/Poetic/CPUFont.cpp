/* COPYRIGHT
 */

#include "CPUFont.hpp"

#include "GPUFont.hpp"

#include <Luminous/RenderContext.hpp>

namespace Poetic
{

  GPUFont * CPUFont::getGPUFont()
  {
    Luminous::RenderContext * glr = Luminous::RenderContext::getThreadContext();
    Luminous::GLResource * gf = glr->getResource(this);

    if(gf) {
      GPUFont * font = static_cast<GPUFont *> (gf);
      assert(font);
      return font;
    }

    // puts("CPUFontBase::getGPUFont # New GPU font");

    GPUFont * font = createGPUFont();
    if(font) {
      glr->addResource(this, font);

      // m_gpuFonts.push_back(font);
    }

    return font;
  }

}

