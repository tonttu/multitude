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
#include "GPUFontBase.hpp"
#include "GlyphContainer.hpp"
#include "CPUFont.hpp"

namespace Poetic
{

  GPUFontBase::GPUFontBase(CPUFontBase * font)
  : m_cpuFont(font)
  {
    m_glyphList = new GlyphContainer(m_cpuFont->face());
  }

  GPUFontBase::~GPUFontBase()
  {
    delete m_glyphList;
  }

  bool GPUFontBase::checkGlyph(unsigned int charCode)
  {
    if(m_glyphList->glyph(charCode) == 0)
    {
      const Glyph * cpuGlyph = m_cpuFont->getGlyph(charCode);
      if(!cpuGlyph) return false;

      Glyph * gpuGlyph = makeGlyph(cpuGlyph);
      if(!gpuGlyph) return false;

      m_glyphList->add(gpuGlyph, charCode);
    }

    return true;
  }

  void GPUFontBase::faceSizeChanged()
  {
    delete m_glyphList;
    m_glyphList = new GlyphContainer(m_cpuFont->face());
  }

  void GPUFontBase::internalRender(const wchar_t * str, int n, const Nimble::Matrix3 & transform)
  {
    const wchar_t * c = str;
    m_pen.x = m_pen.y = 0.f;

    while(n--) {
      if(checkGlyph(*c)) {
        m_pen = m_glyphList->render(*c, *(c + 1), m_pen, transform);
      }
      c++;
    }
  }

  void GPUFontBase::internalRender(const char * str, int n, const Nimble::Matrix3 & transform)
  {
    const unsigned char * c = (unsigned char *)str;
    m_pen.x = m_pen.y = 0.f;

    while(n--) {
      if(checkGlyph(*c)) {
        m_pen = m_glyphList->render(*c, *(c + 1), m_pen, transform);
      }
      c++;
    }
  }


}

