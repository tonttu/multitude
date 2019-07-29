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
#include "BufferGL.hpp"

#include <QMutex>
#include <QWaitCondition>

#include <Nimble/Rect.hpp>
#include <Nimble/Vector3.hpp>

#include <QRegion>

namespace Luminous
{

  /// This class represents a Texture object in GPU memory.
  class TextureGL : public ResourceHandleGL
  {
  public:
    /// How is the texture data uploaded to the GPU
    enum UploadMethod
    {
      /// Simple synchronous method using glTexSubImage2D. This is the default.
      METHOD_TEXTURE,
      /// Use an UNPACK buffer with synchronous BufferGL::upload (glBufferSubData)
      METHOD_BUFFER_UPLOAD,
      /// Use an UNPACK buffer, use synchronous BufferGL::map and memcpy
      METHOD_BUFFER_MAP,
      METHOD_BUFFER_MAP_NOSYNC,
      METHOD_BUFFER_MAP_NOSYNC_ORPHAN,
    };

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
    LUMINOUS_API void upload(const Texture & texture, int textureUnit);
    /// Bind the texture to the given texture unit
    /// @param textureUnit texture unit to bind to
    inline void bind(int textureUnit);

    inline void sync(int textureUnit);

    /// Returns multi-sampling count or zero if this is not a multi-sampled texture
    inline int samples() const { return m_samples; }

    LUMINOUS_API static UploadMethod defaultUploadMethod();
    LUMINOUS_API static void setDefaultUploadMethod(UploadMethod method);

    LUMINOUS_API static bool isAsyncUploadingEnabled();
    LUMINOUS_API static void setAsyncUploadingEnabled(bool enabled);

  private:
    void upload1D(const Texture & texture, int textureUnit);
    void upload2D(const Texture & texture, int textureUnit);
    void upload2DImpl(const Texture::DataInfo& texture, const QRegion & region, bool compressedFormat, bool mipmapsEnabled);
    void upload3D(const Texture & texture, int textureUnit);
    void uploadData(const PixelFormat & dataFormat, const char * data,
                    unsigned int destOffset, const QRect & destRect,
                    unsigned int bytes);
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

    std::unique_ptr<BufferGL> m_uploadBuffer;

    bool m_useAsyncUpload = true;
    int m_asyncUploadTasks = 0;
    std::vector<GLsync> m_fences;
    QMutex m_asyncUploadMutex;
    QWaitCondition m_asyncUploadCond;
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

  void TextureGL::sync(int textureUnit)
  {
    bind(textureUnit);
    while (true) {
      QMutexLocker g(&m_asyncUploadMutex);
      if (!m_fences.empty()) {
        GLsync sync = m_fences.front();
        m_fences.erase(m_fences.begin());
        g.unlock();
        m_state.opengl().glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
        m_state.opengl().glDeleteSync(sync);
        g.relock();
      }
      if (m_asyncUploadTasks > 0)
        m_asyncUploadCond.wait(&m_asyncUploadMutex);
      if (m_fences.empty() && m_asyncUploadTasks == 0)
        break;
    }
  }
}

#endif // LUMINOUS_TEXTUREGL_HPP
