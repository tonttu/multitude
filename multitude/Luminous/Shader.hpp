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

  /// Work in progress
  /** This is a utility class for mananing shaders. Objects of this type
      can be created in any thread, with or without a valid OpenGL context.
      Valid OpenGL context is only needed when one wants to use use the
      GLSLProgramObject that is bound to this Shader handler.

*/
  class Shader : public ContextVariableT<GLSLProgramObject>,
  public Valuable::HasValues
  {
  public:
    LUMINOUS_API Shader();
    LUMINOUS_API Shader(Valuable::HasValues * parent, const char * name);

    LUMINOUS_API virtual ~Shader();

    /** Sets the source code for the fragment (aka pixel) shader.

        The actual shader compilation is delayed until one tries to access
        the program.

        It is safe to call this method without a valid OpenGL context.
    */
    LUMINOUS_API void setFragmentShader(const char * shadercode);
    /** Loads a fragment shader from a file.

        @return Returns true if the file was read successfully. Note that
        this does not necessarily mean that the shader works, as the shader is not
        compiled in this stage.
    */
    LUMINOUS_API bool loadFragmentShader(const char * filename);
    /** Sets the source code for the vertex shader. */
    LUMINOUS_API void setVertexShader(const char * shadercode);
    /** Returns a compiled OpenGL shader program. */
    LUMINOUS_API GLSLProgramObject * bind();

    LUMINOUS_API void addShaderVarying(const Valuable::ValueObject *);
    LUMINOUS_API void addShaderUniform(const Valuable::ValueObject *);

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
