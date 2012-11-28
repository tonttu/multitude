#ifndef LUMINOUS_STATEGL_HPP
#define LUMINOUS_STATEGL_HPP

#include "Luminous/Luminous.hpp"

#include <Radiant/TimeStamp.hpp>

#include <unordered_map>
#include <algorithm>
#include <cstdint>

namespace Luminous
{
  class RenderDriverGL;

  struct BufferMapping
  {
    BufferMapping() : target(0), access(0), offset(0), length(0), data(0) {}
    GLenum target;
    GLenum access;
    int offset;
    std::size_t length;
    void * data;
  };

  /// Keeps track of current OpenGL state, one instance is shared between all *GL
  /// -classes in the same context
  /// None of these functions actually modify any OpenGL state
  class StateGL
  {
  public:
    //This would probably be more efficient if using plain array, with some
    //safe-mechanisms, since keys are usually somtehing like: 0,1,2,3,...
    struct IdHash { std::size_t operator()(const GLuint& i) const { return i; } };
    typedef std::unordered_map<GLuint, BufferMapping, IdHash> BufferMaps;

    inline StateGL(unsigned int threadIndex, RenderDriverGL & driver);

    /// Sets the current program object
    /// @return true if current program was changed
    inline bool setProgram(GLuint handle);
    inline GLuint program() const;

    inline bool setVertexArray(GLuint handle);
    inline GLuint vertexArray() const;

    inline unsigned int threadIndex() const;

    inline int64_t availableUploadBytes() const;
    inline void consumeUploadBytes(int64_t bytes);
    inline void clearUploadedBytes();

    inline BufferMaps & bufferMaps();

    inline RenderDriverGL & driver() { return m_driver; }

    inline bool setFramebuffer(GLenum target, GLuint handle);
    inline GLuint readFramebuffer() const;
    inline GLuint drawFramebuffer() const;

    void setFrameTime(Radiant::TimeStamp t) { m_frameTime = t; }
    Radiant::TimeStamp frameTime() const { return m_frameTime; }

  private:
    /// Currently bound shader program
    GLuint m_currentProgram;

    /// Currently bound vertex array object
    GLuint m_currentVertexArray;

    const unsigned int m_threadIndex;

    /// Uploaded bytes this frame
    int64_t m_uploadedBytes;

    BufferMaps m_bufferMaps;

    RenderDriverGL & m_driver;

    GLuint m_currentReadFrameBuffer;
    GLuint m_currentDrawFrameBuffer;

    Radiant::TimeStamp m_frameTime;
  };

  /////////////////////////////////////////////////////////////////////////////

  StateGL::StateGL(unsigned int threadIndex, RenderDriverGL &driver)
    : m_currentProgram(0)
    , m_currentVertexArray(0)
    , m_threadIndex(threadIndex)
    , m_uploadedBytes(0)
    , m_driver(driver)
    , m_currentReadFrameBuffer(0)
    , m_currentDrawFrameBuffer(0)
  {}

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
    /// @todo this should be configurable
    /// PCIe bandwidth
    /// PCIe 1.0 x16: 4GB/sec (2001)
    /// PCIe 2.0 x16: 8GB/sec (2007)
    /// PCIe 3.0 x16: 15.8GB/sec (2011)
    const int64_t upload_bytes_limit_frame = ((uint64_t)4 << 30) / 60;
    return std::max(int64_t(0), upload_bytes_limit_frame - m_uploadedBytes);
  }

  void StateGL::consumeUploadBytes(int64_t bytes)
  {
    m_uploadedBytes += bytes;
  }

  void StateGL::clearUploadedBytes()
  {
    m_uploadedBytes = 0;
  }

  StateGL::BufferMaps & StateGL::bufferMaps()
  {
    return m_bufferMaps;
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

}
#endif // LUMINOUS_STATEGL_HPP
