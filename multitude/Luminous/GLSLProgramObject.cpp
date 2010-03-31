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

#include <Luminous/GLSLProgramObject.hpp>

#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

using namespace std;

namespace Luminous
{

  using namespace Radiant;

  GLSLProgramObject::GLSLProgramObject(GLResources * resources)
      : GLResource(resources),
      m_isLinked(false)
  {
    m_handle = glCreateProgram();
  }

  GLSLProgramObject::~GLSLProgramObject()
  {
    GLSLProgramObject::clear();

    glDeleteProgram(m_handle);
  }

  void GLSLProgramObject::addObject(GLSLShaderObject* obj)
  {
    if(obj == 0) {
      error("GLSLProgramObject::addObject # attempt to add "
            "null shader object");
      return;
    }

    if(!obj->m_isCompiled) {
      debug("GLSLProgramObject::addObject # attempt to add "
            "non-compiled object: trying to compile it...");
      if(!obj->compile()) {
        error("GLSLProgramObject::addObject # compilation failed");
        return;
      } else {
        debug("Shader compilation ok");
      }
    }

    m_shaderObjects.push_back(obj);
  }

  bool GLSLProgramObject::link()
  {
    list<GLSLShaderObject*>::iterator i;

    if(m_isLinked) {
      error("GLSLProgramObject::link # program already "
            "linked, trying to re-link");
      for(i = m_shaderObjects.begin(); i != m_shaderObjects.end(); i++) {
        glDetachShader(m_handle, (*i)->m_handle);
      }
    }

    for(i = m_shaderObjects.begin(); i != m_shaderObjects.end(); i++) {
      glAttachShader(m_handle, (*i)->m_handle);
    }

    glLinkProgram(m_handle);

    GLint linked;
    glGetProgramiv(m_handle, GL_LINK_STATUS, &linked);

    if(linked) {
      m_isLinked = true;
      const char * log = linkerLog();
      if(log)
        debug("GLSLProgramObject::link # log:\n%s", log);
    } else  {
      error("GLSLProgramObject::link # linking failed");
      m_isLinked = false;
    }

    return m_isLinked;
  }

  void GLSLProgramObject::clear()
  {
    std::list<GLSLShaderObject*>::iterator i;
    for(i = m_shaderObjects.begin(); i != m_shaderObjects.end(); i++) {
      glDetachShader(m_handle, (*i)->m_handle);
      delete (*i);
    }

    m_shaderObjects.clear();

    m_isLinked = false;
  }

  const char* GLSLProgramObject::linkerLog()
  {
    if(m_handle == 0) {
      error("GLSLProgramObject::linkerLog # program object is null");
      return 0;
    }

#if 0
    GLint logLen = 0;
    glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &logLen);

    if(logLen == 0) {
      m_linkerLog.clear();
      return 0;
    }

    m_linkerLog.resize(logLen);

    GLsizei readLen;
    glGetProgramInfoLog(m_handle, logLen, &readLen, & m_linkerLog[0]);
#else
    m_linkerLog.resize(512);

    glGetProgramInfoLog(m_handle, 512, 0, & m_linkerLog[0]);
#endif

