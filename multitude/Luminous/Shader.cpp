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

#include "Shader.hpp"
#include "Utils.hpp"

#include <Radiant/FileUtils.hpp>

#include <Valuable/ValueInt.hpp>
#include <Valuable/ValueFloat.hpp>
#include <Valuable/ValueVector.hpp>
#include <Valuable/ValueMatrix.hpp>

#include <typeinfo>
#include <vector>

namespace Luminous {

  using namespace Radiant;
  using namespace Valuable;

#define SHADER_PARAM_CHECK(obj, type, target) \
  { \
    const type * tmp = dynamic_cast<const type *> (obj); \
    if(tmp) {\
      target.push_back(Item(tmp)); \
      return; \
    } \
  }

#define SHADER_PARAM_APPLY1(objs, type, func, glslprog, glslfunc) \
  { \
    for(std::vector<Item>::iterator it = objs.begin(); it != objs.end(); it++) { \
      Item & v = (*it); \
      if(v.m_param == -2) \
        continue; \
      if(v.m_param == -1) {\
        int tmp = glslprog->glslfunc(((type*)v.m_obj)->name()); \
        if(tmp < 0) { \
          error("Could not find location for %s", ((type*)v.m_obj)->name().c_str()); \
          v.m_param = -2; \
          continue; \
        } \
        else { \
          v.m_param = tmp; \
        }\
      }\
      func(v.m_param, ((const type *) v.m_obj)->data()); \
    } \
  }

#define SHADER_PARAM_APPLYN(objs, type, func, glslprog, glslfunc) \
  { \
    for(std::vector<Item>::iterator it = objs.begin(); it != objs.end(); it++) { \
      Item & v = (*it); \
      if(v.m_param == -2) \
        continue; \
      if(v.m_param == -1) {\
        int tmp = glslprog->glslfunc(((type*)v.m_obj)->name()); \
        if(tmp < 0) { \
          error("Could not find location for %s", ((type*)v.m_obj)->name().c_str()); \
          v.m_param = -2; \
          continue; \
        } \
        else { \
          v.m_param = tmp; \
        }\
      }\
      func(v.m_param, 1, ((const type *) v.m_obj)->data()); \
    } \
  }

#define SHADER_PARAM_APPLYMATRIX(objs, type, func, glslprog, glslfunc) \
  { \
    for(std::vector<Item>::iterator it = objs.begin(); it != objs.end(); it++) { \
      Item & v = (*it); \
      if(v.m_param == -2) \
        continue; \
      if(v.m_param == -1) {\
        int tmp = glslprog->glslfunc(((type*)v.m_obj)->name()); \
        if(tmp < 0) { \
          error("Could not find location for %s", ((type*)v.m_obj)->name().c_str()); \
          v.m_param = -2; \
          continue; \
        } \
        else { \
          v.m_param = tmp; \
        }\
      }\
      func(v.m_param, 1, true, ((const type *) v.m_obj)->data()); \
    } \
  }

  class Shader::Params
  {
  public:

    class Item
    {
    public:
      Item(const void * obj) : m_param(-1), m_obj(obj) {}

      int m_param;
      const void * m_obj;
    };

    Params() {}

    void add(const Valuable::ValueObject * obj)
    {
      SHADER_PARAM_CHECK(obj, ValueInt,   m_i1);
      SHADER_PARAM_CHECK(obj, ValueFloat, m_f1);
      SHADER_PARAM_CHECK(obj, ValueVector2f, m_vec2f);
      SHADER_PARAM_CHECK(obj, ValueVector3f, m_vec3f);
      SHADER_PARAM_CHECK(obj, ValueVector4f, m_vec4f);
      SHADER_PARAM_CHECK(obj, ValueMatrix2f, m_mat2f);
      SHADER_PARAM_CHECK(obj, ValueMatrix3f, m_mat3f);
      SHADER_PARAM_CHECK(obj, ValueMatrix4f, m_mat4f);

      error("When adding shader parameter %s, type %s not supported",
            obj->name().c_str(), typeid(*obj).name());
    }


