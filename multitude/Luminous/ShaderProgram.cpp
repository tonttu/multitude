#include "Luminous/ShaderProgram.hpp"
#include "Luminous/ShaderUniform.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector3.hpp>
#include <Nimble/Vector4.hpp>
#include <Nimble/Matrix2.hpp>
#include <Nimble/Matrix3.hpp>
#include <Nimble/Matrix4.hpp>
#include <Radiant/Color.hpp>
#include <Valuable/AttributeFloat.hpp>
#include <Valuable/AttributeVector.hpp>
#include <Valuable/AttributeColor.hpp>
#include <Valuable/AttributeMatrix.hpp>

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
    typedef std::vector< std::shared_ptr<ShaderUniform> > UniformList;
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

#define ADDSHADERUNIFORMCONST(TYPE) \
  template <> LUMINOUS_API void ShaderProgram::addShaderUniform(const QString & name, const TYPE & value) \
  { \
    std::shared_ptr<ShaderUniform> uniform(new ShaderUniformT<TYPE>(name, value)); \
    m_d->uniforms.push_back(uniform); \
  }

#define ADDSHADERUNIFORMATTR(ATTRTYPE) \
  template <> LUMINOUS_API void ShaderProgram::addShaderUniform(const QString & name, ATTRTYPE & value) \
  { \
    std::shared_ptr<ShaderUniform> uniform(new ShaderUniformT<ATTRTYPE>(name, value)); \
    m_d->uniforms.push_back(uniform); \
    /* Remove when the attribute is deleted */ \
    value.addListener(this, [=](){ this->removeShaderUniform(name); }, DELETE_ROLE); \
  }

  ADDSHADERUNIFORMCONST(int);
  ADDSHADERUNIFORMCONST(unsigned int);
  ADDSHADERUNIFORMCONST(float);
  ADDSHADERUNIFORMCONST(Nimble::Vector2i);
  ADDSHADERUNIFORMCONST(Nimble::Vector3i);
  ADDSHADERUNIFORMCONST(Nimble::Vector4i);
  ADDSHADERUNIFORMCONST(Nimble::Vector2f);
  ADDSHADERUNIFORMCONST(Nimble::Vector3f);
  ADDSHADERUNIFORMCONST(Nimble::Vector4f);
  ADDSHADERUNIFORMCONST(Radiant::Color);
  ADDSHADERUNIFORMCONST(Nimble::Matrix2f);
  ADDSHADERUNIFORMCONST(Nimble::Matrix3f);
  ADDSHADERUNIFORMCONST(Nimble::Matrix4f);
  ADDSHADERUNIFORMATTR(Valuable::AttributeInt);
  ADDSHADERUNIFORMATTR(Valuable::AttributeFloat);

  ADDSHADERUNIFORMATTR(Valuable::AttributeVector2i);
  ADDSHADERUNIFORMATTR(Valuable::AttributeVector3i);
  ADDSHADERUNIFORMATTR(Valuable::AttributeVector4i);
  
  ADDSHADERUNIFORMATTR(Valuable::AttributeVector2f);
  ADDSHADERUNIFORMATTR(Valuable::AttributeVector3f);
  ADDSHADERUNIFORMATTR(Valuable::AttributeVector4f);
  ADDSHADERUNIFORMATTR(Valuable::AttributeColor);

  ADDSHADERUNIFORMATTR(Valuable::AttributeMatrix2f);
  ADDSHADERUNIFORMATTR(Valuable::AttributeMatrix3f);
  ADDSHADERUNIFORMATTR(Valuable::AttributeMatrix4f);

#undef ADDSHADERUNIFORMCONST
#undef ADDSHADERUNIFORMATTR

  void ShaderProgram::removeShaderUniform(const QString & name)
  {
    m_d->uniforms.erase(
      std::remove_if(std::begin(m_d->uniforms), std::end(m_d->uniforms), [&](const std::shared_ptr<ShaderUniform> & u) { return u->name == name; }),
      std::end(m_d->uniforms));
  }

  size_t ShaderProgram::uniformCount() const
  {
    return m_d->uniforms.size();
  }

  ShaderUniform & ShaderProgram::uniform(size_t index) const
  {
    assert(index <m_d->uniforms.size());
    return *(m_d->uniforms[index]);
  }
}
