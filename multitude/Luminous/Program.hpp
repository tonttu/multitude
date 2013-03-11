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

#if !defined (LUMINOUS_PROGRAM_HPP)
#define LUMINOUS_PROGRAM_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"
#include "Luminous/UniformDescription.hpp"

//#include <Valuable/Node.hpp>
#include <memory>

#include <QString>

namespace Luminous
{
  /// A single shader (vertex, fragment, etc) written GLSL
  class ShaderGLSL
  {
  public:

    /// Type of the shader
    enum Type
    {
      Vertex,   /// Vertex shader
      Fragment, /// Fragment shader
      Geometry, /// Geometry shader
    };
  public:
    /// Constructor of shader
    /// @param type Type of the shader
    LUMINOUS_API ShaderGLSL(Type type);
    /// Destructor of shader
    LUMINOUS_API ~ShaderGLSL();

    /// Move constructor of shader
    /// @param s Shader to move
    LUMINOUS_API ShaderGLSL(ShaderGLSL && s);
    /// @todo
    /// @param s
    /// @return Reference to this
    LUMINOUS_API ShaderGLSL & operator=(ShaderGLSL && s);

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
    ShaderGLSL(const ShaderGLSL &);
    ShaderGLSL & operator=(const ShaderGLSL &);

    class D;
    D * m_d;
  };

  /// A shader program, combining multiple ShaderGLSL objects into one runnable program
  /// @todo implement copying
  class Program
    : public RenderResource
    , public Patterns::NotCopyable
    //, public Valuable::Node
  {
  public:
    /// Constructor for Program
    LUMINOUS_API Program();
    /// Destructor for Program
    LUMINOUS_API ~Program();

    /*LUMINOUS_API Program(Program & prog);
    LUMINOUS_API Program & operator=(Program & prog);*/

    /// Move constructor
    /// @param prog Program to move
    LUMINOUS_API Program(Program && prog);
    /// @todo
    /// @param prog
    /// @return Reference to this
    LUMINOUS_API Program & operator=(Program && prog);

    /// Adds shader for this program. The newly created shader is managed by this object.
    /// @param code Source code for the shader
    /// @param type Type of the shader
    /// @return Pointer to the shader
    LUMINOUS_API ShaderGLSL * addShader(const QByteArray & code, ShaderGLSL::Type type);

    /// Adds shader for this program. The newly created shader is managed by this object.
    /// @param filename File where shader source code is located
    /// @param type Type of the shader
    /// @return Pointer to the shader. nullptr if couldn't read
    LUMINOUS_API ShaderGLSL * loadShader(const QString & filename, ShaderGLSL::Type type);

    /// Reads fragment shader from the given file
    /// @param filename File where shader source code is located
    /// @return Pointer to the shader. nullptr if couldn't read
    ShaderGLSL * loadFragmentShader(const QString & filename)
    { return loadShader(filename, ShaderGLSL::Fragment); }

    /// Reads vertex shader from the given file
    /// @param filename File where shader source code is located
    /// @return Pointer to the shader. nullptr if couldn't read
    ShaderGLSL * loadVertexShader(const QString & filename)
    { return loadShader(filename, ShaderGLSL::Vertex); }

    /// Removes and destroys all shaders attached for this program
    LUMINOUS_API void removeAllShaders();
    /// Removes given shader. The comparison between shaders is done based on the memory
    /// adresses of shaders.
    /// @param shader Shader to remove.
    LUMINOUS_API void removeShader(const ShaderGLSL & shader);

    /// Returns the list of filenames where shaders are read.
    /// @return List of filenames
    LUMINOUS_API QStringList shaderFilenames() const;

    /// Returns shader associated with the program.
    /// @param index The ordering number of shader (first read gets 0, second 1 etc)
    /// @return Reference to the shader
    LUMINOUS_API ShaderGLSL & shader(size_t index) const;
    /// How many shaders are attached to this program
    /// @return Number of shaders
    LUMINOUS_API size_t shaderCount() const;

    /// Hash value for this program which is calculated based on the hashes of attached shaders
    /// @return Hash value for this program
    LUMINOUS_API Hash hash() const;

    /// Returns vertex format used in program
    /// @return Vertex description used in program
    LUMINOUS_API const VertexDescription & vertexDescription() const;
    /// Sets format for vertices used in program
    /// @param description Description to use in vertex shaders
    LUMINOUS_API void setVertexDescription(const VertexDescription & description);

    /// Sample shading value for objects rendered with this shader, see glMinSampleShading
    /// This is not supported in OS X Mountain Lion.
    LUMINOUS_API float sampleShading() const;
    /// Sets sample shading. Used as a value for glMinSampleShading
    LUMINOUS_API void setSampleShading(float sample);

    /// @cond
    LUMINOUS_API const UniformDescription & uniformDescription() const;
    LUMINOUS_API void setUniformDescription(const UniformDescription & description);
    /// @endcond

    /// Does the program render translucent geometry. Is important for reordering of
    /// rendering commands.
    /// @return Is there possibility for translucent rendering
    LUMINOUS_API bool translucent() const;
    /// Sets translucency flag.
    /// @param translucency Is there possibility for translucent rendering
    LUMINOUS_API void setTranslucency(bool translucency);

  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_PROGRAM_HPP
