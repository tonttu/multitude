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
#ifndef POETIC_GLYPH_CONTAINER_HPP
#define POETIC_GLYPH_CONTAINER_HPP

#include "Face.hpp"
#include "Glyph.hpp"
#include "Charmap.hpp"
#include <vector>

namespace Poetic
{

  /// A container class for glyphs.
  class GlyphContainer
  {
    public:
      GlyphContainer(Face * face);
      ~GlyphContainer();

      void add(Glyph * glyph, unsigned int characterCode);
      float advance(unsigned int characterCode, unsigned int nextCharacterCode);
      BBox bbox(unsigned int characterCode) const;
      bool charMap(int encoding);
      
      unsigned int fontIndex(unsigned int characterCode) const;

      const Glyph * glyph(unsigned int characterCode) const;

      Nimble::Vector2 render(unsigned int charCode, unsigned int nextCharCode, Nimble::Vector2 penPos, const Nimble::Matrix3 & m);

    private:
      Face * m_face;
      std::vector<Glyph *> m_glyphs;
      int m_error;
      Charmap * m_charmap;
  };

}

#endif