    void applyUniforms(GLSLProgramObject * glslprog)
    {
      SHADER_PARAM_APPLY1(m_i1, ValueInt, glUniform1i, glslprog, getUniformLoc);
      SHADER_PARAM_APPLY1(m_f1, ValueFloat, glUniform1f, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYN(m_vec2f, ValueVector2f, glUniform2fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYN(m_vec3f, ValueVector3f, glUniform3fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYN(m_vec4f, ValueVector4f, glUniform4fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYMATRIX(m_mat2f, ValueMatrix2f, glUniformMatrix2fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYMATRIX(m_mat3f, ValueMatrix3f, glUniformMatrix3fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYMATRIX(m_mat4f, ValueMatrix4f, glUniformMatrix4fv, glslprog, getUniformLoc);
    }

  private:

    std::vector<Item> m_i1;
    std::vector<Item> m_f1;
    std::vector<Item> m_vec2f;
    std::vector<Item> m_vec3f;
    std::vector<Item> m_vec4f;
    std::vector<Item> m_mat2f;
    std::vector<Item> m_mat3f;
    std::vector<Item> m_mat4f;
  };

  class Shader::Self
  {
  public:
    Self() : m_generation(0) {}

    Params m_uniforms;
    Params m_varyings;

    std::string m_fragmentShader;
    std::string m_vertexShader;
    std::string m_geometryShader;

    size_t m_generation;
  };


  Shader::Shader()
      : m_self(new Self)
  {}

  Shader::Shader(Valuable::HasValues * host, const char * name)
      : Valuable::HasValues(host, name, true),
      m_self(new Self)
  {}

  Shader::~Shader()
  {
    delete m_self;
  }

  void Shader::setFragmentShader(const char * shadercode)
  {
    m_self->m_fragmentShader = shadercode;
    m_self->m_generation++;
  }

  bool Shader::loadFragmentShader(const char * filename)
  {
    char * str = Radiant::FileUtils::loadTextFile(filename);

    if(!str)
      return false;

    setFragmentShader(str);
    delete [] str;

    return true;
  }

  void Shader::setVertexShader(const char * shadercode)
  {
    m_self->m_vertexShader = shadercode;
    m_self->m_generation++;
  }

  bool Shader::loadVertexShader(const char * filename)
  {
    char * str = Radiant::FileUtils::loadTextFile(filename);

    if(!str)
      return false;

    setVertexShader(str);
    delete [] str;

    return true;
  }

  void Shader::setGeometryShader(const char * shadercode)
  {
    m_self->m_geometryShader = shadercode;
    m_self->m_generation++;
  }

  bool Shader::loadGeometryShader(const char * filename)
  {
    char * str = Radiant::FileUtils::loadTextFile(filename);

    if(!str)
      return false;

    setGeometryShader(str);
    delete [] str;

    return true;
  }

  /*
  void Shader::addShaderAttribute(const Valuable::ValueObject * obj)
  {
    m_self->m_varyings.add(obj);
  }*/

  void Shader::addShaderUniform(const Valuable::ValueObject * obj)
  {
    m_self->m_uniforms.add(obj);
  }

  GLSLProgramObject * Shader::bind()
  {
    Luminous::Utils::glCheck("Shader::bind # Before entry");

    GLSLProgramObject * prog = program();

//    Radiant::info("prog %p", prog);
    if(!prog) {
      return 0;
    }

    if(!prog->isLinked()) {

      bool ok = prog->link();

      if(!ok) {
        error("Shader::program # Shader compilation failed: %s", prog->linkerLog());
        return 0;
      }
      else {
        // info("Compiled shader");
      }

    }

    Luminous::Utils::glCheck("Shader::bind # Before bind");

    prog->bind();

    /*
    info("Prog = %p handle = %d isLinked = %d valid = %d objs = %d",
         &prog, (int) prog.handle(), (int) prog.isLinked(),
         (int) prog.validate(), prog.shaderObjectCount());
         */
    m_self->m_uniforms.applyUniforms(prog);

    return prog;
  }

  void Shader::unbind()
  {
    glUseProgram(0);
  }

  GLSLProgramObject * Shader::program(Luminous::GLResources * res)
  {
    GLSLProgramObject & prog = ref(res);

    if(m_self->m_generation != prog.generation()) {

      bool ok = true;

      if(!m_self->m_vertexShader.empty())
        ok = ok && prog.loadString(GL_VERTEX_SHADER, m_self->m_vertexShader.c_str());

      if(!m_self->m_fragmentShader.empty())
        ok = ok && prog.loadString(GL_FRAGMENT_SHADER, m_self->m_fragmentShader.c_str());

      if(!m_self->m_geometryShader.empty())
        ok = ok && prog.loadString(GL_GEOMETRY_SHADER_EXT,
                                   m_self->m_geometryShader.c_str());

      /* Set the generation even if something has failed. */
      prog.setGeneration(m_self->m_generation);

      if(!ok || !prog.shaderObjectCount()) {
        prog.setErrors(true);
        return 0;
      } else prog.setErrors(false);

    } else if(prog.hasErrors()) {
      return 0;
    }

    return & prog;
  }

  bool Shader::isDefined() const
  {
    return !m_self->m_fragmentShader.empty() ||
        !m_self->m_vertexShader.empty();
  }
}
