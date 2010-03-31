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
#ifndef POETIC_CHARMAP_HPP
#define POETIC_CHARMAP_HPP

#include "Face.hpp"
#include <map>

struct FT_FaceRec_;

namespace Poetic
{

  /// A character map contains the translation from character codes to glyphs
  /// indices.
  class Charmap
  {
    public:
      Charmap(Face * face);
      ~Charmap();

      bool charmap(int encoding);
      int encoding() const { return m_ftEncoding; }

      unsigned int glyphListIndex(unsigned int charCode);
      unsigned int fontIndex(unsigned int charCode);
      void insertIndex(unsigned int charCode, unsigned int index);

      int error() const { return m_error; }

    private:
      int m_ftEncoding;
      FT_FaceRec_ * m_ftFace;

      typedef std::map<unsigned long, unsigned long> container;

      container m_charmap;

      int m_error;
  };

}

#endif

