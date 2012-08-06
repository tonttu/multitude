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
    GLERROR("TextureGL::upload");

    // Reset usage timer
    touch();

    bool bound = false;

    const bool compressedFormat = texture.dataFormat().compression() != PixelFormat::COMPRESSION_NONE;

    if(m_generation != texture.generation()) {
      m_generation = texture.generation();

      bool recreate = (texture.dimensions() == 1 && m_target != GL_TEXTURE_1D) ||
          (texture.dimensions() == 2 && m_target != GL_TEXTURE_2D) ||
          (texture.dimensions() == 3 && m_target != GL_TEXTURE_3D);

      recreate = recreate || (m_internalFormat != texture.internalFormat());

      if(recreate) {
        m_target = 0;
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
          if (texture.dataFormat().numChannels() == 1)      intFormat = GL_RED;
          else if (texture.dataFormat().numChannels() == 2) intFormat = GL_RG;
          else if (texture.dataFormat().numChannels() == 3) intFormat = GL_RGB;
          else intFormat = GL_RGBA;
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

      /// @todo Get these from the texture settings
      glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
      int alignment = 8;
      while (texture.width() % alignment)
        alignment >>= 1;
      glPixelStorei(GL_PACK_ALIGNMENT, alignment);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, texture.lineSizePixels());

      int uploaded = 0;

      if (texture.dimensions() == 1) {
        /// @todo incremental upload
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, texture.width(), texture.dataFormat().layout(), texture.dataFormat().type(), texture.data());
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
            uploaded += bytesPerScanline * scanLines;
            bytesFree -= uploaded;

            if(scanLines != rect.height()) {
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
      }

      // Update upload-limiter
      m_state.consumeUploadBytes(uploaded);
    }
  }
}
