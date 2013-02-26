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

#include "GLSLProgramObject.hpp"
#include "RenderContext.hpp"
#include "DummyOpenGL.hpp"

#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

namespace Luminous
{
  GLSLProgramObject::GLSLProgramObject(RenderContext * resources)
      : GLResource(resources),
      m_isLinked(false),
      m_errors(false)
  {
    m_handle = glCreateProgram();
    setPersistent(true);
  }

  GLSLProgramObject::~GLSLProgramObject()
  {
    GLSLProgramObject::clear();

    glDeleteProgram(m_handle);
    // Radiant::info("GLSLProgramObject::~GLSLProgramObject # %p", this);
  }

  void GLSLProgramObject::addObject(GLSLShaderObject* obj)
  {
    if(obj == 0) {
      Radiant::error("GLSLProgramObject::addObject # attempt to add "
            "null shader object");
      return;
    }

    if(!obj->m_isCompiled) {
      debugLuminous("GLSLProgramObject::addObject # attempt to add "
            "non-compiled object: trying to compile it...");
      if(!obj->compile()) {
        Radiant::error("GLSLProgramObject::addObject # compilation failed");
        return;
      } else {
        debugLuminous("Shader compilation ok");
      }
    }

    m_shaderObjects.push_back(obj);
  }

  bool GLSLProgramObject::link()
  {
    std::list<GLSLShaderObject*>::iterator i;

    if(m_isLinked) {
      Radiant::error("GLSLProgramObject::link # program already "
            "linked, trying to re-link");
      for(i = m_shaderObjects.begin(); i != m_shaderObjects.end(); ++i) {
        glDetachShader(m_handle, (*i)->m_handle);
      }
    }

    for(i = m_shaderObjects.begin(); i != m_shaderObjects.end(); ++i) {
      glAttachShader(m_handle, (*i)->m_handle);
    }

    glLinkProgram(m_handle);

    GLint linked = 1;
    glGetProgramiv(m_handle, GL_LINK_STATUS, &linked);

    if(linked) {
      m_isLinked = true;
      const char * log = linkerLog();
      if(log)
        debugLuminous("GLSLProgramObject::link # log:\n%s", log);
    } else  {
      const char * log = linkerLog();
      Radiant::error("GLSLProgramObject::link # linking failed, log: %s",
            log);
      m_isLinked = false;
    }

    return m_isLinked;
  }

  void GLSLProgramObject::clear()
  {
    std::list<GLSLShaderObject*>::iterator i;
    for(i = m_shaderObjects.begin(); i != m_shaderObjects.end(); ++i) {
      glDetachShader(m_handle, (*i)->m_handle);
      delete (*i);
    }

    m_shaderObjects.clear();

    m_isLinked = false;
  }

  const char* GLSLProgramObject::linkerLog()
  {
    if(m_handle == 0) {
      Radiant::error("GLSLProgramObject::linkerLog # program object is null");
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
      Radiant::error("GLSLProgramObject::bind # attempt to bind null program");
      return;
    }

    if(!m_isLinked) {
      Radiant::error("GLSLProgramObject::bind # attempt to "
            "bind program that is not linked");
      return;
    }

    if(!context()) {
      Radiant::fatal("GLSLProgramObject::bind # NULL context");
    }

    context()->bindProgram(this);
  }

  void GLSLProgramObject::unbind()
  {
    context()->bindProgram(0);
  }

  int GLSLProgramObject::getUniformLoc(const QByteArray& name)
  {
    return glGetUniformLocation(m_handle, name.data());
  }

  int GLSLProgramObject::getUniformLoc(const char * name)
  {
    return glGetUniformLocation(m_handle, name);
  }

  int GLSLProgramObject::getAttribLoc(const QByteArray & name)
  {
    return glGetAttribLocation(m_handle, name.data());
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

    if(loc < 0) {
      Radiant::error("GLSLProgramObject::setUniformVector2 # %s undefined", name);
      return false;
    }
    glUniform2f(loc, value.x, value.y);

    return true;
  }

  bool GLSLProgramObject::setUniformVector3(const char * name,
                                            Nimble::Vector3f value)
  {
    int loc = getUniformLoc(name);

    if(loc < 0)
      return false;

    glUniform3f(loc, value.x, value.y, value.z);

    return true;
  }

  bool GLSLProgramObject::setUniformVector4(const char * name,
                                            Nimble::Vector4f value)
  {
    int loc = getUniformLoc(name);

    if(loc < 0)
      return false;

    glUniform4f(loc, value.x, value.y, value.z, value.w);

    return true;
  }

