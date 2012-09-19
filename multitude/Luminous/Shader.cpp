/* COPYRIGHT
 */

#include "Shader.hpp"
#include "Utils.hpp"

#include <Radiant/FileUtils.hpp>

#include <Valuable/AttributeInt.hpp>
#include <Valuable/AttributeFloat.hpp>
#include <Valuable/AttributeVector.hpp>
#include <Valuable/AttributeMatrix.hpp>

#include <typeinfo>
#include <vector>

namespace Luminous {

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
          Radiant::error("Could not find location for %s", ((type*)v.m_obj)->name().data()); \
          v.m_param = -2; \
          continue; \
        } \
        else { \
          v.m_param = tmp; \
        }\
      }\
      func(v.m_param, *((const type *) v.m_obj)); \
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
          Radiant::error("Could not find location for %s", ((type*)v.m_obj)->name().data()); \
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
          Radiant::error("Could not find location for %s", ((type*)v.m_obj)->name().data()); \
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

    void add(const Valuable::Attribute * obj)
    {
      SHADER_PARAM_CHECK(obj, Valuable::AttributeInt,   m_i1);
      SHADER_PARAM_CHECK(obj, Valuable::AttributeFloat, m_f1);
      SHADER_PARAM_CHECK(obj, Valuable::AttributeVector2f, m_vec2f);
      SHADER_PARAM_CHECK(obj, Valuable::AttributeVector3f, m_vec3f);
      SHADER_PARAM_CHECK(obj, Valuable::AttributeVector4f, m_vec4f);
      SHADER_PARAM_CHECK(obj, Valuable::AttributeMatrix2f, m_mat2f);
      SHADER_PARAM_CHECK(obj, Valuable::AttributeMatrix3f, m_mat3f);
      SHADER_PARAM_CHECK(obj, Valuable::AttributeMatrix4f, m_mat4f);

      Radiant::error("When adding shader parameter %s, type %s not supported",
            obj->name().data(), typeid(*obj).name());
    }


    void applyUniforms(GLSLProgramObject * glslprog)
    {
      SHADER_PARAM_APPLY1(m_i1, Valuable::AttributeInt, glUniform1i, glslprog, getUniformLoc);
      SHADER_PARAM_APPLY1(m_f1, Valuable::AttributeFloat, glUniform1f, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYN(m_vec2f, Valuable::AttributeVector2f, glUniform2fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYN(m_vec3f, Valuable::AttributeVector3f, glUniform3fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYN(m_vec4f, Valuable::AttributeVector4f, glUniform4fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYMATRIX(m_mat2f, Valuable::AttributeMatrix2f, glUniformMatrix2fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYMATRIX(m_mat3f, Valuable::AttributeMatrix3f, glUniformMatrix3fv, glslprog, getUniformLoc);
      SHADER_PARAM_APPLYMATRIX(m_mat4f, Valuable::AttributeMatrix4f, glUniformMatrix4fv, glslprog, getUniformLoc);
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

    QString m_fragmentShader;
    QString m_vertexShader;
    QString m_geometryShader;
    QString m_label; // just for bug hunting

    size_t m_generation;
  };


  Shader::Shader()
      : m_self(new Self)
  {}

  Shader::Shader(Valuable::Node * host, const char * name)
      : Valuable::Node(host, name, true),
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
    if(m_self->m_label.isEmpty()) {
      m_self->m_label = filename;
    }

    const QByteArray str = Radiant::FileUtils::loadTextFile(filename);

    if(str.isNull())
      return false;

    setFragmentShader(str);

    return true;
  }

  void Shader::setVertexShader(const char * shadercode)
  {
    m_self->m_vertexShader = shadercode;
    m_self->m_generation++;
  }

  bool Shader::loadVertexShader(const char * filename)
  {
    if(m_self->m_label.isEmpty()) {
      m_self->m_label = filename;
    }

    const QByteArray str = Radiant::FileUtils::loadTextFile(filename);

    if(str.isNull())
      return false;

    setVertexShader(str.data());

    return true;
  }

  void Shader::setGeometryShader(const char * shadercode)
  {
    m_self->m_geometryShader = shadercode;
    m_self->m_generation++;
  }

  bool Shader::loadGeometryShader(const char * filename)
  {
    const QByteArray str = Radiant::FileUtils::loadTextFile(filename);

    if(str.isNull())
      return false;

    setGeometryShader(str.data());

    return true;
  }

  /*
  void Shader::addShaderAttribute(const Valuable::Attribute * obj)
  {
    m_self->m_varyings.add(obj);
  }*/

  void Shader::addShaderUniform(const Valuable::Attribute * obj)
  {
    m_self->m_uniforms.add(obj);
  }

  GLSLProgramObject * Shader::bind() const
  {
    //Luminous::Utils::glCheck("Shader::bind # Before entry");

    GLSLProgramObject * prog = program();

//    Radiant::info("prog %p", prog);
    if(!prog) {
      return 0;
    }

    if(!prog->isLinked()) {

      prog->setLabel(m_self->m_label);
      bool ok = prog->link();

      if(!ok) {
        Radiant::error("Shader::program # Shader compilation failed: %s", prog->linkerLog());
        return 0;
      }
      else {
        // Radiant::info("Compiled shader");
      }

    }

    //Luminous::Utils::glCheck("Shader::bind # Before bind");

    prog->bind();

    /*
    Radiant::info("Prog = %p handle = %d isLinked = %d valid = %d objs = %d",
         &prog, (int) prog.handle(), (int) prog.isLinked(),
         (int) prog.validate(), prog.shaderObjectCount());
         */
    m_self->m_uniforms.applyUniforms(prog);

    return prog;
  }

  void Shader::unbind() const
  {
    GLSLProgramObject * p = program();
    if(p)
      p->unbind();
  }

  GLSLProgramObject * Shader::program(Luminous::RenderContext * res) const
  {
    GLSLProgramObject & prog = ref(res);

    if(m_self->m_generation != prog.generation()) {

      bool ok = true;

      if(!m_self->m_vertexShader.isEmpty())
        ok = ok && prog.loadString(GL_VERTEX_SHADER, m_self->m_vertexShader.toUtf8().data());

      if(!m_self->m_fragmentShader.isEmpty())
        ok = ok && prog.loadString(GL_FRAGMENT_SHADER, m_self->m_fragmentShader.toUtf8().data());

#ifndef LUMINOUS_OPENGLES
      if(!m_self->m_geometryShader.isEmpty())
        ok = ok && prog.loadString(GL_GEOMETRY_SHADER,
                                   m_self->m_geometryShader.toUtf8().data());
#endif // LUMINOUS_OPENGLES

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
    return !m_self->m_fragmentShader.isEmpty() &&
        !m_self->m_vertexShader.isEmpty();
  }
}
