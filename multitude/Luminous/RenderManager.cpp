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
#include "ContextArray.hpp"

#include <Radiant/Timer.hpp>

#include <map>

namespace Luminous
{
  class RenderManager::D {
  public:
    D()
      : resourceId(0)
      , frameTime(0)
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
    Radiant::Mutex contextArraysMutex;
    Radiant::Timer timer;
    int frameTime;
    std::set<ContextArray*> contextArrays;
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
    Radiant::Guard g(m_d->contextArraysMutex);
    for(auto it = m_d->contextArrays.begin(), end = m_d->contextArrays.end(); it != end; ++it)
      (*it)->resize(drivers.size());
  }

  RenderResource::Id RenderManager::createResource(RenderResource * resource)
  {
    auto id = instance().m_d->resourceId++;
    instance().m_d->resourceMap[id] = resource;
    return id;
  }

  void RenderManager::updateResource(RenderResource::Id id, RenderResource * resource)
  {
    instance().m_d->resourceMap[id] = resource;
  }

  void RenderManager::destroyResource(RenderResource::Id id)
  {
    /// @todo Make this thread-safe
    /* Widgets can be destroyed in any thread, which can trigger this function call
       from any thread. */
    instance().m_d->resourceMap.erase(id);
    auto it = instance().m_d->drivers.begin(), end = instance().m_d->drivers.end();
    while(it != end) {
      (*it)->releaseResource(id);
      ++it;
    }
  }

  void RenderManager::addContextArray(ContextArray * contextArray)
  {
    auto & d = *instance().m_d;
    Radiant::Guard g(d.contextArraysMutex);
    d.contextArrays.insert(contextArray);
  }

  void RenderManager::removeContextArray(ContextArray * contextArray)
  {
    auto & d = *instance().m_d;
    Radiant::Guard g(d.contextArraysMutex);
    d.contextArrays.erase(contextArray);
  }

  unsigned int RenderManager::driverCount()
  {
    auto & d = *instance().m_d;
    return d.drivers.size();
  }

  int RenderManager::frameTime()
  {
    return instance().m_d->frameTime;
  }

  void RenderManager::updateFrameTime()
  {
    auto & d = *instance().m_d;
    d.frameTime = d.timer.time() * 10;
  }

  RenderManager & RenderManager::instance()
  {
    assert(RenderManager::D::s_instance != 0);
    return *RenderManager::D::s_instance;
  }  

  // Only specialize for valid types
  template <> LUMINOUS_API Buffer * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<Buffer>(id); }
  template <> LUMINOUS_API VertexArray * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<VertexArray>(id); }
  template <> LUMINOUS_API VertexDescription * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<VertexDescription>(id); }
  template <> LUMINOUS_API Texture * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<Texture>(id); }
  template <> LUMINOUS_API Program * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<Program>(id); }
  template <> LUMINOUS_API ShaderGLSL * RenderManager::getResource( RenderResource::Id id ) { return RenderManager::instance().m_d->getResource<ShaderGLSL>(id); }
}
