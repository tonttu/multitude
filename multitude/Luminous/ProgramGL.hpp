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
    LUMINOUS_API ShaderGL();
    LUMINOUS_API ~ShaderGL();

    LUMINOUS_API ShaderGL(ShaderGL && shader);
    LUMINOUS_API ShaderGL & operator=(ShaderGL && shader);

    LUMINOUS_API inline GLuint handle() const { return m_handle; }
    LUMINOUS_API bool compile(const ShaderGLSL & shader);

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
    LUMINOUS_API ProgramGL(StateGL & state, const Program & program);
    LUMINOUS_API ~ProgramGL();

    LUMINOUS_API ProgramGL(ProgramGL && program);
    LUMINOUS_API ProgramGL & operator=(ProgramGL && program);

    LUMINOUS_API void bind();
    LUMINOUS_API void bind(const Program & program);
    LUMINOUS_API void link(const Program & program);

    LUMINOUS_API int attributeLocation(const QByteArray & name);
    LUMINOUS_API int uniformLocation(const QByteArray & name);
    LUMINOUS_API int uniformBlockLocation(const QByteArray & blockname);

    LUMINOUS_API const VertexDescription & vertexDescription() const { return m_vertexDescription; }

  private:
    std::vector<ShaderGL> m_shaders;
    std::map<QByteArray, int> m_attributes;
    std::map<QByteArray, int> m_uniforms;
    std::map<QByteArray, int> m_uniformBlocks;
    VertexDescription m_vertexDescription;
    float m_sampleShading;
    bool m_linked;
    // UniformDescription m_baseDescription
  };
} // namespace Luminous

#endif // LUMINOUS_PROGRAMGL_HPP
