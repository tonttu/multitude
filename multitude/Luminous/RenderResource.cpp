#include "Luminous/RenderResource.hpp"
#include "Luminous/RenderManager.hpp"
#include "Luminous/RenderDriver.hpp"

namespace Luminous
{
  int RenderResource::s_generation = 1;

  RenderResource::RenderResource(Type type)
    : m_generation(0)
    , m_id(RenderManager::createResource(this))
    , m_type(type)
    , m_expiration(300)   // Default expiration time: 5 minutes
  {
  }

  RenderResource::~RenderResource()
  {
    if(m_id != Id(-1))
      RenderManager::destroyResource( resourceId() );
  }

  RenderResource::RenderResource(RenderResource && rr)
    : m_generation(rr.m_generation)
    , m_id(rr.m_id)
    , m_type(rr.m_type)
    , m_expiration(rr.m_expiration)
  {
    rr.m_id = Id(-1);
    RenderManager::updateResource(m_id, this);
  }

  RenderResource & RenderResource::operator=(RenderResource && rr)
  {
    RenderManager::destroyResource(resourceId());
    m_generation = rr.m_generation;
    m_id = rr.m_id;
    m_type = rr.m_type;
    m_expiration = rr.m_expiration;
    rr.m_id = Id(-1);
    RenderManager::updateResource(m_id, this);
    return *this;
  }
}
