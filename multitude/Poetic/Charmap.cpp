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
#include "Charmap.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Poetic
{

  Charmap::Charmap(Face * face)
  : m_ftFace(*face->freetype()),
    m_error(0)
  {
    if(!m_ftFace->charmap)  
      m_error = FT_Set_Charmap(m_ftFace, m_ftFace->charmaps[0]);

    m_ftEncoding = m_ftFace->charmap->encoding;
  }

  Charmap::~Charmap()
  {
    m_charmap.clear();
  }

  bool Charmap::charmap(int encoding)
  {
    if(m_ftEncoding == encoding) 
      return true;

    m_error = FT_Select_Charmap(m_ftFace, (FT_Encoding)encoding);

    if(!m_error) 
      m_ftEncoding = encoding;
    else  
      m_ftEncoding = ft_encoding_none;

    m_charmap.clear();
    return !m_error;
  }

  unsigned int Charmap::glyphListIndex(unsigned int charCode)
  {
    container::const_iterator it = m_charmap.find(charCode);
    
    if(it == m_charmap.end()) return 0;
    else return it->second; 
  }

  unsigned int Charmap::fontIndex(unsigned int charCode)
  {
    return FT_Get_Char_Index(m_ftFace, charCode);
  }

  void Charmap::insertIndex(unsigned int charCode, unsigned int index)
  {
    m_charmap[charCode] = index;
  }

}
