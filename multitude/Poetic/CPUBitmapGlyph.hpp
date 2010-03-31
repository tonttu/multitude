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
#ifndef POETIC_CPU_BITMAP_GLYPH_HPP
#define POETIC_CPU_BITMAP_GLYPH_HPP

#include <Poetic/Glyph.hpp>

namespace Poetic
{

  /// A glyph that is stored as a bitmap on the CPU
  class CPUBitmapGlyph : public Glyph
  {
    public:
      CPUBitmapGlyph(FT_GlyphSlotRec_ * glyph);
      virtual ~CPUBitmapGlyph();

      virtual Nimble::Vector2 render(Nimble::Vector2 pen, const Nimble::Matrix3 & m);

    private:
      Nimble::Vector2i m_size;
      unsigned char * m_bitmap;
      Nimble::Vector2 m_pos;

      friend class GPUTextureGlyph;
  };

}

#endif
