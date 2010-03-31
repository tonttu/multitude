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
#ifndef POETIC_BBOX_HPP
#define POETIC_BBOX_HPP

#include <Poetic/Export.hpp>

#include <Nimble/Rect.hpp>

struct FT_GlyphSlotRec_;

namespace Poetic
{

  /** A bounding box class for glyphs. */
  class POETIC_API BBox : public Nimble::Rect
  {
    public:
      BBox();
      BBox(FT_GlyphSlotRec_ * glyph);
  };

}

#endif
