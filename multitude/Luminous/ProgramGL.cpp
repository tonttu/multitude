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

#include <QByteArray>
#include <QStringList>
#include <cassert>

/// @todo this file contains some (almost blindly) copy-pasted code from RenderDriverGL,
///       need to go through everything

namespace Luminous
{
  ShaderGL::ShaderGL()
    : m_handle(0)
  {
  }

  ShaderGL::~ShaderGL() noexcept
  {
    if(m_handle) {
      glDeleteShader(m_handle);
      GLERROR("ShaderGL::~ShaderGL # glDeleteShader");
    }
  }

  ShaderGL::ShaderGL(ShaderGL && shader) noexcept
    : m_handle(shader.m_handle)
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
        m_handle = glCreateShader(GL_VERTEX_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_VERTEX_SHADER)");
        break;
      case Shader::Fragment:
        m_handle = glCreateShader(GL_FRAGMENT_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_FRAGMENT_SHADER)");
        break;
      case Shader::Geometry:
        m_handle = glCreateShader(GL_GEOMETRY_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_GEOMETRY_SHADER)");
        break;
#if !defined (RADIANT_OSX)
      case Shader::TessControl:
        m_handle = glCreateShader(GL_TESS_CONTROL_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_TESS_CONTROL_SHADER)");
        break;
      case Shader::TessEval:
        m_handle = glCreateShader(GL_TESS_EVALUATION_SHADER);
        GLERROR("ShaderGL::compile # glCreateShader(GL_TESS_EVALUATION_SHADER)");
        break;
      case Shader::Compute:
        m_handle = glCreateShader(GL_COMPUTE_SHADER);
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
    glShaderSource(m_handle, 1, &text, &length);
    GLERROR("ShaderGL::compile # glShaderSource");
    glCompileShader(m_handle);
    GLERROR("ShaderGL::compile # glCompileShader");
    GLint compiled = GL_FALSE;
    glGetShaderiv(m_handle, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
      Radiant::error("Failed to compile shader %s", shader.filename().toUtf8().data());

      // Dump info log
      GLsizei len;
      glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &len);
      std::vector<GLchar> log(len);
      glGetShaderInfoLog(m_handle, len, &len, log.data());
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
    m_handle = glCreateProgram();
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
      glDeleteProgram(m_handle);
      GLERROR("ProgramGL::~ProgramGL # glDeleteProgram");
    }
  }

  void ProgramGL::bind()
  {
    touch();
    // Avoid re-applying the same shader
    if(m_state.setProgram(m_handle)) {
      glUseProgram(m_handle);
      GLERROR("ProgramGL::bind # glUseProgram");
#ifndef RADIANT_OSX_MOUNTAIN_LION
      // OpenGL 4.0 feature, so we use the ARB version
      if (isSampleShadingSupported()) {
        glMinSampleShadingARB(m_sampleShading);
        GLERROR("ProgramGL::bind # glMinSampleShadingARB");
      }
#endif
    }
  }

  void ProgramGL::bind(const Program & program)
  {
    link(program);
    bind();
  }

  void ProgramGL::link(const Program & program)
  {
    if(m_linked)
      return;

    /// @todo better error handling

    /// @todo we should have ShaderGL sharing through driver, for now they aren't shared
    m_shaders.clear();
    m_shaders.resize(program.shaderCount());

    for(std::size_t i = 0; i < program.shaderCount(); ++i) {
      Shader & shader = program.shader(i);
      ShaderGL & shadergl = m_shaders[i];
      shadergl.compile(shader);

      // Attach to the program
      glAttachShader(m_handle, shadergl.handle());
      GLERROR("ProgramGL::link # glAttachShader");
    }

    glLinkProgram(m_handle);
    GLERROR("ProgramGL::link # glLinkProgram");
    // Check for linking errors
    GLint status;
    glGetProgramiv(m_handle, GL_LINK_STATUS, &status);

    if(status == GL_FALSE) {
      Radiant::error("Failed to link shader program (shaders %s)", program.shaderFilenames().join(", ").toUtf8().data());
      GLsizei len;
      glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &len);
      std::vector<GLchar> log(len);
      glGetProgramInfoLog(m_handle, len, &len, log.data());
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
      glGetProgramiv(m_handle, GL_ACTIVE_ATTRIBUTES, &count);
      for (GLint i = 0; i < count; ++i) {
        glGetActiveAttrib(m_handle, i, sizeof(name), &length, &size, &type, name);
        m_attributes[name] = glGetAttribLocation(m_handle, name);
      }

      // Query all names of the uniform
      /// @todo store size/type data in ShaderVariable
      glGetProgramiv(m_handle, GL_ACTIVE_UNIFORMS, &count);
      for (GLint i = 0; i < count; ++i) {
        glGetActiveUniform(m_handle, i, sizeof(name), &length, &size, &type, name);
        m_uniforms[name] = glGetUniformLocation(m_handle, name);
      }

      // Query all names of the uniform blocks
      glGetProgramiv(m_handle, GL_ACTIVE_UNIFORM_BLOCKS, &count);
      for (GLint i = 0; i < count; ++i) {
        glGetActiveUniformBlockName(m_handle, i, sizeof(name), &length, name);
        m_uniformBlocks[name] = glGetUniformBlockIndex(m_handle, name);
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
      int loc = glGetUniformLocation(m_handle, name.data());
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
