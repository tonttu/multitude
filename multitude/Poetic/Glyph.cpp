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
#include "Glyph.hpp"

#include <ft2build.h>
#include FT_GLYPH_H

namespace Poetic
{

  Glyph::Glyph(FT_GlyphSlotRec_ * glyph)
  : m_error(0)
  {
    if(glyph) {
      m_bbox = BBox(glyph);
      m_advance = Nimble::Vector2(glyph->advance.x / 64.f, glyph->advance.y / 64.f);
    }
  }

  Glyph::~Glyph()
  {}
  
}
