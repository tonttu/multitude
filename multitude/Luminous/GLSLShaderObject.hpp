/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_GLSLSHADEROBJECT_HPP
#define LUMINOUS_GLSLSHADEROBJECT_HPP

#include <Luminous/Luminous.hpp>
#include <Luminous/GLResource.hpp>

#include <Patterns/NotCopyable.hpp>

/// @cond

namespace Luminous
{

  /** OpenGL shading language shader object. Usually shader programs consist of
  multiple shader objects that are first compiled and then linked together to
  create the final shader program that can be executed.*/
  /// @deprecated this class is deprecated and will be removed in Cornerstone 2.1. Use Luminous::ShaderGLSL instead.
  class LUMINOUS_API GLSLShaderObject : public GLResource, public Patterns::NotCopyable
  {
  public:
    /// Creates a new shader of the given type.
    /// @param type either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
    /// @param resources resource collection
    GLSLShaderObject(GLenum type, RenderContext * resources = 0);
    ~GLSLShaderObject();

    /// Compiles the shader
    bool compile();

    /// Returns the compiler log of the shader
    const char* compilerLog();

    /// Sets the source code for the shader
    void setSource(const char* code);

    /// Returns the current source code for this shader
    const char * source() const
    { return m_shaderSource; }

    /// Loads the source for the shader from a given file
    bool loadSourceFile(const char* filename);

    /** Creates and loads a shader object from a file and compiles it.
    @param type type of the shader
    @param filename name of the source code file
    @return A compiled shader object, or NULL (if it could not be compiled). */
    static GLSLShaderObject * fromFile(GLenum type, const char* filename);

  private:

    char*   m_compilerLog;
    bool    m_isCompiled;
    GLchar* m_shaderSource;
    GLuint  m_handle;

    friend class GLSLProgramObject;
  };

}

/// @endcond

#endif

