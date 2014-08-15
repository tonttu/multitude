/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_SHADER_HPP
#define LUMINOUS_SHADER_HPP

#include "Luminous/RenderResource.hpp"

#include <memory>

namespace Luminous
{
  /// A single shader (vertex, fragment, etc) written GLSL.
  class Shader
  {
  public:

    /// Type of the shader
    enum Type
    {
      Vertex,       ///< Vertex shader
      Fragment,     ///< Fragment shader
      Geometry,     ///< Geometry shader
      TessControl,  ///< Tessellation control shader
      TessEval,     ///< Tessellation evaluation shader
      Compute,      ///< Compute shader
    };
  public:
    /// Constructor of shader
    /// @param type Type of the shader
    LUMINOUS_API Shader(Type type);
    /// Destructor of shader
    LUMINOUS_API ~Shader();

    /// Move constructor of shader
    /// @param s Shader to move
    LUMINOUS_API Shader(Shader && s);
    /// @todo
    /// @param s
    /// @return Reference to this
    LUMINOUS_API Shader & operator=(Shader && s);

    /// Reads shader from the given file
    /// @param filename File where shader is read
    /// @return Was the read successful
    LUMINOUS_API bool loadText(const QString & filename);

    /// Sets source code for the shader
    /// @param src Source code for the shader
    LUMINOUS_API void setText(const QByteArray & src);

    /// Returns the source code of the shader
    /// @return Source code of the shader
    LUMINOUS_API const QByteArray & text() const;

    /// Returns the source code file for the shader
    /// @return Filename of the source code. Empty if not read from file
    LUMINOUS_API const QString & filename() const;

    /// Returns the type of the shader
    /// @return Type of the shader
    LUMINOUS_API Type type() const;

    /// Hash for the shader which is calculated based on source code of shader
    /// @return Hash for shader
    LUMINOUS_API RenderResource::Hash hash() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
#endif // LUMINOUS_SHADER_HPP
