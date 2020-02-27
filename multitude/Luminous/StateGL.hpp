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
    LUMINOUS_API StateGL(unsigned int threadIndex, RenderDriverGL & driver);

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
    inline OpenGLAPI & opengl() { assert(m_opengl); return *m_opengl; }
    /// Returns OpenGL 4.5 API, if supported
    inline OpenGLAPI45 * opengl45() { return m_opengl45; }

    LUMINOUS_API void addTask(std::function<void()> task);

    LUMINOUS_API void initGl();

  private:
    /// Currently bound shader program
    GLuint m_currentProgram = 0;

    /// Currently bound vertex array object
    GLuint m_currentVertexArray = 0;

    const unsigned int m_threadIndex;

    RenderDriverGL & m_driver;

    GLuint m_currentReadFrameBuffer = 0;
    GLuint m_currentDrawFrameBuffer = 0;

    int m_currentTextureUnit = 0;

    Radiant::TimeStamp m_frameTime;

    OpenGLAPI * m_opengl = nullptr;
    OpenGLAPI45 * m_opengl45 = nullptr;
  };

  /////////////////////////////////////////////////////////////////////////////

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
