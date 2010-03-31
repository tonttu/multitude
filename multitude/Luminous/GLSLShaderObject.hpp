/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
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

namespace Luminous 
{

  /// OpenGL shading language shader object  
  class LUMINOUS_API GLSLShaderObject : public GLResource, public Patterns::NotCopyable
  {
  public:
    GLSLShaderObject(GLenum type, GLResources * resources = 0);
    ~GLSLShaderObject();

    bool compile();
    const char* compilerLog();   

    void setSource(const char* code);

  protected:

    char*   m_compilerLog;
    bool    m_isCompiled;
    GLchar* m_shaderSource;
    GLuint  m_handle;

    friend class GLSLProgramObject;
  };

}

#endif

