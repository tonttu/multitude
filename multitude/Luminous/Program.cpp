#include "Luminous/Program.hpp"
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
#include <Radiant/ResourceLocator.hpp>

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

namespace
{
  struct ShaderCode
  {
    QByteArray text;
    Luminous::RenderResource::Hash hash;
  };

  static Radiant::Mutex s_shaderCacheMutex;
  static std::map<QString, std::weak_ptr<ShaderCode>> s_shaderCache;

  static std::shared_ptr<ShaderCode> loadFromText(const QByteArray & text)
  {
    std::shared_ptr<ShaderCode> code = std::make_shared<ShaderCode>();
    code->text = text;
    QCryptographicHash hasher(QCryptographicHash::Md5);
    hasher.addData(text);
    memcpy(&code->hash, hasher.result().data(), sizeof(code->hash));
    return code;
  }

  static std::shared_ptr<ShaderCode> loadFromFile(const QString & filename)
  {
    Radiant::Guard g(s_shaderCacheMutex);
    std::weak_ptr<ShaderCode> & weak = s_shaderCache[filename];
    std::shared_ptr<ShaderCode> ptr = weak.lock();
    if(ptr)
      return ptr;

    QFile shaderFile(filename);
    if(!shaderFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      Radiant::warning("ShaderGLSL: Unable to open shader file %s", filename.toAscii().data());
      return ptr;
    }
    ptr = loadFromText(shaderFile.readAll());
    weak = ptr;
    return ptr;
  }
}

namespace Luminous
{
  class ShaderGLSL::D
  {
  public:
    ShaderGLSL::Type type;
    QString filename;
    std::shared_ptr<ShaderCode> code;
  };

  //////////////////////////////////////////////////////////////////////////
  // ShaderGLSL
  ShaderGLSL::ShaderGLSL(Type type)
    : m_d(new ShaderGLSL::D())
  {
    m_d->type = type;
  }

  ShaderGLSL::~ShaderGLSL()
  {
    delete m_d;
  }

  ShaderGLSL::ShaderGLSL(ShaderGLSL && s)
    : m_d(s.m_d)
  {
    s.m_d = nullptr;
  }

  ShaderGLSL & ShaderGLSL::operator=(ShaderGLSL && s)
  {
    std::swap(m_d, s.m_d);
    return *this;
  }

  void ShaderGLSL::setText(const QByteArray & text)
  {
    m_d->code = loadFromText(text);
    m_d->filename = QString();
  }

  bool ShaderGLSL::loadText(const QString & filename)
  {
    m_d->code = loadFromFile(filename);
    m_d->filename = filename;

    return m_d->code && !m_d->code->text.isEmpty();
  }

  const QByteArray & ShaderGLSL::text() const
  {
    static QByteArray s_null;
    return m_d->code ? m_d->code->text : s_null;
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
    return m_d->code ? m_d->code->hash : RenderResource::Hash();
  }

  //////////////////////////////////////////////////////////////////////////
  // Program
  class Program::D {
  public:
    D() : translucent(false), needRehash(false) {}

  public:
    typedef std::vector<std::unique_ptr<ShaderGLSL>> ShaderList;
    typedef std::vector< std::shared_ptr<ShaderUniform> > UniformList;
    ShaderList shaders;
    UniformList uniforms;
    VertexDescription vertexDescription;
    UniformDescription uniformDescription;
    RenderResource::Hash hash;
    bool translucent;
    bool needRehash;
  };

  Program::Program()
    : RenderResource(RenderResource::Program)
    , m_d(new Program::D())
  {
  }

  Program & Program::operator=( Program && prog )
  {
    RenderResource::operator=(std::move(prog));
    //Valuable::Node::operator=(std::move(prog));
    std::swap(m_d, prog.m_d);
    return *this;
  }

  Program::Program( Program && prog )
    : RenderResource(std::move(prog))
    //, Valuable::Node(std::move(prog))
    , m_d(prog.m_d)
  {
    prog.m_d = nullptr;
  }

  Program::~Program()
  {
    delete m_d;
  }

  ShaderGLSL * Program::addShader(const QByteArray & code, ShaderGLSL::Type type)
  {
    auto shader = new ShaderGLSL(type);
    m_d->shaders.emplace_back(shader);
    shader->setText(code);
    m_d->needRehash = true;
    return shader;
  }

  ShaderGLSL * Program::loadShader(const QString & filename, ShaderGLSL::Type type)
  {
    auto * shader = new ShaderGLSL(type);
    if(shader->loadText(filename)) {
      m_d->needRehash = true;
      m_d->shaders.emplace_back(shader);
      return shader;
    }
    else {
      delete shader;
      return 0;
    }
  }

