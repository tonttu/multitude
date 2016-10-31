/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ProgramGL.hpp"

#include "Luminous/Program.hpp"

#include <Radiant/Mutex.hpp>

#include <QByteArray>
#include <QStringList>
#include <cassert>

/// @todo this file contains some (almost blindly) copy-pasted code from RenderDriverGL,
///       need to go through everything

namespace Luminous
{
  ShaderGL::ShaderGL(OpenGLAPI & opengl)
    : m_handle(0)
    , m_opengl(opengl)
  {
  }

  ShaderGL::~ShaderGL() noexcept
  {
    if(m_handle) {
      m_opengl.glDeleteShader(m_handle);
      GLERROR("ShaderGL::~ShaderGL # glDeleteShader");
    }
  }

  ShaderGL::ShaderGL(ShaderGL && shader) noexcept
    : m_handle(shader.m_handle)
    , m_opengl(shader.m_opengl)
  {
    shader.m_handle = 0;
  }

  ShaderGL & ShaderGL::operator=(ShaderGL && shader) noexcept
  {
    std::swap(m_handle, shader.m_handle);
    return *this;
  }

  bool ShaderGL::compile(const Shader & shader)
  {
    if(!m_handle) {
      switch (shader.type())
      {
      case Shader::Vertex:
        m_handle = m_opengl.glCreateShader(GL_VERTEX_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_VERTEX_SHADER)");
        break;
      case Shader::Fragment:
        m_handle = m_opengl.glCreateShader(GL_FRAGMENT_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_FRAGMENT_SHADER)");
        break;
      case Shader::Geometry:
        m_handle = m_opengl.glCreateShader(GL_GEOMETRY_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_GEOMETRY_SHADER)");
        break;
#if !defined (RADIANT_OSX)
      case Shader::TessControl:
        m_handle = m_opengl.glCreateShader(GL_TESS_CONTROL_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_TESS_CONTROL_SHADER)");
        break;
      case Shader::TessEval:
        m_handle = m_opengl.glCreateShader(GL_TESS_EVALUATION_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_TESS_EVALUATION_SHADER)");
        break;
      case Shader::Compute:
        m_handle = m_opengl.glCreateShader(GL_COMPUTE_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_COMPUTE_SHADER)");
        Radiant::warning("ShaderGL::compile # Compute shaders not fully implemented yet");
        break;
#endif
      default:
        Radiant::error("Unknown shader type/Not implemented on this platform");
        return false;
      }
    }

    // Set and compile source
    const QByteArray shaderData = shader.text();
    const GLchar * text = shaderData.data();
    const GLint length = shaderData.size();
    m_opengl.glShaderSource(m_handle, 1, &text, &length);
    GLERROR("ShaderGL::compile # glShaderSource");
    m_opengl.glCompileShader(m_handle);
    GLERROR("ShaderGL::compile # glCompileShader");
    GLint compiled = GL_FALSE;
    m_opengl.glGetShaderiv(m_handle, GL_COMPILE_STATUS, &compiled);
    GLERROR("ShaderGL::compile # glGetShaderiv");
    if (compiled != GL_TRUE) {
      Radiant::error("Failed to compile shader %s", shader.filename().toUtf8().data());

      // Dump info log
      GLsizei len;
      m_opengl.glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &len);
      GLERROR("ShaderGL::compile # glGetShaderiv");
      std::vector<GLchar> log(len);
      m_opengl.glGetShaderInfoLog(m_handle, len, &len, log.data());
      GLERROR("ShaderGL::compile # glGetShaderInfoLog");
      Radiant::error("%s", log.data());
      return false;
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ProgramGL::ProgramGL(StateGL & state, const Program & program)
    : ResourceHandleGL(state)
    , m_vertexDescription(program.vertexDescription())
    , m_sampleShading(0.0f)
    , m_linked(false)
  {
    m_handle = m_state.opengl().glCreateProgram();
    GLERROR("ProgramGL::ProgramGL # glCreateProgram");
  }

  ProgramGL::ProgramGL(ProgramGL && program)
    : ResourceHandleGL(std::move(program))
    , m_shaders(std::move(program.m_shaders))
    , m_attributes(std::move(program.m_attributes))
    , m_uniforms(std::move(program.m_uniforms))
    , m_uniformBlocks(std::move(program.m_uniformBlocks))
    , m_vertexDescription(std::move(program.m_vertexDescription))
    , m_sampleShading(std::move(program.m_sampleShading))
    , m_linked(program.m_linked)
  {
  }

  ProgramGL & ProgramGL::operator=(ProgramGL && program)
  {
    ResourceHandleGL::operator=(std::move(program));
    std::swap(m_shaders, program.m_shaders);
    std::swap(m_attributes, program.m_attributes);
    std::swap(m_uniforms, program.m_uniforms);
    std::swap(m_uniformBlocks, program.m_uniformBlocks);
    std::swap(m_vertexDescription, program.m_vertexDescription);
    std::swap(m_sampleShading, program.m_sampleShading);
    std::swap(m_linked, program.m_linked);
    return *this;
  }

  ProgramGL::~ProgramGL()
  {
    if(m_handle) {
      m_state.opengl().glDeleteProgram(m_handle);
      GLERROR("ProgramGL::~ProgramGL # glDeleteProgram");
    }
  }

  void ProgramGL::bind()
  {
    touch();
    // Avoid re-applying the same shader
    if(m_state.setProgram(m_handle)) {
      m_state.opengl().glUseProgram(m_handle);
      GLERROR("ProgramGL::bind # glUseProgram");
      m_state.opengl().glMinSampleShading(m_sampleShading);
      GLERROR("ProgramGL::bind # glMinSampleShading");
    }
  }

  void ProgramGL::bind(const Program & program)
  {
    link(program);
    bind();
  }

  static Radiant::Mutex s_linkAndCompileLock;
  void ProgramGL::link(const Program & program)
  {
    if(m_linked)
      return;

    /// AMD FirePro driver 14.301.1010 randomly crashes on
    /// glGetShaderiv(m_handle, GL_COMPILE_STATUS, &compiled) if compiling
    /// shaders from multiple contexts/threads at the same time. Try to work
    /// around this issue by having a lock here.
    Radiant::Guard g(s_linkAndCompileLock);

    /// @todo better error handling

    /// @todo we should have ShaderGL sharing through driver, for now they aren't shared
    m_shaders.clear();

    for(std::size_t i = 0; i < program.shaderCount(); ++i) {
      Shader & shader = program.shader(i);
      m_shaders.emplace_back(new ShaderGL(m_state.opengl()));
      std::unique_ptr<ShaderGL> & shadergl = m_shaders.back();
      shadergl->compile(shader);

      // Attach to the program
      m_state.opengl().glAttachShader(m_handle, shadergl->handle());
      GLERROR("ProgramGL::link # glAttachShader");
    }

    m_state.opengl().glLinkProgram(m_handle);
    GLERROR("ProgramGL::link # glLinkProgram");
    // Check for linking errors
    GLint status;
    m_state.opengl().glGetProgramiv(m_handle, GL_LINK_STATUS, &status);
    GLERROR("ProgramGL::link # glGetProgramiv");

    if(status == GL_FALSE) {
      Radiant::error("Failed to link shader program (shaders %s)", program.shaderFilenames().join(", ").toUtf8().data());
      GLsizei len;
      m_state.opengl().glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &len);
      GLERROR("ProgramGL::link # glGetProgramiv");
      std::vector<GLchar> log(len);
      m_state.opengl().glGetProgramInfoLog(m_handle, len, &len, log.data());
      GLERROR("ProgramGL::link # glGetProgramInfoLog");
      Radiant::error("%s", log.data());
    } else {
      // Clear the location caches
      m_attributes.clear();
      m_uniformBlocks.clear();
      m_uniforms.clear();
      // m_baseDescription = uniformDescription(programHandle, "BaseBlock");

      GLchar name[128]; // Name of variable
      GLsizei length;   // length of name
      GLint size;       // Size of variable
      GLenum type;      // Type of variable

      GLint count;      // Number of variables

      // Query all names of the vertex attributes
      /// @todo store size/type data in ShaderVariable
      m_state.opengl().glGetProgramiv(m_handle, GL_ACTIVE_ATTRIBUTES, &count);
      GLERROR("ProgramGL::link # glGetProgramiv");
      for (GLint i = 0; i < count; ++i) {
        m_state.opengl().glGetActiveAttrib(m_handle, i, sizeof(name), &length, &size, &type, name);
        GLERROR("ProgramGL::link # glGetActiveAttrib");
        m_attributes[name] = m_state.opengl().glGetAttribLocation(m_handle, name);
      }

      // Query all names of the uniform
      /// @todo store size/type data in ShaderVariable
      m_state.opengl().glGetProgramiv(m_handle, GL_ACTIVE_UNIFORMS, &count);
      GLERROR("ProgramGL::link # glGetProgramiv");
      for (GLint i = 0; i < count; ++i) {
        m_state.opengl().glGetActiveUniform(m_handle, i, sizeof(name), &length, &size, &type, name);
        GLERROR("ProgramGL::link # glGetActiveUniform");
        m_uniforms[name] = m_state.opengl().glGetUniformLocation(m_handle, name);
      }

      // Query all names of the uniform blocks
      m_state.opengl().glGetProgramiv(m_handle, GL_ACTIVE_UNIFORM_BLOCKS, &count);
      GLERROR("ProgramGL::link # glGetProgramiv");
      for (GLint i = 0; i < count; ++i) {
        m_state.opengl().glGetActiveUniformBlockName(m_handle, i, sizeof(name), &length, name);
        GLERROR("ProgramGL::link # glGetActiveUniformBlockName");
        m_uniformBlocks[name] = m_state.opengl().glGetUniformBlockIndex(m_handle, name);
        /// @todo get more info with glGetActiveUniformBlock
      }
    }
    m_vertexDescription = program.vertexDescription();
    m_sampleShading = program.sampleShading();

    if (m_vertexDescription.attributeCount() == 0)
      Radiant::warning("ProgramGL::link # shader %d (%s) has no vertex attributes defined. Did you forget to assign a vertex description?", handle(), program.shaderFilenames().join(", ").toUtf8().data());

    m_linked = true;
  }

