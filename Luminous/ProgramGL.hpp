/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_PROGRAMGL_HPP
#define LUMINOUS_PROGRAMGL_HPP

#include "ResourceHandleGL.hpp"
#include "Luminous/VertexDescription.hpp"

#include <vector>
#include <map>
#include <vector>
#include <memory>

#include <QByteArray>

namespace Luminous
{

  /// This class represents the Shader object in GPU memory.
  class ShaderGL
  {
  public:
    /// Constructor
    LUMINOUS_API ShaderGL(OpenGLAPI& opengl);
    /// Destructor.
    /// Noexcept specifier is needed because it's not default in GCC 4.7.2
    LUMINOUS_API ~ShaderGL() noexcept;

    /// Move constructor
    /// @param shader shader to move
    LUMINOUS_API ShaderGL(ShaderGL && shader) noexcept;
    /// Move assignment operator
    /// @param shader shader to move
    LUMINOUS_API ShaderGL & operator=(ShaderGL && shader) noexcept;

    /// Get the raw OpenGL handle for the shader
    /// @return object handle
    LUMINOUS_API inline GLuint handle() const { return m_handle; }
    /// Compile the shader object. If there are errors, they are printed to stderr.
    /// @param shader shader object on CPU
    /// @return true if compilation was successful; otherwise false
    LUMINOUS_API bool compile(const Shader & shader);

  private:
    GLuint m_handle;
    OpenGLAPI & m_opengl;

    ShaderGL(const ShaderGL &);
    ShaderGL & operator=(const ShaderGL &);
  };

  /// This class represents the Program object in GPU memory
  class ProgramGL : public ResourceHandleGL
  {
  public:
    /// Constructor
    /// @param state OpenGL state
    /// @param program program object in CPU
    LUMINOUS_API ProgramGL(StateGL & state, const Program & program);
    /// Destructor
    LUMINOUS_API ~ProgramGL();

    /// Move constructor
    /// @param program program object to move
    LUMINOUS_API ProgramGL(ProgramGL && program);
    /// Move assignment operator
    /// @param program program object to move
    LUMINOUS_API ProgramGL & operator=(ProgramGL && program);

    /// Bind the program
    LUMINOUS_API void bind();
    /// Bind the program optinally linking it
    /// @param program program object on CPU
    LUMINOUS_API void bind(const Program & program);
    /// Link the program
    /// @param program program object on CPU
    LUMINOUS_API void link(const Program & program);

    /// Get the location of the given attribute
    /// @param name attribute name
    /// @return index of the attribute location or -1 if not found
    LUMINOUS_API int attributeLocation(const QByteArray & name);
    /// Get the location of the given uniform
    /// @param name uniform name to query
    /// @return index of the uniform location or -1 if not found
    LUMINOUS_API int uniformLocation(const QByteArray & name);
    /// Get the location of the given uniform block
    /// @param blockname uniform block name
    /// @return index of the location of the uniform block or -1 if not found
    LUMINOUS_API int uniformBlockLocation(const QByteArray & blockname);

    /// Get the vertex description associated with the program
    /// @return vertex description
    LUMINOUS_API const VertexDescription & vertexDescription() const { return m_vertexDescription; }

  private:
    std::vector<std::unique_ptr<ShaderGL> > m_shaders;
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
