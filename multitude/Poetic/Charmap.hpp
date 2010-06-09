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
#include <Radiant/Platform.hpp>

// try to detect c++0x
#if __cplusplus > 199711L
  #include <unordered_map>
#else
  #if defined(__GNUC__) || defined(RADIANT_LINUX) || defined(RADIANT_OSX)
    #include <tr1/unordered_map>
  #elif defined(RADIANT_WIN32) && defined(_HAS_TR1)
    #include <unordered_map>
  #else
    #include <boost/tr1/unordered_map.hpp>
  #endif
  namespace std
  {
    using tr1::unordered_map;
  }
#endif

struct FT_FaceRec_;

namespace Poetic
{

  /// A character map contains the translation from character codes to glyphs
  /// indices.
  class Charmap
  {
    public:
      /// Constructs a charmap for the given font face
      Charmap(Face * face);
      ~Charmap();

      /// Selects a charmap by its encoding tag
      bool charmap(int encoding);
      /// Returns the encoding tag of the charmap
      int encoding() const { return m_ftEncoding; }

      /// Returns the index to the glyph in the glyplist given its character code
      unsigned int glyphListIndex(unsigned int charCode);
      /// Returns the glyph index of the given character code
      unsigned int fontIndex(unsigned int charCode);
      /// Inserts an index for character code
      void insertIndex(unsigned int charCode, unsigned int index);

      /// Returns the last error occured
      int error() const { return m_error; }

    private:
      int m_ftEncoding;
      FT_FaceRec_ * m_ftFace;

      typedef std::unordered_map<unsigned long, unsigned long> container;

      container m_charmap;

      int m_error;
  };

}

#endif

