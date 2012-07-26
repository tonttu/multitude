#ifndef LUMINOUS_STATEGL_HPP
#define LUMINOUS_STATEGL_HPP

#include "Luminous.hpp"

namespace Luminous
{
  /// Keeps track of current OpenGL state, one instance is shared between all *GL
  /// -classes in the same context
  /// None of these functions actually modify any OpenGL state
  class StateGL
  {
  public:
    inline StateGL();

    /// Sets the current program object
    /// @return true if current program was changed
    inline bool setProgram(GLuint handle);
    inline GLuint program() const;

    inline bool setVertexArray(GLuint handle);
    inline GLuint vertexArray() const;

  private:
    GLuint m_currentProgram;          /// Currently bound shader program
    GLuint m_currentVertexArray;      /// Currently bound vertex array object
  };

  /////////////////////////////////////////////////////////////////////////////

  StateGL::StateGL()
    : m_currentProgram(0)
    , m_currentVertexArray(0)
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
}
#endif // LUMINOUS_STATEGL_HPP
