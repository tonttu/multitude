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
#include "CPUBitmapFont.hpp"
#include "CPUBitmapGlyph.hpp"
#include "GPUTextureFont.hpp"

#include <Radiant/Trace.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Poetic
{

  CPUBitmapFont::CPUBitmapFont()
	  
  {
  }

  CPUBitmapFont::~CPUBitmapFont()
  {
  }
  
  GPUTextureFont * CPUBitmapFont::createGPUFont()
  {
    return new GPUTextureFont(this);
  }

  Glyph * CPUBitmapFont::makeGlyph(unsigned int glyphIndex)
  {
    Radiant::Guard g( & m_mutex);
    
    assert(m_face != 0);

    FT_GlyphSlot ftGlyph = m_face->glyph(glyphIndex, FT_LOAD_NO_HINTING);
    
    if(ftGlyph) {
      CPUBitmapGlyph * tempGlyph = new CPUBitmapGlyph(ftGlyph);
      return tempGlyph;
    }    

    m_error = m_face->error(); 

    return 0;
  }

  bool CPUBitmapFont::setFaceSize(int size, int resolution)
  {
    assert(m_face != 0);

    return CPUFontBase::setFaceSize(size, resolution);
  }

}

