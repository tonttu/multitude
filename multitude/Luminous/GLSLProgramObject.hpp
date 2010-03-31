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
#include <string>
#include <vector>

namespace Luminous
{

  /// OpenGL shading language program object
  class LUMINOUS_API GLSLProgramObject : public GLResource, public Patterns::NotCopyable
  {
  public:
    GLSLProgramObject(GLResources * resources = 0);
    virtual ~GLSLProgramObject();

    void addObject(GLSLShaderObject* obj);

    virtual bool link();
    virtual void clear();

    const char * linkerLog();

    virtual void bind();
    virtual void unbind();

    int getUniformLoc(const std::string & name);
    int getUniformLoc(const char * name);

    int getAttribLoc(const std::string & name);
    int getAttribLoc(const char * name);

    bool setUniformInt(const char * name, int value);
    bool setUniformFloat(const char * name, float value);
    bool setUniformVector2(const char * name, Nimble::Vector2f value);

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

    @return This functions returns a compiled GLSL program
    object. If the program could not be compiled, this function
    returns NULL.
     */
    static GLSLProgramObject* fromStrings
    (const char* vsString, const char* fsString);

    bool loadStrings(const char* vsString, const char* fsString);

    int shaderObjectCount() const { return (int) m_shaderObjects.size(); } 

    GLuint handle() const { return m_handle; }

    bool isLinked() const { return m_isLinked; }

  protected:

    std::vector<GLchar> m_linkerLog;
    bool m_isLinked;
    std::list<GLSLShaderObject*> m_shaderObjects;
    GLuint m_handle;

  };

}

#endif

