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
#ifndef POETIC_CPU_FONT_BASE_HPP
#define POETIC_CPU_FONT_BASE_HPP

#include <Poetic/CPUFont.hpp>
#include <Poetic/Export.hpp>

#include <Luminous/Collectable.hpp>

#include <Radiant/Mutex.hpp>

namespace Poetic
{
  class GPUFontBase;
  class GPUFont;

  /// A base class for all CPU fonts.
  class POETIC_API CPUFontBase : public CPUFont
  {
    public:
      CPUFontBase();
      virtual ~CPUFontBase();

      Face * face();
      const Size & size() const;

      int error() const;

      float advance(const char * str, int n = -1);
      float advance(const wchar_t * str, int n = -1);

      int faceSize() const;
      virtual bool setFaceSize(int size, int resolution);

      float ascender() const;
      float descender() const;      
      float lineHeight() const;

      void bbox(const char * str, BBox & bbox);
      void bbox(const wchar_t * wstr, BBox & bbox);

    ////
      void detach(GPUFontBase * gpuFont);

      const Glyph * getGlyph(unsigned int charCode);

      virtual bool load(const char * fontFilePath);


    protected:
      virtual Glyph * makeGlyph(unsigned int g) = 0;

      Face * m_face;
      Size m_size;
      int m_error;

      Radiant::MutexAuto m_mutex;

    private:
      inline bool checkGlyph(unsigned int g);

      GlyphContainer * m_glyphList;
      Nimble::Vector2 m_pen;

      typedef std::vector<GPUFontBase *> container;  
      container m_gpuFonts;

      friend class GPUFont;

  };

}

#endif
