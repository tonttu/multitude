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
#ifndef POETIC_FACE_HPP
#define POETIC_FACE_HPP

#include "Size.hpp"

#include <Nimble/Vector2.hpp>

struct FT_GlyphSlotRec_;

namespace Poetic
{

  /// Face contains information stored in a .tff file.
  class Face
  {
    public:
      Face(const char * fontFilePath);
      virtual ~Face();

      FT_FaceRec_ ** freetype() const { return m_ftFace; }

      Nimble::Vector2 kernAdvance(unsigned int index1, unsigned int index2);

      FT_GlyphSlotRec_ * glyph(unsigned int index, signed int flags);
    
      int numGlyphs() const { return m_numGlyphs; }
  
      const Size & size(int size, int res);

      int error() const { return m_error; }

      std::string fontFilePath() const { return m_fontFilePath; }

    private:
      FT_FaceRec_ ** m_ftFace;
      Size m_size;
      int m_numGlyphs;

      int * m_encodingList;
      bool m_hasKerningTable;
      int m_error;
      std::string m_fontFilePath;
  };

}

#endif
