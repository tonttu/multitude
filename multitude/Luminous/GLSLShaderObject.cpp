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


#include <Luminous/GLSLShaderObject.hpp>
#include <Luminous/Luminous.hpp>

#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <string.h>

namespace Luminous
{
  using namespace Radiant;

  GLSLShaderObject::GLSLShaderObject(GLenum shaderType, RenderContext * resources)
    : GLResource(resources),
      m_compilerLog(0),
      m_isCompiled(false),
      m_shaderSource(0)
  {
    m_handle = glCreateShader(shaderType);
    setPersistent(true);
  }

  GLSLShaderObject::~GLSLShaderObject()
  {
    delete[] m_compilerLog;
    delete[] m_shaderSource;

    glDeleteShader(m_handle);
  }

  bool GLSLShaderObject::compile()
  {
    m_isCompiled = false;

    if(m_shaderSource == 0)
    {
      error("GLSLShaderObject::compile # attempt to compile a shader "
            "with no source.");
      return false;
    }

    GLint len = (GLint)strlen((const char*)m_shaderSource);
    glShaderSource(m_handle, 1, (const GLchar**)&m_shaderSource, &len);

    glCompileShader(m_handle);

    GLint wasCompiled = 0;
    glGetShaderiv(m_handle, GL_COMPILE_STATUS, &wasCompiled);

    if(wasCompiled) m_isCompiled = true;

    return m_isCompiled;
  }

  const char* GLSLShaderObject::compilerLog()
  {
    if(m_handle == 0)
    {
      error("GLSLShaderObject::compilerLog # attempt to query null object.");
      return 0;
    }

    GLint logLen;
    glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &logLen);

    delete[] m_compilerLog;
    m_compilerLog = new char [logLen];

    GLsizei readLen;
    glGetShaderInfoLog(m_handle, logLen, &readLen, m_compilerLog);

    return m_compilerLog;
  }

  void GLSLShaderObject::setSource(const char* code)
  {

    delete[] m_shaderSource;

#ifdef LUMINOUS_OPENGL_FULL

    std::string tmp(code);
    static const char * removes [] = {
      " mediump ", " highp ", " lowp ", 0
    };

    for(int i = 0; removes[i]; i++) {
      const char * s;
      int len = strlen(removes[i]);
      while((s = strstr(tmp.c_str(), removes[i]))) {
        int place = s - tmp.c_str();
        tmp.erase(place, len);
        tmp.insert(place, " ");
      }
    }

    m_shaderSource = new char [tmp.size() + 1];

    strcpy(m_shaderSource, tmp.c_str());


#else
    int len = (int) strlen(code) + 1;
    m_shaderSource = new char [len];

    strcpy(m_shaderSource, code);
#endif
  }

  bool GLSLShaderObject::loadSourceFile(const char* filename)
  {
    char * str = Radiant::FileUtils::loadTextFile(filename);
    if(!str)
      return false;
    setSource(str);
    delete [] str;
    return true;
  }

  GLSLShaderObject * GLSLShaderObject::fromFile(GLenum type, const char* filename)
  {
    GLSLShaderObject * shader = new GLSLShaderObject(type);

    if(!shader->loadSourceFile(filename)) {
      error("GLSLShaderObject::fromFile # Could not load \"%s\"", filename);
      delete shader;
      return 0;
    }

    if(!shader->compile()) {
      error("GLSLShaderObject::fromFile # %s\n%s", filename, shader->compilerLog());
      delete shader;
      return 0;
    }

    return shader;
  }
}
