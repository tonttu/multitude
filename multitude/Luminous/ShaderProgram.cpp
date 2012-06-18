#include "Luminous/ShaderProgram.hpp"

#include <QFile>

#include <vector>
#include <cassert>
#include <algorithm>

namespace {
  Luminous::ResourceType getResourceType(Luminous::ShaderType type)
  {
    /// @note This looks a bit dumb since this is a one-to-one-mapping. They are very different types though.
    switch (type) {
    case Luminous::ShaderType_Vertex:   return Luminous::ResourceType_VertexShader;
    case Luminous::ShaderType_Fragment: return Luminous::ResourceType_FragmentShader;
    case Luminous::ShaderType_Geometry: return Luminous::ResourceType_GeometryShader;
    default:
      assert(false);
      Radiant::error("Can't determine resource type: Unknown shader type %d", type);
      return Luminous::ResourceType_VertexShader;
    }
  }
}

namespace Luminous
{
  //////////////////////////////////////////////////////////////////////////
  // ShaderGLSL
  class ShaderGLSL::D {
  public:
    D(ShaderType type)
      : type(type)
    {
    }

  public:
    ShaderType type;
    QString text;
  };


  ShaderGLSL::ShaderGLSL(RenderResource::Id id, ShaderType type, RenderDriver & driver)
    : RenderResource(id, getResourceType(type), driver)
    , m_d(new ShaderGLSL::D(type))
  {
  }

  ShaderGLSL::~ShaderGLSL()
  {
    delete m_d;
  }

  void ShaderGLSL::setText(const QString & text)
  {
    m_d->text = text;
    invalidate();
  }

  void ShaderGLSL::loadText(const QString & filename)
  {
    QFile shaderFile(filename);
    if (!shaderFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      Radiant::warning("ShaderGLSL: Unable to open shader file %s", filename.toAscii().data());
      return;
    }
    m_d->text = shaderFile.readAll();
    invalidate();
  }

  const QString & ShaderGLSL::text() const
  {
    return m_d->text;
  }

  ShaderType ShaderGLSL::type() const
  {
    return m_d->type;
  }

  //////////////////////////////////////////////////////////////////////////
  // ShaderProgram
  class ShaderProgram::D {
  public:
    typedef std::vector< std::shared_ptr<ShaderGLSL> > ShaderList;
    ShaderList shaders;
  };

  ShaderProgram::ShaderProgram(RenderResource::Id id, RenderDriver & driver)
    : RenderResource(id, ResourceType_ShaderProgram, driver)
    , m_d(new ShaderProgram::D())
  {

  }

  ShaderProgram::~ShaderProgram()
  {
    delete m_d;
  }

  void ShaderProgram::addShader(const std::shared_ptr<ShaderGLSL> & shader)
  {
    m_d->shaders.push_back(shader);
    invalidate();
  }

  void ShaderProgram::removeShader(const std::shared_ptr<ShaderGLSL> & shader)
  {
    D::ShaderList::iterator it = std::remove(m_d->shaders.begin(), m_d->shaders.end(), shader);
    m_d->shaders.erase(it, m_d->shaders.end());
    invalidate();
  }

  size_t ShaderProgram::shaderCount() const
  {
    return m_d->shaders.size();
  }

  const std::shared_ptr<ShaderGLSL> & ShaderProgram::shader(size_t index) const
  {
    assert(index < shaderCount());
    return m_d->shaders[index];
  }
}
