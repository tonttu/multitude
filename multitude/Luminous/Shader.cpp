#include "Shader.hpp"
#include "Utils.hpp"

#include <Radiant/FileUtils.hpp>

#include <Valuable/ValueInt.hpp>
#include <Valuable/ValueFloat.hpp>
#include <Valuable/ValueVector.hpp>

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
      func(v.m_param, ((const type *) v.m_obj)->native()); \
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
      func(v.m_param, 1, ((const type *) v.m_obj)->native()); \
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
    }

  private:

    std::vector<Item> m_i1;
    std::vector<Item> m_f1;
    std::vector<Item> m_vec2f;
    std::vector<Item> m_vec3f;
    std::vector<Item> m_vec4f;
  };

  class Shader::Self
  {
  public:
    Self() : m_dirty(false) {}

    Params m_uniforms;
    Params m_varyings;

    std::string m_fragmentShader;
    std::string m_vertexShader;

    bool m_dirty;
  };


  Shader::Shader()
      : m_self(new Self)
  {}

  Shader::Shader(Valuable::HasValues * parent, const char * name)
      : Valuable::HasValues(parent, name, true),
      m_self(new Self)
  {}

  Shader::~Shader()
  {
    delete m_self;
  }

  void Shader::setFragmentShader(const char * shadercode)
  {
    m_self->m_fragmentShader = shadercode;
    m_self->m_dirty = true;
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
    m_self->m_dirty = true;
  }

  void Shader::addShaderVarying(const Valuable::ValueObject * obj)
  {
    m_self->m_varyings.add(obj);
  }

  void Shader::addShaderUniform(const Valuable::ValueObject * obj)
  {
    m_self->m_uniforms.add(obj);
  }

  GLSLProgramObject * Shader::bind()
  {
    Luminous::Utils::glCheck("Shader::bind # Before entry");

    GLSLProgramObject & prog = ref();

    if(m_self->m_dirty) {
      const char * vs = m_self->m_vertexShader.empty() ?
                        0 : m_self->m_vertexShader.c_str();
      const char * fs = m_self->m_fragmentShader.empty() ?
                  0 : m_self->m_fragmentShader.c_str();
      bool ok = prog.loadStrings(vs, fs);

      if(!ok) {
        error("Shader::program # Shader compilation failed: %s", prog.linkerLog());
      }
      else {
        // info("Compiled shader");
      }

      m_self->m_dirty = false;
    }

    Luminous::Utils::glCheck("Shader::bind # Before bind");

    prog.bind();

    /*
    info("Prog = %p handle = %d isLinked = %d valid = %d objs = %d",
         &prog, (int) prog.handle(), (int) prog.isLinked(),
         (int) prog.validate(), prog.shaderObjectCount());
         */
    m_self->m_uniforms.applyUniforms( & prog);

    return & prog;
  }

  bool Shader::isDefined() const
  {
    return !m_self->m_fragmentShader.empty() ||
        !m_self->m_vertexShader.empty();
  }
}
