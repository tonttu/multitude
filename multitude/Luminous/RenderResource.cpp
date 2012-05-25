#include "Luminous/RenderResource.hpp"
#include "Luminous/RenderDriver.hpp"

namespace Luminous
{
  RenderResource::RenderResource(Id id, ResourceType type, RenderDriver & driver)
    : m_id(id)
    , m_generation(0)
    , m_type(type)
    , m_driver(driver)
  {
  }

  RenderResource::~RenderResource()
  {
    m_driver.removeResource( resourceId() );
  }
}