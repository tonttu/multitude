/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "TextureGL.hpp"
#include "Luminous/Texture.hpp"
#include "Luminous/PixelFormat.hpp"
#include <QVector>

#include <cassert>

namespace
{
  /// Specify the internal format (number of channels or explicitly requested format)
  GLenum internalFormat(const Luminous::Texture & texture)
  {
    // Check for compressed formats
    if (texture.dataFormat().compression() != Luminous::PixelFormat::COMPRESSION_NONE)
      return texture.dataFormat().compression();

    /// Specify the internal format (number of channels or explicitly requested format)
    GLenum intFormat = texture.internalFormat();
    if(intFormat == 0) {
      // The following code assumes that the formats in groups of 4
      const GLenum formats[] = { GL_RED, GL_RG, GL_RGB, GL_RGBA,
        GL_R16, GL_RG16, GL_RGB16, GL_RGBA16 };

      int channels = texture.dataFormat().numChannels();
      if (channels < 1) {
        Radiant::warning("TextureGL::upload # Unknown texture layout: '%s'", texture.dataFormat().toString().toUtf8().data());
        channels = 4;
      }
      const int bytesPerChannel = texture.dataFormat().bytesPerPixel() / channels;
      if (bytesPerChannel > 1) {
        intFormat = formats[4+channels-1];
      } else {
        intFormat = formats[channels-1];
      }
    }
    return intFormat;
  }

  GLenum getWrapMode(Luminous::Texture::Wrap wrapMode)
  {
    switch (wrapMode)
    {
    case Luminous::Texture::WRAP_BORDER: return GL_CLAMP_TO_BORDER;
    case Luminous::Texture::WRAP_CLAMP: return GL_CLAMP_TO_EDGE;
    case Luminous::Texture::WRAP_MIRROR: return GL_MIRRORED_REPEAT;
    case Luminous::Texture::WRAP_REPEAT: return GL_REPEAT;
    default:
      Radiant::error("TextureGL: Invalid wrapmode %d - Assuming default (repeat)", wrapMode);
      return GL_REPEAT;
    }
  }
}

namespace Luminous
{
  TextureGL::TextureGL(StateGL & state)
    : ResourceHandleGL(state)
    , m_generation(0)
    , m_paramsGeneration(-1)
    , m_internalFormat(0)
    , m_target(0)
    , m_size(0, 0, 0)
    , m_samples(0)
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
    , m_paramsGeneration(t.m_paramsGeneration)
    , m_internalFormat(t.m_internalFormat)
    , m_target(t.m_target)
    , m_dirtyRegion2D(t.m_dirtyRegion2D)
    , m_size(0, 0, 0)
    , m_samples(t.m_samples)
  {
  }

  TextureGL & TextureGL::operator=(TextureGL && t)
  {
    ResourceHandleGL::operator=(std::move(t));
    m_target = t.m_target;
    m_generation = t.m_generation;
    m_paramsGeneration = t.m_paramsGeneration;
    m_internalFormat = t.m_internalFormat;
    m_dirtyRegion2D = t.m_dirtyRegion2D;
    m_size = t.m_size;
    m_samples = t.m_samples;
    return *this;
  }

  void TextureGL::upload1D(const Texture & texture, int textureUnit, bool forceBind)
  {
    bool bound = false;

    const bool paramsDirty = updateParams(texture);
    const bool dirty = m_generation != texture.generation();

    if (dirty) {
      m_generation = texture.generation();

      // Check if we need to reallocate the texture. We reallocate if the
      // dimensions, size, or format has changed.
      bool recreate = 
        (m_target != GL_TEXTURE_1D) ||
        (m_size[0] != texture.width()) ||
        (m_internalFormat != texture.internalFormat()) ||
        (m_samples != texture.samples());

      if(recreate) {
        m_target = 0;
        m_size.make(texture.width(), 1, 1);
        m_samples = texture.samples();
      }
    }

    if(m_target == 0) {
      m_target = GL_TEXTURE_1D;
      bind(textureUnit);
      bound = true;

      // Create a new texture
      glTexImage1D(GL_TEXTURE_1D, 0, internalFormat(texture), texture.width(), 0,
        texture.dataFormat().layout(), texture.dataFormat().type(), nullptr);
      GLERROR("TextureGL::upload # glTexImage1D");
    }

    if(!bound && forceBind) {
      bind(textureUnit);
      bound = true;
    }

    /// @todo should we touch the dirty region if data is null?
    if(!texture.data()) {
      if (paramsDirty) {
        if (!bound)
          bind(textureUnit);
        setTexParameters();
      }
      return;
    }

    if(texture.samples() > 0) {
      Radiant::error("TextureGL::upload # Trying to upload data to multisampled texture");
      return;
    }

    // Perform an (incremental) upload of the data
    if(dirty) {
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

      int uploaded = texture.dataSize();
      /// @todo Use upload limiter
      glTexSubImage1D(m_target, 0, 0, texture.width(), texture.dataFormat().layout(), texture.dataFormat().type(), texture.data());

      // Update upload-limiter
      m_state.consumeUploadBytes(uploaded);
    }

    if (paramsDirty) {
      if (!bound)
        bind(textureUnit);
      setTexParameters();
    }
  }

