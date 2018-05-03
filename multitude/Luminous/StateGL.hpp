/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_STATEGL_HPP
#define LUMINOUS_STATEGL_HPP

#include "Export.hpp"
#include "Luminous.hpp"

#include <Radiant/TimeStamp.hpp>

#include <map>
#include <algorithm>
#include <cstdint>

namespace Luminous
{
  class RenderDriverGL;

  /// Keeps track of current OpenGL state, one instance is shared between all
  /// *GL -classes in the same rendering context. None of these functions
  /// actually modify any OpenGL state. This class is used to minimize OpenGL
  /// state changes to improve rendering performance.
  class StateGL
  {
  public:
    /// Constructor
    /// @param threadIndex render thread index
    /// @param driver render driver to use
    inline StateGL(unsigned int threadIndex, RenderDriverGL & driver);

    /// Sets the current program object.
    /// @param handle handle to the program object
    /// @return true if current program was changed
    inline bool setProgram(GLuint handle);
    /// Get the current program object handle
    /// @return current program object handle
    inline GLuint program() const;

    /// Set the current vertex array
    /// @param handle vertex array handle
    /// @return true if the current vertex array was changed
    inline bool setVertexArray(GLuint handle);
    /// Get the current vertex array
    /// @return current vertex array handle
    inline GLuint vertexArray() const;

    /// Get the render thread index this state object was associated with
    /// @return render thread index
    inline unsigned int threadIndex() const;

    /// Get the number of bytes available for uploading content to the GPU.
    /// This limit is used to spread heavy upload operations across multiple
    /// rendering frames to avoid stalling the application.
    /// @return number of bytes available
    inline int64_t availableUploadBytes() const;

    /// Set the maximum and minimum number of bytes per second for uploading content to the GPU.
    /// @param limit maximum number of bytes to upload per second
    /// @param margin minimum number of bytes to upload per second
    inline void setUploadLimits(int64_t limit, int64_t margin);

    /// Set the target update frequency. This is only used to calculate
    /// frame-based upload limits
    /// @param fps target frames per second
    inline void setUpdateFrequency(int64_t fps);

    /// Get the maximum number of bytes per second used for uploading content to the GPU
    /// @return number of bytes per second
    inline int64_t uploadLimit() const;

    /// Get the minimum number of bytes per second used for uploading content to the GPU
    /// @return number of bytes per second
    inline int64_t uploadMargin() const;

    /// Consume the given amount of bytes from the upload budget. This function
    /// decreases the upload budget by the given amount.
    /// @param bytes number of bytes to consume
    /// @sa availableUploadBytes
    inline void consumeUploadBytes(int64_t bytes);
    /// Sets the number of available upload bytes to zero.
    /// @sa consumeUploadBytes
    inline void clearUploadedBytes();

    /// Get the render driver associated with this state
    /// @return render driver
    inline RenderDriverGL & driver() { return m_driver; }

    /// Set the current framebuffer
    /// @param target framebuffer target (GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER)
    /// @param handle handle to the framebuffer
    /// @return true if the current framebuffer was changed; otherwise false
    inline bool setFramebuffer(GLenum target, GLuint handle);
    /// Get the handle to the current read framebuffer
    /// @return framebuffer handle
    inline GLuint readFramebuffer() const;
    /// Get the handle to the current draw framebuffer
    /// @return framebuffer handle
    inline GLuint drawFramebuffer() const;

    /// Set the current active texture unit. Indexing starts from zero.
    /// @param index texture unit index
    inline bool setTextureUnit(int index);
    /// Get the current active texture unit
    /// @return active texture unit
    inline int textureUnit() const;

    /// Set the frame time. The frame time is used to keep track of resource
    /// expiration.
    /// @param t frame time
    void setFrameTime(Radiant::TimeStamp t) { m_frameTime = t; }
    /// Get the current frame time
    /// @return frame time
    Radiant::TimeStamp frameTime() const { return m_frameTime; }

    /// Return the OpenGL API for the associated render driver
    /// @todo inline would be nice, but circular dependencies don't allow it
    LUMINOUS_API OpenGLAPI& opengl();