  int ProgramGL::attributeLocation(const QByteArray & name)
  {
    auto it = m_attributes.find(name);
    if(it == m_attributes.end())
      return -1;
    return it->second;
  }

  int ProgramGL::uniformLocation(const QByteArray & name)
  {
    auto it = m_uniforms.find(name);
    if(it == m_uniforms.end()) {
      /// Uniforms that have an array index / different kind of notation
      /// (foo vs foo[0]) might not be in the m_uniforms-map, yet
      int loc = m_state.opengl().glGetUniformLocation(m_handle, name.data());
      GLERROR("ProgramGL::uniformLocation");
      m_uniforms[name] = loc;
      return loc;
    }
    return it->second;
  }

  int ProgramGL::uniformBlockLocation(const QByteArray & name)
  {
    auto it = m_uniformBlocks.find(name);
    if(it == m_uniformBlocks.end())
      return -1;
    return it->second;
  }

#if 0
  UniformDescription uniformDescription(const ProgramHandle & programHandle, const QByteArray & blockName)
  {
    /// @todo error checking/handling

    UniformDescription desc;

    GLuint blockIndex = glGetUniformBlockIndex(programHandle.handle, blockName.data());
    if(blockIndex == GL_INVALID_INDEX)
      return desc;

    int uniformCount = 0;
    glGetProgramiv(programHandle.handle, GL_ACTIVE_UNIFORMS, &uniformCount);

    for(GLuint uniformIndex = 0, c = uniformCount; uniformIndex < c; ++uniformIndex) {
      GLint nameLength = 0, size, blockIndex = 0, offset = 0;
      glGetActiveUniformsiv(programHandle.handle, 1, &uniformIndex, GL_UNIFORM_NAME_LENGTH, &nameLength);
      glGetActiveUniformsiv(programHandle.handle, 1, &uniformIndex, GL_UNIFORM_SIZE, &size);
      glGetActiveUniformsiv(programHandle.handle, 1, &uniformIndex, GL_UNIFORM_BLOCK_INDEX, &blockIndex);
      glGetActiveUniformsiv(programHandle.handle, 1, &uniformIndex, GL_UNIFORM_OFFSET, &offset);

      if(blockIndex == GLint(blockIndex) && nameLength > 0 && offset >= 0) {
        QByteArray ba(nameLength, '0');
        GLsizei len = 0;
        glGetActiveUniformName(programHandle.handle, uniformIndex, ba.size(), &len, ba.data());
        ba.resize(len);
        desc.addAttribute(ba, offset, size);
      }
    }

    return desc;
  }
#endif
} // namespace Luminous
