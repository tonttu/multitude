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

    enum UploadFlag
    {
      /// No special flags, upload everything immediately
      UPLOAD_SYNC  = 0,
      /// Only start uploading data in a background thread, or check if the
      /// uploading is finished
      UPLOAD_ASYNC = 1 << 0,
    };
    typedef Radiant::FlagsT<UploadFlag> UploadFlags;

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
    /// @returns true if all data was uploaded.
    ///          Can be non-true only if UPLOAD_ASYNC was given.
    LUMINOUS_API bool upload(const Texture & texture, int textureUnit, UploadFlags flags);
    /// Bind the texture to the given texture unit
    /// @param textureUnit texture unit to bind to
    inline void bind(int textureUnit);

    LUMINOUS_API bool isUploaded(Texture & texture);

    /// Returns multi-sampling count or zero if this is not a multi-sampled texture
    inline int samples() const { return m_samples; }

    inline int generation() const { return m_generation; }
    inline void setGeneration(int generation) { m_generation = generation; }

    inline GLenum target() const { return m_target; }
    inline void setTarget(GLenum target) { m_target = target; }

    inline int paramsGeneration() const { return m_paramsGeneration; }
    inline void setParamsGeneration(int generation) { m_paramsGeneration = generation; }

    LUMINOUS_API static UploadMethod defaultUploadMethod();
    LUMINOUS_API static void setDefaultUploadMethod(UploadMethod method);

    LUMINOUS_API static bool isAsyncUploadingEnabled();
    LUMINOUS_API static void setAsyncUploadingEnabled(bool enabled);

  private:
    void upload1D(const Texture & texture, int textureUnit);
    bool upload2D(const Texture & texture, int textureUnit, UploadFlags flags);
    void upload2DImpl(const Texture::DataInfo& texture, const QRegion & region, bool compressedFormat, bool mipmapsEnabled);
    void upload3D(const Texture & texture, int textureUnit);
    void uploadData(const PixelFormat & dataFormat, const char * data,
                    const QRect & destRect, unsigned int bytes,
                    UploadMethod method);
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
}

#endif // LUMINOUS_TEXTUREGL_HPP
