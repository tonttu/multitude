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
#include "GPUTextureGlyph.hpp"
#include "CPUBitmapGlyph.hpp"

#include <Luminous/Utils.hpp>

#include <Nimble/Vector4.hpp>

#include <ft2build.h>
#include FT_GLYPH_H

namespace Poetic
{
  using namespace Nimble;

  GLuint GPUTextureGlyph::s_activeTexture = 0;

  GPUTextureGlyph::GPUTextureGlyph(const CPUBitmapGlyph * glyph, int texId, int xOff, int yOff, GLsizei width, GLsizei height)
    : Glyph(*glyph),
    m_width(0),
    m_height(0),
    m_textureId(texId)
  {
    m_width = glyph->m_size.x;
    m_height = glyph->m_size.y;

    if(m_width && m_height) {
      glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT); 
      glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      glBindTexture(GL_TEXTURE_2D, m_textureId);
      glTexSubImage2D(GL_TEXTURE_2D, 0, xOff, yOff, m_width, m_height, GL_ALPHA, GL_UNSIGNED_BYTE, glyph->m_bitmap);

      glPopClientAttrib();
    }

    m_uv[0].x = static_cast<float> (xOff) / static_cast<float> (width);
    m_uv[0].y = static_cast<float> (yOff) / static_cast<float> (height);

    m_uv[1].x = static_cast<float> (xOff + m_width) / static_cast<float> (width);
    m_uv[1].y = static_cast<float> (yOff + m_height) / static_cast<float> (height);

    m_pos = glyph->m_pos;
  }

  GPUTextureGlyph::~GPUTextureGlyph()
  {}

  Nimble::Vector2 GPUTextureGlyph::render(Nimble::Vector2 pen, const Nimble::Matrix3 & m)
  {
    if(s_activeTexture != m_textureId) {
      glBindTexture(GL_TEXTURE_2D, m_textureId);
      s_activeTexture = m_textureId;
    }

#ifdef POETIC_FLIP_Y
    Vector2f v0 = pen + Vector2f(m_pos.x,              -m_pos.y);
    Vector2f v1 = pen + Vector2f(m_pos.x,              -m_pos.y + m_height);
    Vector2f v2 = pen + Vector2f(m_width + m_pos.x,    -m_pos.y + m_height);
    Vector2f v3 = pen + Vector2f(m_width + m_pos.x,    -m_pos.y);
#else
    Vector2f v0 = pen + Vector2f(m_pos.x,               m_pos.y);
    Vector2f v1 = pen + Vector2f(m_pos.x,               m_pos.y - m_height);
    Vector2f v2 = pen + Vector2f(m_width + m_pos.x,     m_pos.y - m_height);
    Vector2f v3 = pen + Vector2f(m_width + m_pos.x,     m_pos.y);
#endif


#if 1
    Nimble::Vector2f v[] = {
      v0,
      v1,
      v2,
      v3
    };

    const GLfloat uvs[] = {
      m_uv[0].x, m_uv[0].y,
      m_uv[0].x, m_uv[1].y,
      m_uv[1].x, m_uv[1].y,
      m_uv[1].x, m_uv[0].y
    };

    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<GLfloat*>(v));

    glDrawArrays(GL_QUADS, 0, 4);

#else

    Nimble::Vector2f v[] = {
      m.project(v0),
      m.project(v1),
      m.project(v2),
      m.project(v3)
    };

    glBegin(GL_QUADS);
    glTexCoord2f(m_uv[0].x, m_uv[0].y);
    glVertex2fv(v[0].data());

    glTexCoord2f(m_uv[0].x, m_uv[1].y);
    glVertex2fv(v[1].data());

    glTexCoord2f(m_uv[1].x, m_uv[1].y);
    glVertex2fv(v[2].data());

    glTexCoord2f(m_uv[1].x, m_uv[0].y);
    glVertex2fv(v[3].data());
    glEnd();
#endif

    return m_advance + pen;
  }

}
