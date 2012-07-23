#include "Luminous/ShaderProgram.hpp"
#include "Luminous/ShaderUniform.hpp"
#include "Luminous/RenderManager.hpp"
#include "Luminous/VertexDescription.hpp"

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
#include <QCryptographicHash>
#include <QStringList>

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
    ShaderGLSL::Type type;
    QString text;
    RenderResource::Hash hash;
    QString filename;
  };

  //////////////////////////////////////////////////////////////////////////
  // ShaderGLSL
  ShaderGLSL::ShaderGLSL(Type type)
    : RenderResource(getResourceType(type))
    , m_d(new ShaderGLSL::D())
  {
  }

  ShaderGLSL::~ShaderGLSL()
  {
    delete m_d;
  }

  void ShaderGLSL::setText(const QString & text)
  {
    m_d->text = text;
    m_d->filename = QString();
    QCryptographicHash hasher(QCryptographicHash::Md5);
    hasher.addData(text.toUtf8());
    memcpy(&m_d->hash, hasher.result().data(), sizeof(m_d->hash));
    invalidate();
  }

  void ShaderGLSL::loadText(const QString & filename)
  {
    QFile shaderFile(filename);
    if (!shaderFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      Radiant::warning("ShaderGLSL: Unable to open shader file %s", filename.toAscii().data());
      return;
    }
    setText(shaderFile.readAll());
    m_d->filename = filename;
  }

  const QString & ShaderGLSL::text() const
  {
    return m_d->text;
  }

  const QString & ShaderGLSL::filename() const
  {
    return m_d->filename;
  }

  ShaderGLSL::Type ShaderGLSL::type() const
  {
    return m_d->type;
  }

  RenderResource::Hash ShaderGLSL::hash() const
  {
    return m_d->hash;
  }

  //////////////////////////////////////////////////////////////////////////
  // ShaderProgram
  class ShaderProgram::D {
  public:
    D() : hashGeneration(0) {}

  public:
    // (resource, generation)
    // typedef std::vector<std::pair<RenderResource::Id, uint64_t>> ShaderList;
    typedef std::vector< RenderResource::Id > ShaderList;
    typedef std::vector< std::shared_ptr<ShaderUniform> > UniformList;
    ShaderList shaders;
    UniformList uniforms;
    VertexDescription vertexDescription;
    UniformDescription uniformDescription;
    uint64_t hashGeneration;
    RenderResource::Hash hash;
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
    m_d->shaders.push_back(shader.resourceId());
    invalidate();
  }

  void ShaderProgram::removeShader(const ShaderGLSL & shader)
  {
    auto it = std::remove(m_d->shaders.begin(), m_d->shaders.end(), shader.resourceId());
    m_d->shaders.erase(it, m_d->shaders.end());
    invalidate();
  }

  QStringList ShaderProgram::shaderFilenames() const
  {
    QStringList shaders;
    for(auto it = m_d->shaders.begin(); it != m_d->shaders.end(); ++it) {
      ShaderGLSL * shader = RenderManager::getResource<ShaderGLSL>(*it);
      if(shader)
        shaders << shader->filename();
    }
    return shaders;
  }

  size_t ShaderProgram::shaderCount() const
  {
    return m_d->shaders.size();
  }

  RenderResource::Hash ShaderProgram::hash() const
  {
    bool rehash = m_d->hashGeneration != generation();
    if(!rehash) {
      /// @todo iterate ShaderList and check generations.. or something similar
    }
    if(rehash) {
      QCryptographicHash hasher(QCryptographicHash::Md5);
      for(auto it = m_d->shaders.begin(); it != m_d->shaders.end(); ++it) {
        ShaderGLSL * shader = RenderManager::getResource<ShaderGLSL>(*it);
        assert(shader);
        const Hash shaderHash = shader->hash();
        hasher.addData((const char*)&shaderHash, sizeof(shaderHash));
      }
      /// @todo hash vertex/uniform descriptions
      memcpy(&m_d->hash, hasher.result().data(), sizeof(m_d->hash));
      m_d->hashGeneration = generation();
    }
    return m_d->hash;
  }

  RenderResource::Id ShaderProgram::shader(size_t index) const
  {
    assert(index < shaderCount());
    return m_d->shaders[index];
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
    return *m_d->uniforms[index];
  }

  const VertexDescription & ShaderProgram::vertexDescription() const
  {
    return m_d->vertexDescription;
  }

  void ShaderProgram::setVertexDescription(const VertexDescription & description)
  {
    m_d->vertexDescription = description;
    /// @todo invalidate?
  }

  const UniformDescription & ShaderProgram::uniformDescription() const
  {
    return m_d->uniformDescription;
  }

  void ShaderProgram::setUniformDescription(const UniformDescription & description)
  {
    m_d->uniformDescription = description;
    /// @todo invalidate?
  }
}
