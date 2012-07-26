#ifndef LUMINOUS_PROGRAMGL_HPP
#define LUMINOUS_PROGRAMGL_HPP

#include "OpenGL/ResourceHandleGL.hpp"

#include <map>

class QByteArray;

namespace Luminous
{
  /// @todo ShaderGLSL class has a bit confusing name, since actually this class
  ///       should be "ShaderGLSLGL"... ShaderGLSL should be renamed to Shader
  class ShaderGL
  {
  public:
    ShaderGL();
    ~ShaderGL();

    ShaderGL(ShaderGL && shader);
    ShaderGL & operator=(ShaderGL && shader);

    inline GLuint handle() const { return m_handle; }
    bool compile(const ShaderGLSL & shader);

  private:
    GLuint m_handle;

  private:
    ShaderGL(const ShaderGL &);
    ShaderGL & operator=(const ShaderGL &);
  };

  class ProgramGL : public ResourceHandleGL
  {
  public:
    ProgramGL(StateGL & state);
    ~ProgramGL();

    void bind();
    void bind(const Program & program);
    void link(const Program & program);

    int samplerLocation(const QByteArray & name);

  private:
    std::vector<ShaderGL> m_shaders;
    std::map<QByteArray, int> m_samplers;
    bool m_linked;
    // UniformDescription m_baseDescription
  };
} // namespace Luminous

#endif // LUMINOUS_PROGRAMGL_HPP
