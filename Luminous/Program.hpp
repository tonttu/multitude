/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (LUMINOUS_PROGRAM_HPP)
#define LUMINOUS_PROGRAM_HPP

#include "Shader.hpp"
#include "UniformDescription.hpp"

/*
#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

#include <memory>

#include <QString>*/

namespace Luminous
{
  /// A shader program, combining multiple Shader objects into one runnable program.
  /// GPU correspondent of this class is ProgramGL.
  /// @todo implement copying
  class Program : public RenderResource
  {
  public:
    /// Constructor for Program
    LUMINOUS_API Program();
    /// Destructor for Program
    LUMINOUS_API ~Program();

    /// Move constructor
    /// @param prog Program to move
    LUMINOUS_API Program(Program && prog);
    /// @param prog
    /// @return Reference to this
    LUMINOUS_API Program & operator=(Program && prog);

    /// Adds shader for this program. The newly created shader is managed by this object.
    /// @param code Source code for the shader
    /// @param type Type of the shader
    /// @return Pointer to the shader
    LUMINOUS_API Shader * addShader(const QByteArray & code, Shader::Type type);

    /// Adds shader for this program. The newly created shader is managed by this object.
    /// @param filename File where shader source code is located
    /// @param type Type of the shader
    /// @return Pointer to the shader. nullptr if couldn't read
    LUMINOUS_API Shader * loadShader(const QString & filename, Shader::Type type);

    /// Reads fragment shader from the given file
    /// @param filename File where shader source code is located
    /// @return Pointer to the shader. nullptr if couldn't read
    Shader * loadFragmentShader(const QString & filename)
    { return loadShader(filename, Shader::Fragment); }

    /// Reads vertex shader from the given file
    /// @param filename File where shader source code is located
    /// @return Pointer to the shader. nullptr if couldn't read
    Shader * loadVertexShader(const QString & filename)
    { return loadShader(filename, Shader::Vertex); }

    /// Removes and destroys all shaders attached for this program
    LUMINOUS_API void removeAllShaders();
    /// Removes given shader. The comparison between shaders is done based on the memory
    /// adresses of shaders.
    /// @param shader Shader to remove.
    LUMINOUS_API void removeShader(const Shader & shader);

    /// Returns the list of filenames where shaders are read.
    /// @return List of filenames
    LUMINOUS_API QStringList shaderFilenames() const;

    /// Returns shader associated with the program.
    /// @param index The ordering number of shader (first read gets 0, second 1 etc)
    /// @return Reference to the shader
    LUMINOUS_API Shader & shader(size_t index) const;
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
    /// rendering commands. Default is false.
    /// @return Is there possibility for translucent rendering
    LUMINOUS_API bool translucent() const;
    /// Sets translucency flag.
    /// @param translucency Is there possibility for translucent rendering
    LUMINOUS_API void setTranslucency(bool translucency);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
#endif // LUMINOUS_PROGRAM_HPP
