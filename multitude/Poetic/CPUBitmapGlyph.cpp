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
#include "CPUBitmapGlyph.hpp"
#include <GL/glew.h>
#include <Radiant/Trace.hpp>

#include <ft2build.h>
#include FT_GLYPH_H

namespace Poetic
{
  using namespace Nimble;

  CPUBitmapGlyph::CPUBitmapGlyph(FT_GlyphSlotRec_ * glyph)
  : Glyph(glyph),
    m_bitmap(0)
  {
    int error = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
    if(error || glyph->format != ft_glyph_format_bitmap) {
      Radiant::error("CPUBitmapGlyph::CPUBitmapGlyph # failed to render glyph");
      return;
    }

    FT_Bitmap bitmap = glyph->bitmap;
    m_size.x = bitmap.width;
    m_size.y = bitmap.rows;

    // Not all glyphs have bitmaps (e.g. space)
    if(m_size.x && m_size.y) {
      m_bitmap = new unsigned char [m_size.x * m_size.y];
      memcpy(m_bitmap, bitmap.buffer, sizeof(unsigned char) * m_size.x * m_size.y);
    }

    m_pos.x = glyph->bitmap_left;
    m_pos.y = glyph->bitmap_top;
  }

  CPUBitmapGlyph::~CPUBitmapGlyph()
  {
    delete[] m_bitmap;
  }


  Nimble::Vector2 CPUBitmapGlyph::render(Nimble::Vector2 pen, const Nimble::Matrix3 & )
  {
    Radiant::error("CPUBitmapGlyph::render # not implemented");

    return m_advance + pen;
  }

}
