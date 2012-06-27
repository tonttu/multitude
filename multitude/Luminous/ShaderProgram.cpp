#include "Luminous/ShaderProgram.hpp"
#include "Luminous/ShaderUniform.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector3.hpp>
#include <Nimble/Vector4.hpp>
#include <Nimble/Matrix2.hpp>
#include <Nimble/Matrix3.hpp>
#include <Nimble/Matrix4.hpp>
#include <Radiant/Color.hpp>

#include <QFile>

#include <vector>
#include <cassert>
#include <algorithm>

namespace {
  Luminous::RenderResource::Type getResourceType(Luminous::ShaderGLSL::Type type)
  {
    /// @note This looks a bit dumb since this is a one-to-one-mapping. They are very different types though.
    switch (type) {
    case Luminous::ShaderGLSL::Vertex:   return Luminous::RenderResource::VertexShader;
    case Luminous::ShaderGLSL::Fragment: return Luminous::RenderResource::FragmentShader;
    case Luminous::ShaderGLSL::Geometry: return Luminous::RenderResource::GeometryShader;
    default:
      assert(false);
      Radiant::error("Can't determine resource type: Unknown shader type %d", type);
      return Luminous::RenderResource::VertexShader;
    }
  }
}

namespace Luminous
{
  class ShaderGLSL::D
  {
  public:
    struct Data
    {
      ShaderGLSL * owner;
      ShaderGLSL::Type type;
      QString text;
    };
    typedef std::shared_ptr<ShaderGLSL::D::Data> DataPtr;

  public:
    D(ShaderGLSL * owner, ShaderGLSL::Type type)
    {
      data = std::make_shared<Data>();
      data->owner = owner;
      data->type = type;
    }

    DataPtr data;
  };

  //////////////////////////////////////////////////////////////////////////
  // ShaderGLSL
  ShaderGLSL::ShaderGLSL(Type type)
    : RenderResource(getResourceType(type))
    , m_d(new ShaderGLSL::D(this, type))
  {
  }

  ShaderGLSL::~ShaderGLSL()
  {
    delete m_d;
  }

  void ShaderGLSL::setText(const QString & text)
  {
    m_d->data->text = text;
    invalidate();
  }

  void ShaderGLSL::loadText(const QString & filename)
  {
    QFile shaderFile(filename);
    if (!shaderFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      Radiant::warning("ShaderGLSL: Unable to open shader file %s", filename.toAscii().data());
      return;
    }
    m_d->data->text = shaderFile.readAll();
    invalidate();
  }

  const QString & ShaderGLSL::text() const
  {
    return m_d->data->text;
  }

  ShaderGLSL::Type ShaderGLSL::type() const
  {
    return m_d->data->type;
  }

  //////////////////////////////////////////////////////////////////////////
  // ShaderProgram
  class ShaderProgram::D {
  public:
    typedef std::vector< std::shared_ptr<ShaderGLSL::D::Data> > ShaderList;
    typedef std::vector<ShaderUniform> UniformList;
    ShaderList shaders;
    UniformList uniforms;
  };

  ShaderProgram::ShaderProgram()
    : RenderResource(RenderResource::ShaderProgram)
    , m_d(new ShaderProgram::D())
  {
  }

  ShaderProgram::~ShaderProgram()
  {
    delete m_d;
  }

  void ShaderProgram::addShader(const ShaderGLSL & shader)
  {
    auto ShaderGLSLPtr = shader.m_d->data;
    m_d->shaders.push_back(ShaderGLSLPtr);
    invalidate();
  }

  void ShaderProgram::removeShader(const ShaderGLSL & shader)
  {
    auto ShaderGLSLPtr = shader.m_d->data;

    D::ShaderList::iterator it = std::remove(m_d->shaders.begin(), m_d->shaders.end(), ShaderGLSLPtr);
    m_d->shaders.erase(it, m_d->shaders.end());
    invalidate();
  }

  size_t ShaderProgram::shaderCount() const
  {
    return m_d->shaders.size();
  }

  const ShaderGLSL & ShaderProgram::shader(size_t index) const
  {
    assert(index < shaderCount());
    return *((m_d->shaders[index])->owner);
  }

#define ADDSHADERUNIFORM(TYPE, DATATYPE) \
  template <> LUMINOUS_API void ShaderProgram::addShaderUniform(const QString & name, const TYPE & value) \
  { \
    ShaderUniform uniform; \
    uniform.name = name; \
    uniform.type = DATATYPE; \
    uniform.index = -1; \
    uniform.value.resize(sizeof(value)); \
    std::copy((const char *)&value, (const char *)&value + sizeof(value), uniform.value.begin()); \
    m_d->uniforms.push_back(uniform); \
  }
  
  ADDSHADERUNIFORM(int, ShaderUniform::Int);
  ADDSHADERUNIFORM(unsigned int, ShaderUniform::UnsignedInt);
  ADDSHADERUNIFORM(float, ShaderUniform::Float);
  ADDSHADERUNIFORM(Nimble::Vector2i, ShaderUniform::Int2);
  ADDSHADERUNIFORM(Nimble::Vector3i, ShaderUniform::Int3);
  ADDSHADERUNIFORM(Nimble::Vector4i, ShaderUniform::Int4);
  ADDSHADERUNIFORM(Nimble::Vector2f, ShaderUniform::Float2);
  ADDSHADERUNIFORM(Nimble::Vector3f, ShaderUniform::Float3);
  ADDSHADERUNIFORM(Nimble::Vector4f, ShaderUniform::Float4);
  ADDSHADERUNIFORM(Nimble::Matrix2f, ShaderUniform::Float2x2);
  ADDSHADERUNIFORM(Nimble::Matrix3f, ShaderUniform::Float3x3);
  ADDSHADERUNIFORM(Nimble::Matrix4f, ShaderUniform::Float4x4);

  // Manual conversion: Radiant::Color > Nimble::Vector4f
  template<> LUMINOUS_API void ShaderProgram::addShaderUniform(const QString & name, const Radiant::Color & value)
  {
    addShaderUniform<Nimble::Vector4f>(name, value);
  }
#undef ADDSHADERUNIFORM

  void ShaderProgram::removeShaderUniform(const QString & name)
  {
    m_d->uniforms.erase(
      std::remove_if(std::begin(m_d->uniforms), std::end(m_d->uniforms), [&](const ShaderUniform & u) { return u.name == name; }),
      std::end(m_d->uniforms));
  }

  size_t ShaderProgram::uniformCount() const
  {
    return m_d->uniforms.size();
  }

  ShaderUniform & ShaderProgram::uniform(size_t index) const
  {
    assert(index <m_d->uniforms.size());
    return m_d->uniforms[index];
  }
}
