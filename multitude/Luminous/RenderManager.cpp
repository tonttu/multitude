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
    D(Luminous::RenderDriver & driver)
      : driver(driver)
      , resourceId(0)
    {
    }

    typedef std::map<RenderResource::Id, RenderResource *> ResourceMap;
    ResourceMap resourceMap;

    RenderResource::Id resourceId;      // Next available resource ID
    Luminous::RenderDriver & driver;    // Currently used driver
    static RenderManager * s_instance;  // Singleton instance
  };

  RenderManager * RenderManager::D::s_instance = 0;

  RenderManager::RenderManager(Luminous::RenderDriver & driver)
    : m_d(new RenderManager::D(driver))
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

  RenderResource::Id RenderManager::createResource(RenderResource * resource)
  {
    auto id = m_d->resourceId++;
    m_d->resourceMap[id] = resource;
    return id;
  }

  void RenderManager::destroyResource(RenderResource::Id id)
  {
    m_d->resourceMap.erase(id);
    m_d->driver.releaseResource(id);
  }

  RenderDriver & RenderManager::driver()
  {
    return m_d->driver;
  }

  RenderManager & RenderManager::instance()
  {
    assert(RenderManager::D::s_instance != 0);
    return *RenderManager::D::s_instance;
  }  

  HardwareBuffer * RenderManager::getBuffer( RenderResource::Id id )
  {
    auto buffer = RenderManager::instance().m_d->resourceMap.find(id);
    if (buffer != std::end(RenderManager::instance().m_d->resourceMap))
      return reinterpret_cast<HardwareBuffer*>(buffer->second);
    return nullptr;
  }

  VertexDescription * RenderManager::getVertexDescription( RenderResource::Id id )
  {
    auto descr = RenderManager::instance().m_d->resourceMap.find(id);
    if (descr != std::end(RenderManager::instance().m_d->resourceMap))
      return reinterpret_cast<VertexDescription*>(descr->second);
    return nullptr;
  }
}
