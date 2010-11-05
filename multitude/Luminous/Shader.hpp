/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef LUMINOUS_SHADER_HPP
#define LUMINOUS_SHADER_HPP

#include <Luminous/Luminous.hpp> // First for GLEW

#include <Luminous/Collectable.hpp>
#include <Luminous/ContextVariable.hpp>
#include <Luminous/GLSLProgramObject.hpp>
#include <Valuable/HasValues.hpp>


namespace Luminous {

  /** This is a utility class for managing shaders. Objects of this type
      can be created in any thread, with or without a valid OpenGL context.
      Valid OpenGL context is only needed when one wants to use use the
      GLSLProgramObject that is bound to this Shader handler.

      @see ShaderExample demo application
*/
  class Shader : public ContextVariableT<GLSLProgramObject>,
  public Valuable::HasValues
  {
  public:
    LUMINOUS_API Shader();
    /// Constructs a shader
    LUMINOUS_API Shader(Valuable::HasValues * parent, const char * name);
    /// Deletes the shader
    LUMINOUS_API virtual ~Shader();

    /** Sets the source code for the fragment (aka pixel) shader.

        The actual shader compilation is delayed until one tries to access
        the program.

        It is safe to call this method without a valid OpenGL context.

        @param shadercode Shader source code
    */
    LUMINOUS_API void setFragmentShader(const char * shadercode);
    /** Loads a fragment shader from a file.

        @param filename name of the file to load
        @return Returns true if the file was read successfully. Note that
        this does not necessarily mean that the shader works, as the shader is not
        compiled in this stage.
    */
    LUMINOUS_API bool loadFragmentShader(const char * filename);
    /// @copydoc loadFragmentShader(const char * filename);
    bool loadFragmentShader(const std::string & filename)
    { return loadFragmentShader(filename.c_str()); }

    /** Sets the source code for the vertex shader.
    @param shadercode Shader source code */
    LUMINOUS_API void setVertexShader(const char * shadercode);
    /** Loads a vertex shader source code from a file.
    @param filename name of the file to load */
    LUMINOUS_API bool loadVertexShader(const char * filename);
    /// @copydoc loadVertexShader(const char * filename);
    bool loadVertexShader(const std::string & filename)
    { return loadVertexShader(filename.c_str()); }

    /** Sets the source code for the geometry shader.
    @param shadercode Shader source code */
    LUMINOUS_API void setGeometryShader(const char * shadercode);
    /// Loads a geometry shader source code from a file.
    /// @param filename name of the source code filename
    LUMINOUS_API bool loadGeometryShader(const char * filename);
    /// @copydoc loadGeometryShader(const char * filename);
    bool loadGeometryShader(const std::string & filename)
    { return loadGeometryShader(filename.c_str()); }

    /** Returns a compiled and bound OpenGL shader program. */
    LUMINOUS_API GLSLProgramObject * bind();

    /** Unbinds the shader. */
    LUMINOUS_API void unbind();

    /** Returns a non-compiled OpenGL shader program.
    @param res resource container to associate the shader program with */
    LUMINOUS_API GLSLProgramObject * program(Luminous::GLResources * res = 0);

    // Adds a ValueObject as a shader attribute
    //LUMINOUS_API void addShaderAttribute(const Valuable::ValueObject *);
    /// Adds a ValueObject as a shader uniform
    /** Once the ValueObject has been added to this shader its value
        is automatically used with the shader, as the shader is bound.
        This feature can be used to automatically set the uniforms of the shader
        so that one does not need to set them via some parameter.

        The name of the object should match some of the uniforms of the shader.

        @param vo The object to be used
    */
    LUMINOUS_API void addShaderUniform(const Valuable::ValueObject * vo);

    /** Returns true if the shader code is defined.
        Note that the shader might not be compiled, as this function only
        tests for the presence of shader strings.
    */
    LUMINOUS_API bool isDefined() const;

  private:
    class Params;
    class Self;
    Self * m_self;
  };

}

#endif // SHADER_HPP
