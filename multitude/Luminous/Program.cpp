/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Program.hpp"
#include "VertexDescription.hpp"

#include <QCryptographicHash>
#include <QStringList>

#include <cassert>

namespace Luminous
{
  class Program::D
  {
  public:
    D() : m_sampleShading(0.0f), m_translucent(false), m_needRehash(false) {}

  public:
    typedef std::vector<std::unique_ptr<Shader>> ShaderList;
    ShaderList m_shaders;
    VertexDescription m_vertexDescription;
    UniformDescription m_uniformDescription;
    RenderResource::Hash m_hash;
    float m_sampleShading;
    bool m_translucent;
    bool m_needRehash;
  };

  Program::Program()
    : RenderResource(RenderResource::Program)
    , m_d(new Program::D())
  {
  }

  Program & Program::operator=( Program && prog )
  {
    RenderResource::operator=(std::move(prog));
    std::swap(m_d, prog.m_d);
    return *this;
  }

  Program::Program( Program && prog )
    : RenderResource(std::move(prog))
    , m_d(std::move(prog.m_d))
  {
  }

  Program::~Program()
  {
  }

  Shader * Program::addShader(const QByteArray & code, Shader::Type type)
  {
    auto shader = new Shader(type);
    m_d->m_shaders.emplace_back(shader);
    shader->setText(code);
    m_d->m_needRehash = true;
    return shader;
  }

  Shader * Program::loadShader(const QString & filename, Shader::Type type)
  {
    auto * shader = new Shader(type);
    if(shader->loadText(filename)) {
      m_d->m_needRehash = true;
      m_d->m_shaders.emplace_back(shader);
      return shader;
    }
    else {
      delete shader;
      return 0;
    }
  }

  void Program::removeAllShaders()
  {
    m_d->m_shaders.clear();
    m_d->m_needRehash = true;
  }

  void Program::removeShader(const Shader & shader)
  {
    auto it = std::remove_if(m_d->m_shaders.begin(), m_d->m_shaders.end(), [&](const std::unique_ptr<Shader> & s) {
      return s.get() == &shader; });
    m_d->m_shaders.erase(it, m_d->m_shaders.end());
    m_d->m_needRehash = true;
  }

  QStringList Program::shaderFilenames() const
  {
    QStringList shaders;
    for(auto it = m_d->m_shaders.begin(); it != m_d->m_shaders.end(); ++it) {
      shaders << (*it)->filename();
    }
    return shaders;
  }

  size_t Program::shaderCount() const
  {
    return m_d->m_shaders.size();
  }

  RenderResource::Hash Program::hash() const
  {
    if(!m_d->m_needRehash) {
      /// @todo iterate ShaderList and check generations.. or something similar
    }
    if(m_d->m_needRehash) {
      QCryptographicHash hasher(QCryptographicHash::Md5);
      for(auto it = m_d->m_shaders.begin(); it != m_d->m_shaders.end(); ++it) {
        const Hash shaderHash = (*it)->hash();
        hasher.addData((const char*)&shaderHash, sizeof(shaderHash));
      }
      /// @todo hash vertex/uniform descriptions
      memcpy(&m_d->m_hash, hasher.result().data(), sizeof(m_d->m_hash));
      m_d->m_needRehash = false;
    }
    return m_d->m_hash;
  }

  Shader & Program::shader(size_t index) const
  {
    assert(index < shaderCount());
    return *m_d->m_shaders[index];
  }

  const VertexDescription & Program::vertexDescription() const
  {
    return m_d->m_vertexDescription;
  }

  void Program::setVertexDescription(const VertexDescription & description)
  {
    m_d->m_vertexDescription = description;
    /// @todo invalidate?
  }

  float Program::sampleShading() const
  {
    return m_d->m_sampleShading;
  }

  void Program::setSampleShading(float sample)
  {
    m_d->m_sampleShading = sample;
    invalidate();
  }

  const UniformDescription & Program::uniformDescription() const
  {
    return m_d->m_uniformDescription;
  }

  void Program::setUniformDescription(const UniformDescription & description)
  {
    m_d->m_uniformDescription = description;
    /// @todo invalidate?
  }

  bool Program::translucent() const
  {
    return m_d->m_translucent;
  }

  void Program::setTranslucency(bool translucency)
  {
    m_d->m_translucent = translucency;
  }

}
