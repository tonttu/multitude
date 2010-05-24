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

      /// Returns a face for the current font
      Face * face();
      /// Returns the size for the font
      const Size & size() const;

      /// Returns the last error
      int error() const;

      /// Returns the advance for the given string
      float advance(const char * str, int n = -1);
      /// @copydoc advance
      float advance(const wchar_t * str, int n = -1);

      /// Returns the face size in points
      int faceSize() const;
      /// Sets the face size in points
      virtual bool setFaceSize(int size, int resolution);

      /// Returns the ascender height
      float ascender() const;
      /// Returns the descender height
      float descender() const;      
      /// Returns the line height
      float lineHeight() const;

      /// Returns the bounding box for the given string
      void bbox(const char * str, BBox & bbox);
      /// @copydoc bbox
      void bbox(const wchar_t * wstr, BBox & bbox);

      /// Detaches the given GPU font
      void detach(GPUFontBase * gpuFont);

      /// Gets the glyph for the given character code
      const Glyph * getGlyph(unsigned int charCode);

      /// Loads the font from the given .ttf file
      virtual bool load(const char * fontFilePath);


    protected:
      /// Constructs a new glyph for the given glyph index
      virtual Glyph * makeGlyph(unsigned int g) = 0;

      /// Face for the font
      Face * m_face;
      /// Size of the font
      Size m_size;
      /// Last error
      int m_error;

      /// Mutex to control access to CPU resources
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
