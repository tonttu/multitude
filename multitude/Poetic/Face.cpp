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
#include "Poetic.hpp"
#include "Face.hpp"

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H

namespace Poetic
{

  Face::Face(const char * fontFilePath)
  : m_numGlyphs(0),
    m_encodingList(0),
    m_error(0),
    m_fontFilePath(fontFilePath)
  {
    FT_Long DEFAULT_FACE_INDEX = 0;
    m_ftFace = new FT_Face;

    m_error = FT_New_Face(*Poetic::freetype(), fontFilePath, DEFAULT_FACE_INDEX, m_ftFace);
    if(m_error) {
      delete m_ftFace;
      m_ftFace = 0;
    } else {
      m_numGlyphs = (*m_ftFace)->num_glyphs;
      m_hasKerningTable = bool(FT_HAS_KERNING((*m_ftFace)));
    }
  }

  Face::~Face()
  {
    if(m_ftFace) {
      FT_Done_Face(*m_ftFace);
      delete m_ftFace;
      m_ftFace = 0;
    }
  }

  Nimble::Vector2 Face::kernAdvance(unsigned int index1, unsigned int index2)
  {
    float x = 0.f;
    float y = 0.f;

    if(m_hasKerningTable && index1 && index2) {
      FT_Vector kernAdvance;
      kernAdvance.x = kernAdvance.y = 0;

      m_error = FT_Get_Kerning(*m_ftFace, index1, index2, ft_kerning_unfitted, &kernAdvance);
      if(!m_error) {
        x = static_cast <float> (kernAdvance.x) / 64.0f;
        y = static_cast <float> (kernAdvance.y) / 64.0f;
      }
    }

    return Nimble::Vector2(x, y);
  }

  const Size & Face::size(int size, int res)
  {
    m_size.charSize(m_ftFace, size, res, res);
    m_error = m_size.error();

    return m_size;
  }

  FT_GlyphSlot Face::glyph(unsigned int index, signed int flags)
  {
    m_error = FT_Load_Glyph(*m_ftFace, index, flags);
    if(m_error)
        return 0;

    return (*m_ftFace)->glyph;
  }

}
