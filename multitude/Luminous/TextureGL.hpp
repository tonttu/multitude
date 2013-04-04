/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_TEXTUREGL_HPP
#define LUMINOUS_TEXTUREGL_HPP

#include "ResourceHandleGL.hpp"
#include "Texture.hpp"

#include <Nimble/Vector3.hpp>

#include <QRegion>

namespace Luminous
{
  class TextureGL : public ResourceHandleGL
  {
  public:
    LUMINOUS_API TextureGL(StateGL & state);
    LUMINOUS_API ~TextureGL();

    LUMINOUS_API TextureGL(TextureGL && t);
    LUMINOUS_API TextureGL & operator=(TextureGL && t);

    inline QRegion & dirtyRegion2D();

    LUMINOUS_API void setTexParameters() const;
    /// @param textureUnit Texture unit, starting from 0
    LUMINOUS_API void upload(const Texture & texture, int textureUnit, bool alwaysBind);
    inline void bind(int textureUnit);

  private:
    int m_generation;
    int m_paramsGeneration;
    int m_internalFormat;
    GLenum m_target;
    QRegion m_dirtyRegion2D;
    Nimble::Vector3u m_size;
    unsigned int m_samples;

    Texture::Filter m_minFilter, m_magFilter;
    Texture::Wrap m_wrap[3];
    Radiant::Color m_borderColor;
  };

  /////////////////////////////////////////////////////////////////////////////

  QRegion & TextureGL::dirtyRegion2D()
  {
    return m_dirtyRegion2D;
  }

  void TextureGL::bind(int textureUnit)
  {
    if (m_state.setTextureUnit(textureUnit))
      glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(m_target, m_handle);

    touch();
  }
}

#endif // LUMINOUS_TEXTUREGL_HPP
