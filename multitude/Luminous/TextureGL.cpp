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
  static GLenum internalFormat(const Luminous::Texture & texture)
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

  static GLenum getWrapMode(Luminous::Texture::Wrap wrapMode)
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
  static TextureGL::UploadMethod s_defaultUploadMethod = TextureGL::METHOD_TEXTURE;
  static bool s_asyncUploadingEnabled = false;

  TextureGL::TextureGL(StateGL & state)
    : ResourceHandleGL(state)
    , m_generation(0)
    , m_paramsGeneration(-1)
    , m_internalFormat(0)
    , m_target(0)
    , m_size(0, 0, 0)
    , m_samples(0)
  {
    m_state.opengl().glGenTextures(1, &m_handle);
    GLERROR("TextureGL::TextureGL # glGenTextures");
  }

  TextureGL::~TextureGL()
  {
    if (m_asyncUploadTasks > 0)
      sync(0);
    if(m_handle) {
      m_state.opengl().glDeleteTextures(1, &m_handle);
      GLERROR("TextureGL::~TextureGL # glDeleteTextures");
    }
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
    , m_uploadBuffer(std::move(t.m_uploadBuffer))
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
    m_uploadBuffer = std::move(t.m_uploadBuffer);
    return *this;
  }

  void TextureGL::upload1D(const Texture & texture, int textureUnit)
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
        m_internalFormat = texture.internalFormat();
        m_samples = texture.samples();
      }
    }

    if(m_target == 0) {
      m_target = GL_TEXTURE_1D;
      bind(textureUnit);
      bound = true;

      // Create a new texture
      m_state.opengl().glTexImage1D(GL_TEXTURE_1D, 0, internalFormat(texture), texture.width(), 0,
                                    texture.dataFormat().layout(), texture.dataFormat().type(), nullptr);
      GLERROR("TextureGL::upload # glTexImage1D");
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

      int uploaded = texture.dataSize();
      /// @todo Use upload limiter
      m_state.opengl().glTexSubImage1D(m_target, 0, 0, texture.width(), texture.dataFormat().layout(), texture.dataFormat().type(), texture.data().get());
      GLERROR("TextureGL::upload1D # glTexSubImage1D");

      if (texture.mipmapsEnabled()) {
        m_state.opengl().glGenerateMipmap(m_target);
        GLERROR("TextureGL::upload1D # glGenerateMipmap");
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

  void TextureGL::upload2D(const Texture & texture, int textureUnit)
  {
    bool bound = false;

    const bool compressedFormat = texture.dataFormat().compression() != PixelFormat::COMPRESSION_NONE;
    const bool paramsDirty = updateParams(texture);

    const bool dirty = m_generation != texture.generation();
    if (dirty) {
      m_generation = texture.generation();
      m_useAsyncUpload = s_asyncUploadingEnabled && texture.allowAsyncUpload();

      // Check if we need to reallocate the texture. We reallocate if the
      // dimensions, size, or format has changed.
      bool recreate =
          (m_size[0] != texture.width() || m_size[1] != texture.height()) ||
          (m_internalFormat != texture.internalFormat()) ||
          (m_samples != texture.samples());

      if(recreate) {
        m_target = 0;
        m_size.make(texture.width(), texture.height(), 1);
        m_internalFormat = texture.internalFormat();
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
        m_state.opengl().glCompressedTexImage2D(GL_TEXTURE_2D, 0, intFormat, texture.width(),
                                                texture.height(), 0, texture.dataSize(), texture.data().get());
        GLERROR("TextureGL::upload # glCompressedTexImage2D");
        m_dirtyRegion2D = QRegion();
      }
      else {
        if(texture.samples() > 0) {
          // The last parameter fixedSampleLocations needs to be true in order
          // to use this texture inside a multisampled FBO with a multisampled
          // render buffer
          m_state.opengl().glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, texture.samples(), intFormat,
                                                   texture.width(), texture.height(), GL_TRUE);
        } else {
          m_state.opengl().glTexImage2D(GL_TEXTURE_2D, 0, intFormat, texture.width(), texture.height(), 0,
                                        texture.dataFormat().layout(), texture.dataFormat().type(), nullptr);
        }
        /// @todo is it more efficient to call glGenerateMipmap here to pre-allocate the mipmap levels?
        /// We should use glTexStorage2D, but it's OpenGL 4.
        // if (texture.mipmapsEnabled())
        //   glGenerateMipmap(GL_TEXTURE_2D);
        GLERROR("TextureGL::upload # glTexImage2D");
      }
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
      QRegion dirty;
      dirty.swap(m_dirtyRegion2D);

      if (m_useAsyncUpload) {
        {
          QMutexLocker g(&m_asyncUploadMutex);
          ++m_asyncUploadTasks;
        }
        m_state.addTask([this, tex=texture.dataInfo(), mipmaps=texture.mipmapsEnabled(), dirty, compressedFormat, textureUnit] {
          m_state.opengl().glBindTexture(m_target, m_handle);
          GLERROR("TextureGL::upload2D # glBindTexture");
          upload2DImpl(tex, dirty, compressedFormat, mipmaps);
          auto fence = m_state.opengl().glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
          {
            QMutexLocker g(&m_asyncUploadMutex);
            --m_asyncUploadTasks;
            m_fences.push_back(fence);
          }
          m_asyncUploadCond.wakeAll();
        });
      } else {
        if (!bound)
          bind(textureUnit);

        upload2DImpl(texture.dataInfo(), dirty, compressedFormat, texture.mipmapsEnabled());
      }
    }

    if (paramsDirty) {
      if (!bound)
        bind(textureUnit);
      setTexParameters();
    }
  }

  void TextureGL::upload2DImpl(const Texture::DataInfo & texture, const QRegion & region,
                               bool compressedFormat, bool mipmapsEnabled)
  {
    /// @todo glCompressedTexImage2D, probably needs some alignment
    if(compressedFormat) {
      m_state.opengl().glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.size.x, texture.size.y,
                                                 texture.dataFormat.compression(), texture.dataSize, texture.data.get());
      GLERROR("TextureGL::upload # glCompressedTexSubImage2D");
    } else {
      const int lineSizeBytes = texture.lineSizeBytes;
      const int bytesPerPixel = texture.dataFormat.bytesPerPixel();

      // Set proper alignment
      int alignment = 8;
      while (lineSizeBytes % alignment)
        alignment >>= 1;

      m_state.opengl().glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
      GLERROR("TextureGL::upload # glPixelStorei GL_UNPACK_ALIGNMENT");
      m_state.opengl().glPixelStorei(GL_UNPACK_ROW_LENGTH, lineSizeBytes / bytesPerPixel);
      GLERROR("TextureGL::upload # glPixelStorei GL_UNPACK_ROW_LENGTH");

      for(const QRect & rect : region.rects()) {
        const int lineSizeBytes = texture.lineSizeBytes;

        auto offset = rect.left() * bytesPerPixel + rect.top() * lineSizeBytes;
        auto data = static_cast<const char *>(texture.data.get()) + offset;

        uploadData(texture.dataFormat, data, offset, rect,
                   rect.height() * lineSizeBytes);
      }
    }
    if (mipmapsEnabled) {
      m_state.opengl().glGenerateMipmap(GL_TEXTURE_2D);
      GLERROR("TextureGL::upload2D # glGenerateMipmap");
    }
  }

  void TextureGL::upload3D(const Texture & texture, int textureUnit)
  {
    bool bound = false;

    bool paramsDirty = updateParams(texture);
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
        m_internalFormat = texture.internalFormat();
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
        m_state.opengl().glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE, texture.samples(), intFormat,
                                                 texture.width(), texture.height(), texture.depth(), GL_FALSE);
      } else {
        m_state.opengl().glTexImage3D(GL_TEXTURE_3D, 0, intFormat, texture.width(), texture.height(), texture.depth(), 0,
                                      texture.dataFormat().layout(), texture.dataFormat().type(), nullptr);
      }
      GLERROR("TextureGL::upload # glTexImage3D");
      paramsDirty = true;
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

      const int lineSizeBytes = texture.lineSizeBytes();
      const int bytesPerPixel = texture.dataFormat().bytesPerPixel();

      // Set proper alignment
      int alignment = 8;
      while (lineSizeBytes % alignment)
        alignment >>= 1;

      m_state.opengl().glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
      GLERROR("TextureGL::upload # glPixelStorei");
      m_state.opengl().glPixelStorei(GL_UNPACK_ROW_LENGTH, lineSizeBytes / bytesPerPixel);
      GLERROR("TextureGL::upload # glPixelStorei");

      int uploaded = texture.dataSize();
      m_state.opengl().glTexSubImage3D(m_target, 0, 0, 0, 0, texture.width(), texture.height(), texture.depth(),
                                       texture.dataFormat().layout(), texture.dataFormat().type(), texture.data().get());
      GLERROR("TextureGL::upload3D # glTexSubImage3D");

      if (texture.mipmapsEnabled()) {
        m_state.opengl().glGenerateMipmap(m_target);
        GLERROR("TextureGL::upload3D # glGenerateMipmap");
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

  void TextureGL::uploadData(const PixelFormat & dataFormat, const char * data, unsigned int destOffset,
                             const QRect & destRect, unsigned int bytes)
  {
    if (s_defaultUploadMethod == METHOD_TEXTURE) {
      m_state.opengl().glTexSubImage2D(GL_TEXTURE_2D, 0, destRect.left(), destRect.top(),
                                       destRect.width(), destRect.height(),
                                       dataFormat.layout(), dataFormat.type(), data);
      GLERROR("TextureGL::uploadData # glTexSubImage2D");
    } else {
      if (!m_uploadBuffer)
        m_uploadBuffer.reset(new BufferGL(m_state, Buffer::DYNAMIC_DRAW));

      if (s_defaultUploadMethod == METHOD_BUFFER_UPLOAD) {
        m_uploadBuffer->upload(Buffer::UNPACK, destOffset, bytes, data);
      } else if (s_defaultUploadMethod == METHOD_BUFFER_MAP ||
                 s_defaultUploadMethod == METHOD_BUFFER_MAP_NOSYNC ||
                 s_defaultUploadMethod == METHOD_BUFFER_MAP_NOSYNC_ORPHAN) {
        Radiant::FlagsT<Buffer::MapAccess> flags = Buffer::MAP_WRITE;
        if (s_defaultUploadMethod == METHOD_BUFFER_MAP ||
            s_defaultUploadMethod == METHOD_BUFFER_MAP_NOSYNC_ORPHAN)
          flags |= Buffer::MAP_INVALIDATE_BUFFER;
        if (s_defaultUploadMethod == METHOD_BUFFER_MAP_NOSYNC ||
            s_defaultUploadMethod == METHOD_BUFFER_MAP_NOSYNC_ORPHAN)
          flags |= Buffer::MAP_UNSYNCHRONIZED;
        void * target = m_uploadBuffer->map(Buffer::UNPACK, destOffset, bytes, flags);
        memcpy(target, data, bytes);
        m_uploadBuffer->unmap(Buffer::UNPACK, destOffset, bytes);
      } else {
        Radiant::error("TextureGL::uploadData # Unknown upload method %d", s_defaultUploadMethod);
        return;
      }

      m_state.opengl().glTexSubImage2D(GL_TEXTURE_2D, 0, destRect.left(), destRect.top(),
                                       destRect.width(), destRect.height(),
                                       dataFormat.layout(), dataFormat.type(),
                                       (const void *)(uintptr_t)destOffset);
      GLERROR("TextureGL::uploadData # glTexSubImage2D");

      m_state.opengl().glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      GLERROR("TextureGL::uploadData # glBindBuffer");
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

  void TextureGL::upload(const Texture & texture, int textureUnit)
  {
    // Reset usage timer
    touch();

    switch (texture.dimensions())
    {
    case 1: upload1D(texture, textureUnit); break;
    case 2: upload2D(texture, textureUnit); break;
    case 3: upload3D(texture, textureUnit); break;
    default:
      Radiant::error("TextureGL::upload # Error: unknown number of dimensions (%d) while trying to upload texture", texture.dimensions());
      assert(false);
    }
  }

  TextureGL::UploadMethod TextureGL::defaultUploadMethod()
  {
    return s_defaultUploadMethod;
  }

  void TextureGL::setDefaultUploadMethod(TextureGL::UploadMethod method)
  {
    s_defaultUploadMethod = method;
  }

  bool TextureGL::isAsyncUploadingEnabled()
  {
    return s_asyncUploadingEnabled;
  }

  void TextureGL::setAsyncUploadingEnabled(bool enabled)
  {
    s_asyncUploadingEnabled = enabled;
  }

  void TextureGL::setTexParameters() const
  {
    if(m_samples == 0) {
      m_state.opengl().glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, m_minFilter);
      GLERROR("TextureGL::upload # glTexParameteri");
      m_state.opengl().glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, m_magFilter);
      GLERROR("TextureGL::upload # glTexParameteri");

      m_state.opengl().glTexParameteri(m_target, GL_TEXTURE_WRAP_S, getWrapMode(m_wrap[0]));
      GLERROR("TextureGL::upload # glTexParameteri");
      m_state.opengl().glTexParameteri(m_target, GL_TEXTURE_WRAP_T, getWrapMode(m_wrap[1]));
      GLERROR("TextureGL::upload # glTexParameteri");
      m_state.opengl().glTexParameteri(m_target, GL_TEXTURE_WRAP_R, getWrapMode(m_wrap[2]));
      GLERROR("TextureGL::upload # glTexParameteri");

      m_state.opengl().glTexParameterfv(m_target, GL_TEXTURE_BORDER_COLOR, m_borderColor.data());
      GLERROR("TextureGL::upload # glTexParameterfv GL_TEXTURE_BORDER_COLOR");
    }
  }
}
