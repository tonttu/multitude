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
  class LUMINOUS_API Shader : public ContextVariableT<GLSLProgramObject>,
  public Valuable::HasValues
  {
    MEMCHECKED_USING(Valuable::HasValues);
  public:
    Shader();
    /// Constructs a shader
    Shader(Valuable::HasValues * host, const char * name);
    /// Deletes the shader
    virtual ~Shader();

    /** Sets the source code for the fragment (aka pixel) shader.

        The actual shader compilation is delayed until one tries to access
        the program.

        It is safe to call this method without a valid OpenGL context.

        @param shadercode Shader source code
    */
    void setFragmentShader(const char * shadercode);
    /** Loads a fragment shader from a file.

        @param filename name of the file to load
        @return Returns true if the file was read successfully. Note that
        this does not necessarily mean that the shader works, as the shader is not
        compiled in this stage.
    */
    bool loadFragmentShader(const char * filename);
    /// @copydoc loadFragmentShader(const char * filename);
    bool loadFragmentShader(const QString & filename)
    { return loadFragmentShader(filename.toUtf8().data()); }

    /** Sets the source code for the vertex shader.
    @param shadercode Shader source code */
    void setVertexShader(const char * shadercode);
    /// Loads a vertex shader source code from a file.
    /// @param filename name of the file to load
    /// @return true of success
    bool loadVertexShader(const char * filename);
    /// @copydoc loadVertexShader(const char * filename);
    bool loadVertexShader(const QString & filename)
    { return loadVertexShader(filename.toUtf8().data()); }

    /** Sets the source code for the geometry shader.
    @param shadercode Shader source code */
    void setGeometryShader(const char * shadercode);
    /// Loads a geometry shader source code from a file.
    /// @param filename name of the source code filename
    /// @return true on success
    bool loadGeometryShader(const char * filename);
    /// @copydoc loadGeometryShader(const char * filename);
    bool loadGeometryShader(const QString & filename)
    { return loadGeometryShader(filename.toUtf8().data()); }

    /// Bind the shader
    /// Bind the shader Binds the shader to be active and compiles, links, and
    /// applies all defined uniforms if necessary. A valid OpenGL context is
    /// required. This function is thread-safe.
    /// @return the program object or null if it failed to link
    GLSLProgramObject * bind();

    /** Unbinds the shader. */
    void unbind();

    /// Get the OpenGL program object
    /// Returns a compiled program object that has not been linked.
    /// @param res resource container to associate the shader program with
    /// @return OpenGL program object
    GLSLProgramObject * program(Luminous::GLResources * res = 0);

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
    void addShaderUniform(const Valuable::ValueObject * vo);

    /// Check if the shader is defined Check if the shader source code has been
    /// defined. Note that the shader might not be compiled, as this function
    /// only tests for the presence of shader strings.
    /// @return Returns true if the shader code is defined.
    bool isDefined() const;

  private:
    class Params;
    class Self;
    Self * m_self;
  };

}

#endif // SHADER_HPP
