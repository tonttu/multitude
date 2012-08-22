#ifndef LUMINOUS_PROGRAMGL_HPP
#define LUMINOUS_PROGRAMGL_HPP

#include "ResourceHandleGL.hpp"
#include "Luminous/VertexDescription.hpp"

#include <vector>
#include <map>
#include <vector>

#include <QByteArray>

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
    // With new enough compiler (GCC 4.7) using just private copy
    // ctor/assignment don't work as expected, you need to use = delete,
    // but then again, that doesn't work in msvc2010.
    /*ShaderGL(const ShaderGL &) = delete;
    ShaderGL & operator=(const ShaderGL &) = delete;*/
  };

  class ProgramGL : public ResourceHandleGL
  {
  public:
    ProgramGL(StateGL & state, const Program & program);
    ~ProgramGL();

    ProgramGL(ProgramGL && program);
    ProgramGL & operator=(ProgramGL && program);

    void bind();
    void bind(const Program & program);
    void link(const Program & program);

    int attributeLocation(const QByteArray & name);
    int uniformLocation(const QByteArray & name);
    int uniformBlockLocation(const QByteArray & blockname);

    const VertexDescription & vertexDescription() const { return m_vertexDescription; }

  private:
    std::vector<ShaderGL> m_shaders;
    std::map<QByteArray, int> m_attributes;
    std::map<QByteArray, int> m_uniforms;
    std::map<QByteArray, int> m_uniformBlocks;
    VertexDescription m_vertexDescription;
    bool m_linked;
    // UniformDescription m_baseDescription
  };
} // namespace Luminous

#endif // LUMINOUS_PROGRAMGL_HPP
