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

#include "CPUFontBase.hpp"
#include "GPUFontBase.hpp"

#include <Radiant/Trace.hpp>

namespace Poetic
{

  CPUFontBase::CPUFontBase()
    : m_face(0),
	  m_mutex(false, false, true),
      m_glyphList(0)
  {
  }

  CPUFontBase::~CPUFontBase()
  {  
    delete m_glyphList;
    delete m_face;
  }

  Face * CPUFontBase::face()
  {
    return m_face;
  }

  const Size & CPUFontBase::size() const
  {
    return m_size;
  }

  int CPUFontBase::error() const
  {
    return m_error;
  }

  void CPUFontBase::detach(GPUFontBase * gpuFont)
  {
    assert(gpuFont->cpuFont() == this);

    gpuFont->m_cpuFont = 0;

    for(container::iterator it = m_gpuFonts.begin();
	it != m_gpuFonts.end(); it++) {
      if(*it == gpuFont) { 
        m_gpuFonts.erase(it);
        return;
      }
    }
  
    assert(0);
  }

  bool CPUFontBase::setFaceSize(int size, int resolution)
  {
    if(!m_face) {
      Radiant::error("CPUFontBase::setSize # no font loaded yet!");
      return false;
    }

    m_size = m_face->size(size, resolution);
    m_error = m_face->error();

    if(m_error != 0)
      return false;

    if(m_glyphList != NULL)
      delete m_glyphList;

    m_glyphList = new GlyphContainer(m_face);

    // Notify all GPUFonts of the size change
    for(container::iterator it = m_gpuFonts.begin();
        it != m_gpuFonts.end(); it++) {
      (*it)->faceSizeChanged();
    }

    return true;
  }

  bool CPUFontBase::load(const char * fontFilePath)
  {
    Radiant::Guard g( & m_mutex);

    delete m_face;
    m_face = new Face(fontFilePath);

    m_error = m_face->error();
    if(m_error == 0) {
      delete m_glyphList;
      m_glyphList = new GlyphContainer(m_face);
      return true;
    }

     Radiant::error("CPUFontBase::load # loading font '%s' failed (error code: %d)", fontFilePath, m_error);
    return false;
  }

  int CPUFontBase::faceSize() const
  {
    return m_size.charSize();
  }

  float CPUFontBase::lineHeight() const
  {
    return float(m_size.charSize());
  }

  float CPUFontBase::ascender() const
  {
    return m_size.ascender();
  }

  float CPUFontBase::descender() const
  {
    return m_size.descender();
  }

  /// @todo check the bbox calculations, there seems to be some error here
  void CPUFontBase::bbox(const char * str, BBox & bbox)
  {
    if(str && (*str != '\0')) {
        const unsigned char * c = (unsigned char *)str;
        float advance = 0.f;
        if(checkGlyph(*c)) {
            bbox = m_glyphList->bbox(*c); 
            advance = m_glyphList->advance(*c, *(c + 1));
        }

        while(*++c) {
          if(checkGlyph(*c)) {
             BBox tempBBox = m_glyphList->bbox(*c);
                tempBBox.move(Nimble::Vector2(advance, 0.f));
                bbox.expand(tempBBox);
                advance += m_glyphList->advance(*c, *(c + 1));
            }
        }
    }
  }

  void CPUFontBase::bbox(const wchar_t * wstr, BBox & bbox)
  {
    if(wstr && (*wstr != wchar_t('\0')))
    {
#ifndef WIN32
      const unsigned wchar_t *   wc = (const unsigned wchar_t *)wstr;
#else
      const wchar_t *   wc = (const wchar_t *)wstr;
#endif

      float   advance = 0.f;
      if(checkGlyph(*wc))
      {
        bbox = m_glyphList->bbox(*wc); 
        advance = m_glyphList->advance(*wc, *(wc + 1));
      }

      while(*++wc)
      {
        if(checkGlyph(*wc))
        {
          BBox tempBBox = m_glyphList->bbox(*wc);
          tempBBox.move(Nimble::Vector2(advance, 0.0f));
          bbox.expand(tempBBox);
          advance += m_glyphList->advance(*wc, *(wc + 1));
        }
      }
    }
  }

  bool CPUFontBase::checkGlyph(unsigned int characterCode)
  {
    Radiant::Guard g( & m_mutex);

    if(m_glyphList->glyph(characterCode) == 0)
    {
        unsigned int glyphIndex = m_glyphList->fontIndex(characterCode);
        Glyph * tempGlyph = makeGlyph(glyphIndex);
        if(tempGlyph == 0) {
            if(m_error == 0)
                m_error = 0x13;

            return false;
        }
        
        m_glyphList->add(tempGlyph, characterCode);
    }

    return true;
  }

  float CPUFontBase::advance(const char * str, int n)
  {
    const unsigned char * c = (unsigned char *)str;
    float width = 0.f;

    int i = 0;
    while(*c) {
      
      if(n >= 0 && i >= n)
	break;

      if(checkGlyph(*c)) {
        width += m_glyphList->advance(*c, *(c + 1));
      }
      c++;
      i++;
    }

    return width;
  }

  float CPUFontBase::advance(const wchar_t * str, int n)
  {
    const wchar_t * c = str;
    float width = 0.f;

    int i = 0;

    while(*c) {

      if(n >= 0 && i >= n)
	break;

      if(checkGlyph(*c)) {
        width += m_glyphList->advance(*c, *(c + 1));
      }
      c++;
      i++;
    }

    return width;
  }

  const Glyph * CPUFontBase::getGlyph(unsigned int charCode)
  {
    // Guard guard(m_mutex);
    if(checkGlyph(charCode))
      return m_glyphList->glyph(charCode);
    
    return 0;
  }


}

