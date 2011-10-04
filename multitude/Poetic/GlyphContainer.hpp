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

#include "Export.hpp"
#include "Face.hpp"
#include "Glyph.hpp"
#include "Charmap.hpp"

#include <vector>

namespace Poetic
{

  /// A container class for glyphs.
  class POETIC_API GlyphContainer
  {
    public:
      /// Constructs a new container for the given face
      GlyphContainer(Face * face);
      ~GlyphContainer();

      /// Adds a glyph to the container
      void add(Glyph * glyph, unsigned int characterCode);
      /// Returns the advance between two characters (kerning)
      float advance(unsigned int characterCode, unsigned int nextCharacterCode);
      /// Returns the bounding box for the given character
      BBox bbox(unsigned int characterCode) const;
      /// Returns the character map for the given encoding
      bool charMap(int encoding);
      
      /// Returns the index for the character code
      unsigned int fontIndex(unsigned int characterCode) const;

      /// Returns the glyph for the given character code
      const Glyph * glyph(unsigned int characterCode) const;

      /// Renders the given character code
      Nimble::Vector2 render(unsigned int charCode, unsigned int nextCharCode, Nimble::Vector2 penPos, const Nimble::Matrix3 & m, Nimble::Vector2f ** ptr);

    private:
      Face * m_face;
      std::vector<Glyph *> m_glyphs;
      int m_error;
      Charmap * m_charmap;
  };

}

#endif