  bool GLSLProgramObject::setUniformMatrix3(const char * name, const Nimble::Matrix3f & value)
  {
    int loc = getUniformLoc(name);

    if(loc < 0) {
      Radiant::error("GLSLProgramObject::setUniformMatrix3 # Uniform %s not found", name);
      return false;
    }
    // Radiant::info("GLSLProgramObject::setUniformMatrix3 # %d %s", loc, name);

#ifdef LUMINOUS_OPENGL_FULL
    glUniformMatrix3fv(loc, 1, true, value.data());
#else
    glUniformMatrix3fv(loc, 1, false, value.transposed().data());
#endif
    return true;
  }


  bool GLSLProgramObject::setUniformMatrix4(const char * name, const Nimble::Matrix4f & value)
  {
    int loc = getUniformLoc(name);

    if(loc < 0) {
      Radiant::error("GLSLProgramObject::setUniformMatrix4 # Uniform %s not found", name);
      return false;
    }

#ifdef LUMINOUS_OPENGL_FULL
    glUniformMatrix4fv(loc, 1, true, value.data());

#else
    glUniformMatrix4fv(loc, 1, false, value.transposed().data());
#endif

    return true;
  }

#ifndef LUMINOUS_OPENGLES

  void GLSLProgramObject::setProgramParameter(GLenum pname, GLint value)
  {
    glProgramParameteri(handle(), pname, value);
  }
#endif // LUMINOUS_OPENGLES


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

      const QByteArray code = Radiant::FileUtils::loadTextFile(vsFile);
      if(!code.isNull()) {
        vs = new GLSLShaderObject(GL_VERTEX_SHADER);
        vs->setSource(code.data());

        if(!vs->compile()) {
          Radiant::error("GLSLProgramObject::fromFiles # vertex shader %s compile error: %s",
                vsFile, vs->compilerLog());
          delete vs;
          return 0;
        }
      }
    }

    // Load & compile fragment shader
    GLSLShaderObject* fs = 0;
    if(fsFile) {
      const QByteArray code = Radiant::FileUtils::loadTextFile(fsFile);
      if(!code.isNull()) {
        fs = new GLSLShaderObject(GL_FRAGMENT_SHADER);
        fs->setSource(code.data());

        if(!fs->compile()) {
          Radiant::error("GLSLProgramObject::fromFiles # fragment shader %s "
                "compile error:%s", fsFile, fs->compilerLog());
          delete vs;
          delete fs;
          return 0;
        }
      }
    }

    if (!vs && !fs)
      return 0;

    // Create a program object and link it
    GLSLProgramObject* program = new GLSLProgramObject();

    if(vs) program->addObject(vs);
    if(fs) program->addObject(fs);

    if(!program->link()) {
      Radiant::error("GLSLProgramObject::fromFiles # linking shader failed:\n%s",
            program->linkerLog());
      delete vs;
      delete fs;
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

    //info("GLSLProgramObject::loadStrings # %p %p", vsString, fsString);

    // Load & compile vertex shader
    GLSLShaderObject* vs = 0;
    if(vsString) {
      vs = new GLSLShaderObject(GL_VERTEX_SHADER);

      vs->setSource(vsString);

      if(!vs->compile()) {
        Radiant::error("GLSLProgramObject::fromStrings # vertex shader compile error:\n%s",
              vs->compilerLog());
        Radiant::error("GLSLProgramObject::fromStrings # When compiling:\n%s\n",
              vsString);
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
        Radiant::error("GLSLProgramObject::fromStrings # fragment shader "
              "compile error:\n%s", fs->compilerLog());
        delete fs;
        if(vs) delete vs;
        return 0;
      }
    }

    if(vs) addObject(vs);
    if(fs) addObject(fs);

    if(!link()) {
      Radiant::error("GLSLProgramObject::fromStrings # linking shader failed:\n%s",
            linkerLog());
      return false;
    }

    return true;
  }

  bool GLSLProgramObject::loadFile(GLenum shaderType, const char * filename)
  {
    GLSLShaderObject * shader = new GLSLShaderObject(shaderType);
    if(!shader->loadSourceFile(filename)) {
      delete shader;
      return false;
    }

    if(!shader->compile()) {
      delete shader;
      return false;
    }

    addObject(shader);

    return true;
  }

  bool GLSLProgramObject::loadString(GLenum shaderType, const char * shaderCode)
  {
    assert(shaderCode != 0);

    GLSLShaderObject * shader = new GLSLShaderObject(shaderType);
    shader->setSource(shaderCode);

    if(!shader->compile()) {
      Radiant::error("GLSLProgramObject::loadString # Compilation failed : %s\n%s",
            shader->compilerLog(), shaderCode);
      delete shader;
      return false;
    }

    addObject(shader);

    return true;

  }

}