    return & m_linkerLog[0];
  }

  void GLSLProgramObject::bind()
  {
    if(m_handle == 0) {
      error("GLSLProgramObject::bind # attempt to bind null program");
      return;
    }

    if(!m_isLinked) {
      error("GLSLProgramObject::bind # attempt to "
            "bind program that is not linked");
      return;
    }

    glUseProgram(m_handle);
  }

  void GLSLProgramObject::unbind()
  {
    glUseProgram(0);
  }

  int GLSLProgramObject::getUniformLoc(const std::string& name)
  {
    return glGetUniformLocation(m_handle, name.c_str());
  }

  int GLSLProgramObject::getUniformLoc(const char * name)
  {
    return glGetUniformLocation(m_handle, name);
  }

  int GLSLProgramObject::getAttribLoc(const std::string & name)
  {
    return glGetAttribLocation(m_handle, name.c_str());
  }

  int GLSLProgramObject::getAttribLoc(const char * name)
  {
    return glGetAttribLocation(m_handle, name);
  }

  bool GLSLProgramObject::setUniformFloat(const char * name, float value)
  {
    int loc = getUniformLoc(name);

    if(loc < 0)
      return false;

    glUniform1f(loc, value);

    return true;
  }

  bool GLSLProgramObject::setUniformInt(const char * name, int value)
  {
    int loc = getUniformLoc(name);

    if(loc < 0)
      return false;

    glUniform1i(loc, value);

    return true;
  }

  bool GLSLProgramObject::setUniformVector2(const char * name,
                                            Nimble::Vector2f value)
  {
    int loc = getUniformLoc(name);

    if(loc < 0)
      return false;

    glUniform2f(loc, value.x, value.y);

    return true;
  }

  bool GLSLProgramObject::validate()
  {
    glValidateProgram(m_handle);

    GLint status;
    glGetProgramiv(m_handle, GL_VALIDATE_STATUS, &status);

    return (status == GL_TRUE);
  }

  GLSLProgramObject* GLSLProgramObject::fromFiles
      (const char* vsFile, const char* fsFile)
  {
    if(vsFile == 0 && fsFile == 0) {
      return 0;
    }

    // Load & compile vertex shader
    GLSLShaderObject* vs = 0;
    if(vsFile) {
      vs = new GLSLShaderObject(GL_VERTEX_SHADER);

      char* code = Radiant::FileUtils::loadTextFile(vsFile);
      vs->setSource(code);

      delete [] code;

      if(!vs->compile()) {
        error("GLSLProgramObject::fromFiles # vertex shader compile error: %s",
              vs->compilerLog());
        delete vs;
        return 0;
      }
    }

    // Load & compile fragment shader
    GLSLShaderObject* fs = 0;
    if(fsFile) {
      fs = new GLSLShaderObject(GL_FRAGMENT_SHADER);

      char* code = Radiant::FileUtils::loadTextFile(fsFile);
      fs->setSource(code);

      delete [] code;

      if(!fs->compile()) {
        error("GLSLProgramObject::fromFiles # fragment shader "
              "compile error:%s", fs->compilerLog());
        delete fs;
        return 0;
      }
    }

    // Create a program object and link it
    GLSLProgramObject* program = new GLSLProgramObject();

    if(vs) program->addObject(vs);
    if(fs) program->addObject(fs);

    if(!program->link()) {
      error("GLSLProgramObject::fromFiles # linking shader failed:\n%s",
            program->linkerLog());
      delete program;
      return 0;
    }

    return program;
  }

  GLSLProgramObject* GLSLProgramObject::fromStrings
      (const char* vsString, const char* fsString)
  {
    if(vsString == 0 && fsString == 0) {
      return 0;
    }

    GLSLProgramObject* program = new GLSLProgramObject();

    if(!program->loadStrings(vsString, fsString)) {
      delete program;
      return 0;
    }

    return program;
  }

  bool GLSLProgramObject::loadStrings(const char* vsString, const char* fsString)
  {
    if(vsString == 0 && fsString == 0) {
      return false;
    }

    info("GLSLProgramObject::loadStrings # %p %p", vsString, fsString);

    // Load & compile vertex shader
    GLSLShaderObject* vs = 0;
    if(vsString) {
      vs = new GLSLShaderObject(GL_VERTEX_SHADER);

      vs->setSource(vsString);

      if(!vs->compile()) {
        error("GLSLProgramObject::fromStrings # vertex shader compile error:\n%s",
              vs->compilerLog());
        delete vs;
        return 0;
      }
    }

    // Load & compile fragment shader
    GLSLShaderObject* fs = 0;
    if(fsString) {
      fs = new GLSLShaderObject(GL_FRAGMENT_SHADER);

      fs->setSource(fsString);

      if(!fs->compile()) {
        error("GLSLProgramObject::fromStrings # fragment shader "
              "compile error:\n%s", fs->compilerLog());
        delete fs;
        return 0;
      }
    }

    if(vs) addObject(vs);
    if(fs) addObject(fs);

    if(!link()) {
      error("GLSLProgramObject::fromStrings # linking shader failed:\n%s",
            linkerLog());
      return false;
    }


    return true;
  }

}

