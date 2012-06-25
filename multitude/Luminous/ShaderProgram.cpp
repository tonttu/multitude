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
  class ShaderGLSL::D
  {
  public:
    struct Data
    {
      ShaderGLSL * owner;
      ShaderType type;
      QString text;
    };
    typedef std::shared_ptr<ShaderGLSL::D::Data> DataPtr;

  public:
    D(ShaderGLSL * owner, ShaderType type)
    {
      data = std::make_shared<Data>();
      data->owner = owner;
      data->type = type;
    }

    DataPtr data;
  };

  //////////////////////////////////////////////////////////////////////////
  // ShaderGLSL
  ShaderGLSL::ShaderGLSL(ShaderType type)
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

  ShaderType ShaderGLSL::type() const
  {
    return m_d->data->type;
  }

  //////////////////////////////////////////////////////////////////////////
  // ShaderProgram
  class ShaderProgram::D {
  public:
    typedef std::vector< std::shared_ptr<ShaderGLSL::D::Data> > ShaderList;
    ShaderList shaders;
  };

  ShaderProgram::ShaderProgram()
    : RenderResource(ResourceType_ShaderProgram)
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
}
