#ifndef LUMINOUS_TEXTUREGL_HPP
#define LUMINOUS_TEXTUREGL_HPP

#include "ResourceHandleGL.hpp"

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

    /// @param textureUnit Texture unit, starting from 0
    LUMINOUS_API void upload(const Texture & texture, int textureUnit, bool alwaysBind);
    inline void bind(int textureUnit);

  private:
    int m_generation;
    int m_internalFormat;
    GLenum m_target;
    QRegion m_dirtyRegion2D;
    Nimble::Vector3u m_size;
  };

  /////////////////////////////////////////////////////////////////////////////

  QRegion & TextureGL::dirtyRegion2D()
  {
    return m_dirtyRegion2D;
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
