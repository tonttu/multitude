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
#ifndef POETIC_GPU_FONT_BASE_HPP
#define POETIC_GPU_FONT_BASE_HPP

#include <Nimble/Matrix3.hpp>

#include <Poetic/Export.hpp>
#include <Poetic/GPUFont.hpp>
#include <Poetic/CPUFontBase.hpp>

namespace Poetic
{
  class GlyphContainer;
  class Glyph;

  /// A base class for different fonts that reside on the GPU
  class POETIC_API GPUFontBase : public GPUFont
  {
    public:
      GPUFontBase(CPUFontBase * font);
      virtual ~GPUFontBase();

      CPUFont * cpuFont() { return dynamic_cast<CPUFont *> (m_cpuFont); }

    protected:
      CPUFontBase * m_cpuFont;

      virtual Glyph * makeGlyph(const Glyph * cpuGlyph) = 0;

      virtual void faceSizeChanged();

      virtual void internalRender(const char * str, int n, const Nimble::Matrix3 & transform);
      virtual void internalRender(const wchar_t * str, int n, const Nimble::Matrix3 & transform);

    private:
      inline bool checkGlyph(unsigned int charCode);

      GlyphContainer * m_glyphList;
      Nimble::Vector2 m_pen;      

      friend class CPUFontBase;
  };

}

#endif
