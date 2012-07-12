/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitouch.fi
 *
 * All rights reserved, 2007-2010
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 */

#include "RenderManager.hpp"
#include "RenderDriver.hpp"

#include <map>

namespace Luminous
{
  class RenderManager::D {
  public:
    D()
      : resourceId(0)
    {
    }

    typedef std::map<RenderResource::Id, RenderResource *> ResourceMap;
    ResourceMap resourceMap;

    template <typename T>
    T * getResource( RenderResource::Id id )
    {
      auto descr = resourceMap.find(id);
      if (descr != std::end(resourceMap))
        return reinterpret_cast<T*>(descr->second);
      return nullptr;
    }

    RenderResource::Id resourceId;      // Next available resource ID
    std::vector<Luminous::RenderDriver*> drivers; // Currently used drivers
    static RenderManager * s_instance;  // Singleton instance
  };

  RenderManager * RenderManager::D::s_instance = 0;

  RenderManager::RenderManager()
    : m_d(new RenderManager::D())
  {
    assert(RenderManager::D::s_instance == 0);
    RenderManager::D::s_instance = this;
  }

  RenderManager::~RenderManager()
  {
    assert(RenderManager::D::s_instance != 0);
    delete m_d;

    RenderManager::D::s_instance = 0;
  }

  void RenderManager::setDrivers(std::vector<Luminous::RenderDriver*> drivers)
  {
    m_d->drivers = drivers;
  }

  RenderResource::Id RenderManager::createResource(RenderResource * resource)
  {
    auto id = instance().m_d->resourceId++;
    instance().m_d->resourceMap[id] = resource;
    return id;
  }

  void RenderManager::destroyResource(RenderResource::Id id)
  {
    instance().m_d->resourceMap.erase(id);
    auto it = instance().m_d->drivers.begin(), end = instance().m_d->drivers.end();
    while(it != end) {
      (*it)->releaseResource(id);
      ++it;
    }
  }

  RenderManager & RenderManager::instance()
  {
    assert(RenderManager::D::s_instance != 0);
    return *RenderManager::D::s_instance;
  }  

  // Only specialize for valid types
  template <> LUMINOUS_API HardwareBuffer * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<HardwareBuffer>(id); }
  template <> LUMINOUS_API VertexAttributeBinding * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<VertexAttributeBinding>(id); }
  template <> LUMINOUS_API VertexDescription * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<VertexDescription>(id); }
  template <> LUMINOUS_API Texture * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<Texture>(id); }
  template <> LUMINOUS_API ShaderProgram * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<ShaderProgram>(id); }
  template <> LUMINOUS_API ShaderGLSL * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<ShaderGLSL>(id); }
}
