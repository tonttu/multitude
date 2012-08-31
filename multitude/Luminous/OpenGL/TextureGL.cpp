#include "TextureGL.hpp"
#include "Luminous/Texture2.hpp"
#include "Luminous/PixelFormat.hpp"

#include <QVector>

namespace Luminous
{
  TextureGL::TextureGL(StateGL & state)
    : ResourceHandleGL(state)
    , m_generation(0)
    , m_internalFormat(0)
    , m_target(0)
  {
    glGenTextures(1, &m_handle);
  }

  TextureGL::~TextureGL()
  {
    if(m_handle)
      glDeleteTextures(1, &m_handle);
  }

  TextureGL::TextureGL(TextureGL && t)
    : ResourceHandleGL(std::move(t))
    , m_generation(t.m_generation)
    , m_internalFormat(t.m_internalFormat)
    , m_target(t.m_target)
    , m_dirtyRegion(t.m_dirtyRegion)
  {
  }

  TextureGL & TextureGL::operator=(TextureGL && t)
  {
    ResourceHandleGL::operator=(std::move(t));
    m_target = t.m_target;
    m_generation = t.m_generation;
    m_internalFormat = t.m_internalFormat;
    m_dirtyRegion = t.m_dirtyRegion;
    return *this;
  }

  void TextureGL::upload(const Texture & texture, int textureUnit, bool alwaysBind)
  {
    // Reset usage timer
    touch();

    bool bound = false;

    const bool compressedFormat = texture.dataFormat().compression() != PixelFormat::COMPRESSION_NONE;

    if(m_generation != texture.generation()) {
      m_generation = texture.generation();

      // Check if we need to reallocate the texture. We reallocate if the
      // dimensions, size, or format has changed.
      bool recreate = (texture.dimensions() == 1 && m_target != GL_TEXTURE_1D) ||
          (texture.dimensions() == 2 && m_target != GL_TEXTURE_2D) ||
          (texture.dimensions() == 3 && m_target != GL_TEXTURE_3D);

      recreate = recreate || (m_size.width() != texture.width() || m_size.height() != texture.height());

      recreate = recreate || (m_internalFormat != texture.internalFormat());

      if(recreate) {
        m_target = 0;
        m_size = QSize(texture.width(), texture.height());
      } else {
        m_dirtyRegion = QRegion(0, 0, texture.width(), texture.height());
      }
    }

    m_dirtyRegion += texture.takeDirtyRegion(m_state.threadIndex());

    if(m_target == 0) {
      // Mark the whole texture dirty
      m_dirtyRegion = QRegion(0, 0, texture.width(), texture.height());

      if (texture.dimensions() == 1)      m_target = GL_TEXTURE_1D;
      else if (texture.dimensions() == 2) m_target = GL_TEXTURE_2D;
      else if (texture.dimensions() == 3) m_target = GL_TEXTURE_3D;

      bind(textureUnit);
      bound = true;

      /// Specify the internal format (number of channels or explicitly requested format)
      GLenum intFormat = texture.internalFormat();
      if(intFormat == 0) {
        if(compressedFormat) {
          intFormat = texture.dataFormat().compression();
        } else {
          GLenum formats[] = { GL_RED, GL_RG, GL_RGB, GL_RGBA,
                               GL_R16, GL_RG16, GL_RGB16, GL_RGBA16 };

          int offset = texture.dataFormat().bytesPerPixel() > 1 ? 4 : 0;
          int channels = texture.dataFormat().numChannels();
          offset += channels < 1 || channels > 4 ? 3 : channels - 1;
          intFormat = formats[offset];
        }
      }

      if(compressedFormat) {
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, intFormat, texture.width(),
                               texture.height(), 0, texture.dataSize(), texture.data());
        GLERROR("TextureGL::upload # glCompressedTexImage2D");
        m_dirtyRegion = QRegion();
      } else {
        if(texture.dimensions() == 1) {
          glTexImage1D(GL_TEXTURE_1D, 0, intFormat, texture.width(), 0,
                       texture.dataFormat().layout(), texture.dataFormat().type(), nullptr);
          GLERROR("TextureGL::upload # glTexImage1D");
        } else if(texture.dimensions() == 2) {
          glTexImage2D(GL_TEXTURE_2D, 0, intFormat, texture.width(), texture.height(), 0,
                       texture.dataFormat().layout(), texture.dataFormat().type(), nullptr);
          GLERROR("TextureGL::upload # glTexImage2D");
        } else if(texture.dimensions() == 3) {
          glTexImage3D(GL_TEXTURE_3D, 0, intFormat, texture.width(), texture.height(), texture.depth(),
                       0, texture.dataFormat().layout(), texture.dataFormat().type(), nullptr);
          GLERROR("TextureGL::upload # glTexImage3D");
        }
      }

      glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, texture.getMinFilter());
      GLERROR("TextureGL::upload # glTexParameteri");
      glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, texture.getMagFilter());
      GLERROR("TextureGL::upload # glTexParameteri");
    }

    if(!bound && alwaysBind) {
      bind(textureUnit);
      bound = true;
    }

    /// @todo should we touch the dirty region if data is null?
    if(!texture.data())
      return;

    /// @todo 1D / 3D textures don't work atm
    if(!m_dirtyRegion.isEmpty()) {
      if(!bound)
        bind(textureUnit);

      // Set proper alignment
      int alignment = 1;
      while ((texture.width() * texture.dataFormat().bytesPerPixel()) % alignment)
        alignment >>= 1;

      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
      GLERROR("TextureGL::upload # glPixelStorei");
      glPixelStorei(GL_UNPACK_ROW_LENGTH, texture.lineSizePixels());
      GLERROR("TextureGL::upload # glPixelStorei");

      int uploaded = 0;

      if (texture.dimensions() == 1) {
        /// @todo incremental upload
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, texture.width(), texture.dataFormat().layout(), texture.dataFormat().type(), texture.data());
        GLERROR("TextureGL::upload # glTexSubImage1D");
        uploaded = texture.width() * texture.dataFormat().bytesPerPixel();
      }
      else if (texture.dimensions() == 2) {
        // See how much of the bytes we can upload in this frame
        int64_t bytesFree = m_state.availableUploadBytes();

        /// @todo glCompressedTexImage2D, probably needs some alignment
        if(compressedFormat) {
          uploaded = texture.dataSize();
          glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.width(), texture.height(),
                                    texture.dataFormat().compression(), uploaded, texture.data());
          GLERROR("TextureGL::upload # glCompressedTexSubImage2D");
          m_dirtyRegion = QRegion();
        } else {
          foreach(const QRect & rect, m_dirtyRegion.rects()) {
            const int bytesPerScanline = rect.width() * texture.dataFormat().bytesPerPixel();
            // Number of scanlines to upload
            const size_t scanLines = std::min<int32_t>(rect.height(), bytesFree / bytesPerScanline);

            auto data = static_cast<const char *>(texture.data()) + (rect.left() + rect.top() * texture.width()) *
                texture.dataFormat().bytesPerPixel();

            // Upload data
            glTexSubImage2D(GL_TEXTURE_2D, 0, rect.left(), rect.top(), rect.width(), scanLines,
                            texture.dataFormat().layout(), texture.dataFormat().type(), data);
            GLERROR("TextureGL::upload # glTexSubImage2D");
            uploaded += bytesPerScanline * scanLines;
            bytesFree -= uploaded;

            if (int(scanLines) != rect.height()) {
              m_dirtyRegion -= QRegion(rect.left(), rect.top(), rect.width(), scanLines);
              break;
            } else {
              m_dirtyRegion -= rect;
            }
          }
        }
      }
      else if (texture.dimensions() == 3) {
        /// @todo incremental upload
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, texture.width(), texture.height(), texture.depth(),
                        texture.dataFormat().layout(), texture.dataFormat().type(), texture.data());
        uploaded = texture.width() * texture.height() * texture.depth() * texture.dataFormat().bytesPerPixel();
        GLERROR("TextureGL::upload # glTexSubImage3D");
      }

      // Update upload-limiter
      m_state.consumeUploadBytes(uploaded);
    }
  }
}
