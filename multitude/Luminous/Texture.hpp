/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef LUMINOUS_TEXTURE_HPP
#define LUMINOUS_TEXTURE_HPP

#include <Luminous/Luminous.hpp>
#include <Luminous/GLResource.hpp>
#include <Luminous/PixelFormat.hpp>

#include <Nimble/Vector2.hpp>

#include <Patterns/NotCopyable.hpp>

namespace Luminous
{
  class PixelFormat;
  class Image;

  /// Base class for different textures
  /** Texture objects can be create without a valid OpenGL context, but their actual
      usage requires a valid OpenGL context. */
  template<GLenum TextureType>
  class LUMINOUS_API TextureT : public GLResource, public Patterns::NotCopyable
  {
    friend class Framebuffer;

  public:
    TextureT(GLResources * res = 0)
    : GLResource(res),
      m_textureId(0),
      m_width(0),
      m_height(0),
      m_pf(PixelFormat::LAYOUT_UNKNOWN, PixelFormat::TYPE_UNKNOWN),
      m_haveMipmaps(false)
    {}
    virtual ~TextureT();

    void allocate()
    {
      if(!m_textureId)
        glGenTextures(1, & m_textureId);
    }

    /** Activate textureUnit and bind this texture to that unit. */
    void bind(GLenum textureUnit)
    {
      allocate();
      glActiveTexture(textureUnit);
      glBindTexture(TextureType, m_textureId);
    }

    void bindN(int textureUnit)
    {
      allocate();
      glActiveTexture(GL_TEXTURE0 + textureUnit);
      glBindTexture(TextureType, m_textureId);
    }

    /** Bind this texture to the currently active tecture unit. */
    void bind()
    {
      allocate();
      glBindTexture(TextureType, m_textureId);
    }

    /// Returns the width of the texture (if known)
    int width() const { return m_width; }
    /// Returns the height of the texture (if known)
    int height() const { return m_height; }

    /// Returns the size (width x height) of the texture, if known
    Nimble::Vector2i size() const
    { return Nimble::Vector2i(m_width, m_height); }

    void setWidth(int w) { m_width = w; }
    void setHeight(int h) { m_height = h; }

    /// Returns the number of pixels in this texture.
    /** Note that the one might have initialized the texture without setting
        width and height, so this information may be unreliable. As a programmer
        you would know if the values have been set properly. */
    int pixelCount() const { return m_width * m_height; }

    /** Returns the aspect ratio of this texture. This operation makes
      sense mostly for 2D tetures.  */
    float aspectRatio()
    { return m_height ? m_width / (float) m_height : 1.0f; }

    /// Returns estimation of much GPU RAM the texture uses.
    virtual long consumesBytes()
    {
      float used = float(m_width) * m_height * m_pf.bytesPerPixel();
      // Mipmaps used 33% more memory
      used *= (m_haveMipmaps ? (4.f / 3.f) : 1.f);
      return (long)used;
    }

    /** Returns the OpenGL texture id. */
    GLuint id() const { return m_textureId; };
    /// Returns true if the texture is defined
    bool isDefined() const { return id() != 0; }

    /// Returns the pixel format used by the texture
    const PixelFormat & pixelFormat() const { return m_pf; }

  protected:
    GLuint m_textureId;
    int m_width;
    int m_height;
    PixelFormat m_pf;
    bool m_haveMipmaps;
  };

  /// A 1D texture
  class LUMINOUS_API Texture1D : public TextureT<GL_TEXTURE_1D>
  {
  public:
    Texture1D(GLResources * resources = 0) : TextureT<GL_TEXTURE_1D> (resources) {}

    static Texture1D* fromImage(Image & image, bool buildMipmaps = true, GLResources * resources = 0);
    static Texture1D* fromBytes(GLenum internalFormat, int w, const void* data, const PixelFormat& srcFormat, bool buildMipmaps = true, GLResources * resources = 0);
  };

  /// A 2D texture
  class LUMINOUS_API Texture2D : public TextureT<GL_TEXTURE_2D>
  {
  public:
    Texture2D(GLResources * resources = 0) : TextureT<GL_TEXTURE_2D>(resources) {}

    /// Load the texture from an image file
    bool loadImage(const char * filename, bool buildMipmaps = true);
    /// Load the texture from an image
    bool loadImage(const Luminous::Image & image, bool buildMipmaps = true);

    /// Load the texture from from raw data, provided by the user
    bool loadBytes(GLenum internalFormat, int w, int h,
                   const void* data,
                   const PixelFormat& srcFormat,
                   bool buildMipmaps = true);
    /// Laod a sub-texture.
    void loadSubBytes(int x, int y, int w, int h, const void * subData);

    /// Create a new texture, from an image file
    static Texture2D * fromFile(const char * filename, bool buildMipmaps = true, GLResources * resources = 0);
    /// Create a new texture, from an image
    static Texture2D * fromImage(Luminous::Image & img, bool buildMipmaps = true, GLResources * resources = 0);
    /// Create a new texture from raw image data, provided by the user
    static Texture2D * fromBytes(GLenum internalFormat, int w, int h,
                const void * data,
                const PixelFormat& srcFormat,
                bool buildMipmaps = true, GLResources * resources = 0);
  };

  /// A 3D texture
  class LUMINOUS_API Texture3D : public TextureT<GL_TEXTURE_3D>
  {
  public:
    Texture3D(GLResources * resources = 0) : TextureT<GL_TEXTURE_3D> (resources) {}
  };

  /// A cubemap texture
  class LUMINOUS_API TextureCube : public TextureT<GL_TEXTURE_CUBE_MAP>
  {
  public:
    TextureCube(GLResources * resources = 0)
        : TextureT<GL_TEXTURE_CUBE_MAP> (resources) {}

  };

}

#endif
