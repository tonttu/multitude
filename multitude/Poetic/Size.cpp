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
#include "Size.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Poetic
{

  Size::Size()
    : m_ftFace(0),
    m_ftSize(0),
    m_size(0),
    m_xRes(0),
    m_yRes(0),
    m_error(0)
  {}

//  Size::~Size()
//  {}

  bool Size::charSize(FT_FaceRec_ ** face, int pointSize, int xRes, int yRes)
  {

    if(m_size != pointSize || xRes != m_xRes || yRes != m_yRes) {
      m_error = FT_Set_Char_Size(*face, 0L, pointSize * 64, xRes, yRes);

      if(!m_error) {
        m_ftFace = face;
        m_size = pointSize;
        m_xRes = xRes;
        m_yRes = yRes;
        m_ftSize = (*m_ftFace)->size;
      } else {
        m_ftFace = 0;
        m_size = 0;
        m_xRes = m_yRes = 0;
        m_ftSize = 0;
      }
    }

    return !m_error;
  }

  float Size::ascender() const
  {
    return m_ftSize == 0 ? 0.f : static_cast <float> (m_ftSize->metrics.ascender) / 64.f;    
  } 

  float Size::descender() const
  {
    return m_ftSize == 0 ? 0.f : static_cast<float> (m_ftSize->metrics.descender) / 64.f;
  }

  float Size::width() const
  {
    if(m_ftSize == 0) 
      return 0.f;

    if(FT_IS_SCALABLE((*m_ftFace))) 
      return ( (*m_ftFace)->bbox.xMax - (*m_ftFace)->bbox.xMin) * ( static_cast <float> (m_ftSize->metrics.x_ppem) / static_cast <float> ((*m_ftFace)->units_per_EM));
    else
      return static_cast <float> (m_ftSize->metrics.max_advance) / 64.0f;
  }

  float Size::height() const
  {
    if(m_ftSize == 0) 
      return 0.f;

    if(FT_IS_SCALABLE((*m_ftFace)))
      return ((*m_ftFace)->bbox.yMax - (*m_ftFace)->bbox.yMin) * ((float)m_ftSize->metrics.y_ppem / (float)(*m_ftFace)->units_per_EM);
    else
      return static_cast <float> (m_ftSize->metrics.height) / 64.f;
  }

}