  void Program::removeShader(const ShaderGLSL & shader)
  {
    auto it = std::remove_if(m_d->shaders.begin(), m_d->shaders.end(), [&](const std::unique_ptr<ShaderGLSL> & s) {
      return s.get() == &shader; });
    m_d->shaders.erase(it, m_d->shaders.end());
    m_d->needRehash = true;
  }

  QStringList Program::shaderFilenames() const
  {
    QStringList shaders;
    for(auto it = m_d->shaders.begin(); it != m_d->shaders.end(); ++it) {
      shaders << (*it)->filename();
    }
    return shaders;
  }

  size_t Program::shaderCount() const
  {
    return m_d->shaders.size();
  }

  RenderResource::Hash Program::hash() const
  {
    if(!m_d->needRehash) {
      /// @todo iterate ShaderList and check generations.. or something similar
    }
    if(m_d->needRehash) {
      QCryptographicHash hasher(QCryptographicHash::Md5);
      for(auto it = m_d->shaders.begin(); it != m_d->shaders.end(); ++it) {
        const Hash shaderHash = (*it)->hash();
        hasher.addData((const char*)&shaderHash, sizeof(shaderHash));
      }
      /// @todo hash vertex/uniform descriptions
      memcpy(&m_d->hash, hasher.result().data(), sizeof(m_d->hash));
      m_d->needRehash = false;
    }
    return m_d->hash;
  }

  ShaderGLSL & Program::shader(size_t index) const
  {
    assert(index < shaderCount());
    return *m_d->shaders[index];
  }

#define ADDSHADERUNIFORMCONST(TYPE) \
  template <> LUMINOUS_API void Program::addShaderUniform(const QString & name, const TYPE & value) \
  { \
    std::shared_ptr<ShaderUniform> uniform(new ShaderUniformT<TYPE>(name, value)); \
    m_d->uniforms.push_back(uniform); \
  }

#if 0
#define ADDSHADERUNIFORMATTR(ATTRTYPE) \
  template <> LUMINOUS_API void Program::addShaderUniform(const QString & name, ATTRTYPE & value) \
  { \
    std::shared_ptr<ShaderUniform> uniform(new ShaderUniformT<ATTRTYPE>(name, value)); \
    m_d->uniforms.push_back(uniform); \
    /* Remove when the attribute is deleted */ \
    value.addListener(this, [=](){ this->removeShaderUniform(name); }, DELETE_ROLE); \
  }
#endif

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
//  ADDSHADERUNIFORMATTR(Valuable::AttributeInt);
//  ADDSHADERUNIFORMATTR(Valuable::AttributeFloat);

//  ADDSHADERUNIFORMATTR(Valuable::AttributeVector2i);
//  ADDSHADERUNIFORMATTR(Valuable::AttributeVector3i);
//  ADDSHADERUNIFORMATTR(Valuable::AttributeVector4i);
  
//  ADDSHADERUNIFORMATTR(Valuable::AttributeVector2f);
//  ADDSHADERUNIFORMATTR(Valuable::AttributeVector3f);
//  ADDSHADERUNIFORMATTR(Valuable::AttributeVector4f);
//  ADDSHADERUNIFORMATTR(Valuable::AttributeColor);

//  ADDSHADERUNIFORMATTR(Valuable::AttributeMatrix2f);
//  ADDSHADERUNIFORMATTR(Valuable::AttributeMatrix3f);
//  ADDSHADERUNIFORMATTR(Valuable::AttributeMatrix4f);

#undef ADDSHADERUNIFORMCONST
//#undef ADDSHADERUNIFORMATTR

  void Program::removeShaderUniform(const QString & name)
  {
    m_d->uniforms.erase(
      std::remove_if(std::begin(m_d->uniforms), std::end(m_d->uniforms), [&](const std::shared_ptr<ShaderUniform> & u) { return u->name == name; }),
      std::end(m_d->uniforms));
  }

  size_t Program::uniformCount() const
  {
    return m_d->uniforms.size();
  }

  ShaderUniform & Program::uniform(size_t index) const
  {
    assert(index <m_d->uniforms.size());
    return *m_d->uniforms[index];
  }

  const VertexDescription & Program::vertexDescription() const
  {
    return m_d->vertexDescription;
  }

  void Program::setVertexDescription(const VertexDescription & description)
  {
    m_d->vertexDescription = description;
    /// @todo invalidate?
  }

  const UniformDescription & Program::uniformDescription() const
  {
    return m_d->uniformDescription;
  }

  void Program::setUniformDescription(const UniformDescription & description)
  {
    m_d->uniformDescription = description;
    /// @todo invalidate?
  }

  bool Program::translucent() const
  {
    return m_d->translucent;
  }

  void Program::setTranslucency(bool translucency)
  {
    m_d->translucent = translucency;
  }

}
