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

#ifndef LUMINOUS_GLSLPROGRAMOBJECT_HPP
#define LUMINOUS_GLSLPROGRAMOBJECT_HPP

#include <Nimble/Matrix4.hpp>

#include <Luminous/Luminous.hpp>
#include <Luminous/GLResource.hpp>
#include <Luminous/GLSLShaderObject.hpp>

#include <Patterns/NotCopyable.hpp>

#include <list>
#include <QString>
#include <vector>

namespace Luminous
{

  /// OpenGL shading language program object. This class provides a wrapper for
  /// OpenGL program object. If you want to use OpenGL shader, you will likely
  /// want to use Luminous::Shader instead which provides a higher level
  /// abstraction for shader. @sa Luminous::Shader
  class LUMINOUS_API GLSLProgramObject : public GLResource, public Patterns::NotCopyable
  {
  public:

    /// Constructs new program object and puts it in the given resources
    /// collection
    GLSLProgramObject(RenderContext * resources = 0);
    virtual ~GLSLProgramObject();

    /// Adds a shader object to the program to be linked
    void addObject(GLSLShaderObject* obj);

    /// Links the attached shader objects to create the program object
    virtual bool link();

    /// Removes all attached shader objects from the program
    virtual void clear();

    /// Returns a log of the linking process
    const char * linkerLog();

    /// Binds the program to make it active
    virtual void bind();
    /// Clears any bound programs
    virtual void unbind();

    /// Gets the location of the given uniform variable
    int getUniformLoc(const QString & name);
    /// Gets the location of the given uniform variable
    int getUniformLoc(const char * name);

    /// Gets the location of the given attribute variable
    int getAttribLoc(const QString & name);
    /// Gets the location of the given attribute variable
    int getAttribLoc(const char * name);

    /// Sets the value of the given uniform
    /// @param name name of the uniform to set
    /// @param value uniform value
    /// @return false if the location of the uniform can not be found, otherwise true
    bool setUniformInt(const char * name, int value);
    /// @copydoc setUniformInt
    bool setUniformFloat(const char * name, float value);
    /// @copydoc setUniformInt
    bool setUniformVector2(const char * name, Nimble::Vector2f value);
    /// @copydoc setUniformInt
    bool setUniformVector3(const char * name, Nimble::Vector3f value);
    /// @copydoc setUniformInt
    bool setUniformVector4(const char * name, Nimble::Vector4f value);
    /// @copydoc setUniformInt
    /// The matrix is automatically transposed for OpenGL
    bool setUniformMatrix3(const char * name, const Nimble::Matrix3f & value);
    /// @copydoc setUniformInt
    /// The matrix is automatically transposed for OpenGL
    bool setUniformMatrix4(const char * name, const Nimble::Matrix4f & value);

#ifndef LUMINOUS_OPENGLES

    /// Sets a given program parameter
    /** This is in practice a call to glProgramParameteriEXT
    @param pname parameter to set
    @param value parameter value*/
    void setProgramParameter(GLenum pname, GLint value);
#endif // LUMINOUS_OPENGLES

    /// Validates the program
    /// @return true if the program is valid and can be used
    bool validate();

    /** Create a GLSLProgramObject from vertex- and fragment shader files.

    @param vsFile File containing the vertex shader source
    code. If vsFile is NULL, then this argument is ignored.

    @param fsFile File containing the fragment shader source
    code. If fsFile is NULL, then this argument is ignored.

    @return This functions returns a compiled GLSL program
    object. If the program could not be compiled, this function
    returns NULL.
     */
    static GLSLProgramObject* fromFiles
    (const char* vsFile, const char* fsFile);

    /** Create a GLSLProgramObject from vertex- and fragment shader strings.

    @param vsString String containing the vertex shader source
    code. If vsString is NULL, then this argument is ignored.

    @param fsString String containing the fragment shader source
    code. If fsString is NULL, then this argument is ignored.

    @return This functions returns a linked GLSL program
    object. If the program could not be linked, this function
    returns NULL.
     */
    static GLSLProgramObject* fromStrings
    (const char* vsString, const char* fsString);

    /// Loads vertex shader and fragment shader from the given strings,
    /// compiles and links them to this program object. This function requires
    /// a valid OpenGL context.
    /// @param vsString vertex shader code
    /// @param fsString fragment shader code
    /// @return true if the loading succeeded
    bool loadStrings(const char* vsString, const char* fsString);

    /// Returns the number of shader objects attached to the program
    int shaderObjectCount() const { return (int) m_shaderObjects.size(); }

    /** Loads a #Luminous::GLSLShaderObject from a file, compiles it, and adds
    it to this program object. This function requires a valid OpenGL context.
    @param shaderType type of the shader
    @param filename shader source code filename
    @return true if the loading succeeded*/
    bool loadFile(GLenum shaderType, const char * filename);

    /** Loads a #Luminous::GLSLShaderObject from a string, compiles it, and
    adds it to this program object. This function requires a valid OpenGL
    context.
    @param shaderType type of the shader
    @param shaderCode source code for the shader
    @return true if the loading succeed*/
    bool loadString(GLenum shaderType, const char * shaderCode);

    /// Returns the OpenGL handle for this program
    /// @return OpenGL handle to the program object
    GLuint handle() const { return m_handle; }

    /// Returns true if the program has been linked successfully
    /// @return true if the program has
    bool isLinked() const { return m_isLinked; }

    /// Sets the internal error flag
    /// @param errors value of error flag
    void setErrors(bool errors) { m_errors = errors; }
    /// Checks if the shader program has errors
    /// @return true if the program has errors
    bool hasErrors() const { return m_errors; }

  protected:
  private:

    friend class RenderContext;
    /// The linker log
    std::vector<GLchar> m_linkerLog;
    /// True if the program has been linked
    bool m_isLinked;
    /// True if there have been errors in the program
    bool m_errors;
    /// Lis of shader objects making up this program
    std::list<GLSLShaderObject*> m_shaderObjects;
    /// The OpenGL handle for the program
    GLuint m_handle;
  };

}

#endif

