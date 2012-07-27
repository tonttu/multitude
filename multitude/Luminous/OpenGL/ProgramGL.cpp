#include "ProgramGL.hpp"

#include "../Program.hpp"

#include <QByteArray>
#include <QStringList>

/// @todo this file contains some (almost blindly) copy-pasted code from RenderDriverGL,
///       need to go through everything

namespace Luminous
{
  ShaderGL::ShaderGL()
    : m_handle(0)
  {
  }

  ShaderGL::~ShaderGL()
  {
    if(!m_handle)
      glDeleteShader(m_handle);
  }

  ShaderGL::ShaderGL(ShaderGL && shader)
    : m_handle(shader.m_handle)
  {
    shader.m_handle = 0;
  }

  ShaderGL & ShaderGL::operator=(ShaderGL && shader)
  {
    std::swap(m_handle, shader.m_handle);
    return *this;
  }

  bool ShaderGL::compile(const ShaderGLSL & shader)
  {
    if(!m_handle) {
      if(shader.type() == ShaderGLSL::Vertex) {
        m_handle = glCreateShader(GL_VERTEX_SHADER);
      } else if(shader.type() == ShaderGLSL::Fragment) {
        m_handle = glCreateShader(GL_FRAGMENT_SHADER);
      } else if(shader.type() == ShaderGLSL::Geometry) {
        m_handle = glCreateShader(GL_GEOMETRY_SHADER);
      } else {
        Radiant::error("Unknown shader type");
        return false;
      }
    }

    // Set and compile source
    const QByteArray shaderData = shader.text();
    const GLchar * text = shaderData.data();
    const GLint length = shaderData.size();
    glShaderSource(m_handle, 1, &text, &length);
    glCompileShader(m_handle);
    //GLERROR("RenderDriverGL::setShaderProgram glCompileShader");
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

  ProgramGL::ProgramGL(StateGL & state)
    : ResourceHandleGL(state)
    , m_linked(false)
  {
    m_handle = glCreateProgram();
  }
  /*
  ProgramGL::ProgramGL( ProgramGL && prog )
    : ResourceHandleGL(std::move(prog))
    , m_shaders(prog.m_shaders)
    , m_samplers(prog.m_samplers)
    , m_linked(prog.m_linked)
  {
    
  }


  ProgramGL & ProgramGL::operator=( ProgramGL && prog )
  {
    //m_shaders = prog.m_shaders;
    return *this;
  }
  */

  ProgramGL::ProgramGL(ProgramGL && program)
    : ResourceHandleGL(std::move(program))
    , m_shaders(std::move(program.m_shaders))
    , m_samplers(std::move(program.m_samplers))
    , m_linked(program.m_linked)
  {
  }

  ProgramGL & ProgramGL::operator=(ProgramGL && program)
  {
    ResourceHandleGL::operator=(std::move(program));
    std::swap(m_shaders, program.m_shaders);
    std::swap(m_samplers, program.m_samplers);
    std::swap(m_linked, program.m_linked);
    return *this;
  }

  ProgramGL::~ProgramGL()
  {
    if(m_handle)
      glDeleteProgram(m_handle);
  }

  void ProgramGL::bind()
  {
    touch();
    // Avoid re-applying the same shader
    if(m_state.setProgram(m_handle)) {
      glUseProgram(m_handle);
      //GLERROR("RenderDriverGL::setShaderProgram glUseProgram");
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

    for(std::size_t i = 0; i < program.shaderCount(); ++i) {
      ShaderGLSL & shader = program.shader(i);
      m_shaders.push_back(std::move(ShaderGL()));
      ShaderGL & shadergl = m_shaders.back();
      shadergl.compile(shader);

      // Attach to the program
      glAttachShader(m_handle, shadergl.handle());
      //GLERROR("RenderDriverGL::setShaderProgram glAttachShader");
    }

    glLinkProgram(m_handle);
    //GLERROR("RenderDriverGL::setShaderProgram glLinkProgram");
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
      // m_baseDescription = uniformDescription(programHandle, "BaseBlock");
    }
    m_linked = true;
  }

  int ProgramGL::samplerLocation(const QByteArray & name)
  {
    auto it = m_samplers.find(name);
    if(it == m_samplers.end()) {
      GLint loc = glGetUniformLocation(m_handle, name.data());
      m_samplers[name] = loc;
      return loc;
    } else {
      return it->second;
    }
  }

#if 0
  void applyShaderUniforms(const Program & program)
  {
    auto & programHandle = m_programs[program.hash()];

    // Set all shader uniforms attached to this shader
    for (size_t i = 0; i < program.uniformCount(); ++i) {
      ShaderUniform & uniform = program.uniform(i);

      // Update location cache if necessary
      if (uniform.index == -1)
        uniform.index = glGetUniformLocation(programHandle.handle, uniform.name.toAscii().data());

      // Set the uniform
      switch (uniform.type())
      {
      case ShaderUniform::Int: glUniform1iv(uniform.index, 1, (const int*)uniform.data()); break;
      case ShaderUniform::Int2: glUniform2iv(uniform.index, 1, (const int*)uniform.data()); break;
      case ShaderUniform::Int3: glUniform3iv(uniform.index, 1, (const int*)uniform.data()); break;
      case ShaderUniform::Int4: glUniform4iv(uniform.index, 1, (const int*)uniform.data()); break;
      case ShaderUniform::UnsignedInt: glUniform1uiv(uniform.index, 1, (const unsigned int*)uniform.data()); break;
      case ShaderUniform::UnsignedInt2: glUniform2uiv(uniform.index, 1, (const unsigned int*)uniform.data()); break;
      case ShaderUniform::UnsignedInt3: glUniform3uiv(uniform.index, 1, (const unsigned int*)uniform.data()); break;
      case ShaderUniform::UnsignedInt4: glUniform4uiv(uniform.index, 1, (const unsigned int*)uniform.data()); break;
      case ShaderUniform::Float: glUniform1fv(uniform.index, 1, (const float*)uniform.data()); break;
      case ShaderUniform::Float2: glUniform2fv(uniform.index, 1, (const float*)uniform.data()); break;
      case ShaderUniform::Float3: glUniform3fv(uniform.index, 1, (const float*)uniform.data()); break;
      case ShaderUniform::Float4: glUniform4fv(uniform.index, 1, (const float*)uniform.data()); break;
      case ShaderUniform::Float2x2: glUniformMatrix2fv(uniform.index, 1, GL_TRUE, (const float*)uniform.data()); break;
      case ShaderUniform::Float3x3: glUniformMatrix3fv(uniform.index, 1, GL_TRUE, (const float*)uniform.data()); break;
      case ShaderUniform::Float4x4: glUniformMatrix4fv(uniform.index, 1, GL_TRUE, (const float*)uniform.data()); break;
      default:
        Radiant::error("RenderDriverGL: Unknown shader uniform type %d", uniform.type());
        assert(false);
      }
    }
  }

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