  void TextureGL::upload2D(const Texture & texture, int textureUnit, bool forceBind)
  {
    bool bound = false;

    const bool compressedFormat = texture.dataFormat().compression() != PixelFormat::COMPRESSION_NONE;
    const bool paramsDirty = updateParams(texture);

    const bool dirty = m_generation != texture.generation();
    if (dirty) {
      m_generation = texture.generation();

      // Check if we need to reallocate the texture. We reallocate if the
      // dimensions, size, or format has changed.
      bool recreate = 
          (m_target != GL_TEXTURE_2D && m_target != GL_TEXTURE_2D_MULTISAMPLE) ||
          (m_size[0] != texture.width() || m_size[1] != texture.height()) ||
          (m_internalFormat != texture.internalFormat()) ||
          (m_samples != texture.samples());

      if(recreate) {
        m_target = 0;
        m_size.make(texture.width(), texture.height(), 1);
        m_samples = texture.samples();
      } else {
        m_dirtyRegion2D = QRegion(0, 0, texture.width(), texture.height());
      }
    }

    m_dirtyRegion2D += texture.takeDirtyRegion(m_state.threadIndex());

    if(m_target == 0) {
      // Mark the whole texture dirty
      m_dirtyRegion2D = QRegion(0, 0, texture.width(), texture.height());
      m_target = texture.samples() == 0
        ? GL_TEXTURE_2D
        : GL_TEXTURE_2D_MULTISAMPLE;

      bind(textureUnit);
      bound = true;

      // Create a new texture
      GLenum intFormat = internalFormat(texture);
      if(compressedFormat) {
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, intFormat, texture.width(),
          texture.height(), 0, texture.dataSize(), texture.data());
        GLERROR("TextureGL::upload # glCompressedTexImage2D");
        m_dirtyRegion2D = QRegion();
      }
      else {
        if(texture.samples() > 0) {
          glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, texture.samples(), intFormat,
            texture.width(), texture.height(), GL_FALSE);
        } else {
          glTexImage2D(GL_TEXTURE_2D, 0, intFormat, texture.width(), texture.height(), 0,
            texture.dataFormat().layout(), texture.dataFormat().type(), nullptr);
        }
        GLERROR("TextureGL::upload # glTexImage2D");
      }
    }

    if(!bound && forceBind) {
      bind(textureUnit);
      bound = true;
    }

    /// @todo should we touch the dirty region if data is null?
    if(!texture.data()) {
      if (paramsDirty) {
        if (!bound)
          bind(textureUnit);
        setTexParameters();
      }
      return;
    }

    if(texture.samples() > 0) {
      Radiant::error("TextureGL::upload # Trying to upload data to multisampled texture");
      return;
    }

    // Perform an (incremental) upload of the data
    if(!m_dirtyRegion2D.isEmpty()) {
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

      
      // See how much of the bytes we can upload in this frame
      int64_t bytesFree = m_state.availableUploadBytes();

      /// @todo glCompressedTexImage2D, probably needs some alignment
      if(compressedFormat) {
        uploaded = texture.dataSize();
        glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.width(), texture.height(),
          texture.dataFormat().compression(), uploaded, texture.data());
        GLERROR("TextureGL::upload # glCompressedTexSubImage2D");
        m_dirtyRegion2D = QRegion();
      } else {
        for(const QRect & rect : m_dirtyRegion2D.rects()) {
          const int bytesPerScanline = rect.width() * texture.dataFormat().bytesPerPixel();
          // Number of scanlines to upload
          const size_t scanLines = Nimble::Math::Clamp<int32_t>(bytesFree / bytesPerScanline, 1, rect.height());

          auto data = static_cast<const char *>(texture.data()) + (rect.left() + rect.top() * texture.width()) *
            texture.dataFormat().bytesPerPixel();

          // Upload data
          glTexSubImage2D(GL_TEXTURE_2D, 0, rect.left(), rect.top(), rect.width(), scanLines,
            texture.dataFormat().layout(), texture.dataFormat().type(), data);
          GLERROR("TextureGL::upload # glTexSubImage2D");
          uploaded += bytesPerScanline * scanLines;
          bytesFree -= bytesPerScanline * scanLines;

          if (int(scanLines) != rect.height()) {
            m_dirtyRegion2D -= QRegion(rect.left(), rect.top(), rect.width(), scanLines);
            break;
          } else {
            m_dirtyRegion2D -= rect;
          }
        }
      }

      // Update upload-limiter
      m_state.consumeUploadBytes(uploaded);
    }

    if (paramsDirty) {
      if (!bound)
        bind(textureUnit);
      setTexParameters();
    }
  }

  void TextureGL::upload3D(const Texture & texture, int textureUnit, bool forceBind)
  {
    bool bound = false;

    const bool paramsDirty = updateParams(texture);
    const bool dirty = m_generation != texture.generation();
    if (dirty) {
      m_generation = texture.generation();

      // Check if we need to reallocate the texture. We reallocate if the
      // dimensions, size, or format has changed.
      bool recreate = 
        (m_target != GL_TEXTURE_3D && m_target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) ||
        (m_size[0] != texture.width() || m_size[1] != texture.height() || m_size[2] != texture.depth()) ||
        (m_internalFormat != texture.internalFormat()) ||
        (m_samples != texture.samples());

      if(recreate) {
        m_target = 0;
        m_size.make(texture.width(), texture.height(), texture.depth());
        m_samples = texture.samples();
      }
    }

    if(m_target == 0) {
      // Mark the whole texture dirty
      m_target = texture.samples() == 0
        ? GL_TEXTURE_3D
        : GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

      bind(textureUnit);
      bound = true;

      // Create a new texture
      //////////////////////////////////////////////////////////////////////////
      // Are we using a compressed format?
      GLenum intFormat = internalFormat(texture);
      if(texture.samples() > 0) {
        glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE, texture.samples(), intFormat,
          texture.width(), texture.height(), texture.depth(), GL_FALSE);
      } else {
        glTexImage3D(GL_TEXTURE_3D, 0, intFormat, texture.width(), texture.height(), texture.depth(), 0,
          texture.dataFormat().layout(), texture.dataFormat().type(), nullptr);
      }
      GLERROR("TextureGL::upload # glTexImage3D");
    }

    if(!bound && forceBind) {
      bind(textureUnit);
      bound = true;
    }

    /// @todo should we touch the dirty region if data is null?
    if(!texture.data()) {
      if (paramsDirty) {
        if (!bound)
          bind(textureUnit);
        setTexParameters();
      }
      return;
    }

    if(texture.samples() > 0) {
      Radiant::error("TextureGL::upload # Trying to upload data to multisampled texture");
      return;
    }

    // Perform an (incremental) upload of the data
    if(dirty) {
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

      int uploaded = texture.dataSize();
      glTexSubImage3D(m_target, 0, 0, 0, 0, texture.width(), texture.height(), texture.depth(),
        texture.dataFormat().layout(), texture.dataFormat().type(), texture.data());

      // Update upload-limiter
      m_state.consumeUploadBytes(uploaded);
    }

    if (paramsDirty) {
      if (!bound)
        bind(textureUnit);
      setTexParameters();
    }
  }

  bool TextureGL::updateParams(const Texture & texture)
  {
    const bool paramsDirty = m_paramsGeneration != texture.paramsGeneration();
    if (paramsDirty) {
      m_paramsGeneration = texture.paramsGeneration();
      // Set parameters of tex unit
      texture.getWrap(m_wrap[0], m_wrap[1], m_wrap[2]);
      m_minFilter = texture.getMinFilter();
      m_magFilter = texture.getMagFilter();
      m_borderColor = texture.borderColor();
    }
    return paramsDirty;
  }

  void TextureGL::upload(const Texture & texture, int textureUnit, bool forceBind)
  {
    // Reset usage timer
    touch();

    switch (texture.dimensions())
    {
    case 1: upload1D(texture, textureUnit, forceBind); break;
    case 2: upload2D(texture, textureUnit, forceBind); break;
    case 3: upload3D(texture, textureUnit, forceBind); break;
    default:
      Radiant::error("TextureGL::upload # Error: unknown number of dimensions (%d) while trying to upload texture", texture.dimensions());
      assert(false);
    }
  }

  void TextureGL::setTexParameters() const
  {
    if(m_samples == 0) {
      glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, m_minFilter);
      GLERROR("TextureGL::upload # glTexParameteri");
      glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, m_magFilter);
      GLERROR("TextureGL::upload # glTexParameteri");

      glTexParameteri(m_target, GL_TEXTURE_WRAP_S, getWrapMode(m_wrap[0]));
      GLERROR("TextureGL::upload # glTexParameteri");
      glTexParameteri(m_target, GL_TEXTURE_WRAP_T, getWrapMode(m_wrap[1]));
      GLERROR("TextureGL::upload # glTexParameteri");
      glTexParameteri(m_target, GL_TEXTURE_WRAP_R, getWrapMode(m_wrap[2]));
      GLERROR("TextureGL::upload # glTexParameteri");

      glTexParameterfv(m_target, GL_TEXTURE_BORDER_COLOR, m_borderColor.data());
      GLERROR("TextureGL::upload # glTexParameterfv GL_TEXTURE_BORDER_COLOR");
    }
  }
}