  private:
    /// Currently bound shader program
    GLuint m_currentProgram;

    /// Currently bound vertex array object
    GLuint m_currentVertexArray;

    const unsigned int m_threadIndex;

    /// Uploaded bytes this frame
    int64_t m_uploadedBytes;
    int64_t m_uploadLimit;
    int64_t m_uploadMargin;
    int64_t m_updateFrequency;

    RenderDriverGL & m_driver;

    GLuint m_currentReadFrameBuffer;
    GLuint m_currentDrawFrameBuffer;

    int m_currentTextureUnit;

    Radiant::TimeStamp m_frameTime;
  };

  /////////////////////////////////////////////////////////////////////////////

  StateGL::StateGL(unsigned int threadIndex, RenderDriverGL &driver)
    : m_currentProgram(0)
    , m_currentVertexArray(0)
    , m_threadIndex(threadIndex)
    , m_uploadedBytes(0)
    , m_uploadLimit(0)
    , m_uploadMargin(0)
    , m_updateFrequency(60)
    , m_driver(driver)
    , m_currentReadFrameBuffer(0)
    , m_currentDrawFrameBuffer(0)
    , m_currentTextureUnit(0)
  {
  }

  bool StateGL::setProgram(GLuint handle)
  {
    bool changed = m_currentProgram != handle;
    m_currentProgram = handle;
    return changed;
  }

  GLuint StateGL::program() const
  {
    return m_currentProgram;
  }

  bool StateGL::setVertexArray(GLuint handle)
  {
    bool changed = m_currentVertexArray != handle;
    m_currentVertexArray = handle;
    return changed;
  }

  GLuint StateGL::vertexArray() const
  {
    return m_currentVertexArray;
  }

  unsigned int StateGL::threadIndex() const
  {
    return m_threadIndex;
  }

  int64_t StateGL::availableUploadBytes() const
  {
    // Return the available number of bytes for this frame.
    return std::max(uploadMargin()/m_updateFrequency, (uploadLimit()/m_updateFrequency - m_uploadedBytes));
  }

  void StateGL::setUploadLimits(int64_t limit, int64_t margin)
  {
    m_uploadLimit = limit;
    m_uploadMargin = margin;
  }

  void StateGL::setUpdateFrequency(int64_t fps)
  {
    m_updateFrequency = fps;
  }

  int64_t StateGL::uploadLimit() const
  {
    return m_uploadLimit;
  }

  int64_t StateGL::uploadMargin() const
  {
    return m_uploadMargin;
  }

  void StateGL::consumeUploadBytes(int64_t bytes)
  {
    m_uploadedBytes += bytes;
  }

  void StateGL::clearUploadedBytes()
  {
    m_uploadedBytes = 0;
  }

  bool StateGL::setFramebuffer(GLenum target, GLuint handle)
  {
    bool changed = false;
    if(target == GL_FRAMEBUFFER) {
      changed = (m_currentReadFrameBuffer != handle || m_currentDrawFrameBuffer != handle);
      m_currentReadFrameBuffer = handle;
      m_currentDrawFrameBuffer = handle;
    } else if(target == GL_READ_FRAMEBUFFER) {
      changed = (m_currentReadFrameBuffer != handle);
      m_currentReadFrameBuffer = handle;
    } else if(target == GL_DRAW_FRAMEBUFFER) {
      changed = (m_currentDrawFrameBuffer != handle);
      m_currentDrawFrameBuffer = handle;
    }
    return changed;
  }

  GLuint StateGL::readFramebuffer() const
  {
    return m_currentReadFrameBuffer;
  }

  GLuint StateGL::drawFramebuffer() const
  {
    return m_currentDrawFrameBuffer;
  }

  bool StateGL::setTextureUnit(int unit)
  {
    bool changed = unit != m_currentTextureUnit;
    m_currentTextureUnit = unit;
    return changed;
  }

  int StateGL::textureUnit() const
  {
    return m_currentTextureUnit;
  }

}
#endif // LUMINOUS_STATEGL_HPP
