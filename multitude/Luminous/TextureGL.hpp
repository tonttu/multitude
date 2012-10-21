#ifndef LUMINOUS_TEXTUREGL_HPP
#define LUMINOUS_TEXTUREGL_HPP

#include "ResourceHandleGL.hpp"

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

    inline QRegion & dirtyRegion();

    /// @param textureUnit Texture unit, starting from 0
    LUMINOUS_API void upload(const Texture & texture, int textureUnit, bool alwaysBind);
    inline void bind(int textureUnit);

  private:
    int m_generation;
    int m_internalFormat;
    GLenum m_target;
    QRegion m_dirtyRegion;
    QSize m_size;
  };

  /////////////////////////////////////////////////////////////////////////////

  QRegion & TextureGL::dirtyRegion()
  {
    return m_dirtyRegion;
  }

  void TextureGL::bind(int textureUnit)
  {
    /// @todo add active texture unit to StateGL
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(m_target, m_handle);

    touch();
  }
}

#endif // LUMINOUS_TEXTUREGL_HPP
