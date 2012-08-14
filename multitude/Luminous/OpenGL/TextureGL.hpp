#ifndef LUMINOUS_TEXTUREGL_HPP
#define LUMINOUS_TEXTUREGL_HPP

#include "ResourceHandleGL.hpp"

#include <QRegion>

namespace Luminous
{
  class TextureGL : public ResourceHandleGL
  {
  public:
    TextureGL(StateGL & state);
    ~TextureGL();

    TextureGL(TextureGL && t);
    TextureGL & operator=(TextureGL && t);

    inline QRegion & dirtyRegion();

    /// @param textureUnit Texture unit, starting from 0
    void upload(const Texture & texture, int textureUnit, bool alwaysBind);
    inline void bind(int textureUnit);

  private:
    int m_generation;
    int m_internalFormat;
    GLenum m_target;
    QRegion m_dirtyRegion;
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
