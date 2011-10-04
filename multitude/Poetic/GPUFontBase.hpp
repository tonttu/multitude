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

#include "Export.hpp"
#include "GPUFont.hpp"
#include "CPUFontBase.hpp"

#include <Nimble/Matrix3.hpp>

namespace Poetic
{
  class GlyphContainer;
  class Glyph;

  /// A base class for different fonts that reside on the GPU
  class POETIC_API GPUFontBase : public GPUFont
  {
    public:
      /// Constructs a new GPU font
      GPUFontBase(CPUFontBase * font);
      virtual ~GPUFontBase();

      /// Returns the CPU font
      CPUFont * cpuFont() { return static_cast<CPUFont *> (m_cpuFont); }

    protected:
      /// CPU font that this GPU font uses
      CPUFontBase * m_cpuFont;

      /// Creates a new glyph
      virtual Glyph * makeGlyph(const Glyph * cpuGlyph) = 0;

      /// Callback to notify that the face size has been changed
      virtual void faceSizeChanged();

      /// Performs the actual rendering
      virtual void internalRender(const char * str, int n, const Nimble::Matrix3 & transform, Nimble::Vector2f ** ptr);
      /// @copybrief internalRender
      virtual void internalRender(const wchar_t * str, int n, const Nimble::Matrix3 & transform, Nimble::Vector2f ** ptr);
      /// Get the location where the pen finished.
      float getLastAdvance() { return m_pen.x; }
    private:
      inline bool checkGlyph(unsigned int charCode);

      GlyphContainer * m_glyphList;
      Nimble::Vector2 m_pen;

      friend class CPUFontBase;
  };

}

#endif
