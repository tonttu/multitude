#include "Luminous/RenderResource.hpp"
#include "Luminous/RenderManager.hpp"
#include "Luminous/RenderDriver.hpp"

namespace Luminous
{
  RenderResource::RenderResource(Type type)
    : m_generation(0)
    , m_id(RenderManager::instance().createResource(this))
    , m_type(type)
    , m_expiration(300)   // Default expiration time: 5 minutes
  {
  }

  RenderResource::~RenderResource()
  {
    RenderManager::instance().destroyResource( resourceId() );
  }
}
