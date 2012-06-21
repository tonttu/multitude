#include "Luminous/RenderResource.hpp"
#include "Luminous/RenderManager.hpp"
#include "Luminous/RenderDriver.hpp"

namespace Luminous
{
  RenderResource::RenderResource(ResourceType type)
    : m_generation(0)
    , m_id(RenderManager::instance().createResource())
    , m_type(type)
  {
  }

  RenderResource::~RenderResource()
  {
    RenderManager::instance().destroyResource( resourceId() );
  }
}
