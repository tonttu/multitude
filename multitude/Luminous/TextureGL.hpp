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

  /// This class represents a Texture object in GPU memory.
  class TextureGL : public ResourceHandleGL
  {
  public:
    /// Constructor
    /// @param state OpenGL state
    LUMINOUS_API TextureGL(StateGL & state);
    /// Destructor
    LUMINOUS_API ~TextureGL();

    /// Move constructor
    /// @param t texture object to move
    LUMINOUS_API TextureGL(TextureGL && t);
    /// Move assignment operator
    /// @param t texture object to move
    LUMINOUS_API TextureGL & operator=(TextureGL && t);

    /// Get the dirty region of the texture. The dirty region keeps track of
    /// regions in the texture that must be re-uploaded.
    /// @return texture dirty region
    inline QRegion & dirtyRegion2D();

    /// Apply the texture parameters to OpenGL state
    LUMINOUS_API void setTexParameters() const;
    /// Upload texture data from CPU object
    /// @param texture texture to upload from
    /// @param textureUnit Texture unit, starting from 0
    /// @param forceBind force binding of texture even if it is already active
    LUMINOUS_API void upload(const Texture & texture, int textureUnit, bool forceBind);
    /// Bind the texture to the given texture unit
    /// @param textureUnit texture unit to bind to
    inline void bind(int textureUnit);

  private:
    void upload1D(const Texture & texture, int textureUnit, bool forceBind);
    void upload2D(const Texture & texture, int textureUnit, bool forceBind);
    void upload3D(const Texture & texture, int textureUnit, bool forceBind);
    bool updateParams(const Texture & texture);

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
    Radiant::ColorPMA m_borderColor;
  };

  /////////////////////////////////////////////////////////////////////////////

  QRegion & TextureGL::dirtyRegion2D()
  {
    return m_dirtyRegion2D;
  }

  void TextureGL::bind(int textureUnit)
  {
    if (m_state.setTextureUnit(textureUnit)) {
      m_state.opengl().glActiveTexture(GL_TEXTURE0 + textureUnit);
      GLERROR("TextureGL::bind # glActiveTexture");
    }
    m_state.opengl().glBindTexture(m_target, m_handle);
    GLERROR("TextureGL::bind # glBindTexture");

    touch();
  }
}

#endif // LUMINOUS_TEXTUREGL_HPP
