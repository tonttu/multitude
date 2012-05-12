#if !defined (LUMINOUS_RENDERRESOURCE_HPP)
#define LUMINOUS_RENDERRESOURCE_HPP

#include "Luminous/Luminous.hpp"

#include <stdint.h>

namespace Luminous
{
  class RenderResource
  {
  public:
    typedef uint64_t Id;
  public:
    RenderResource(Id id, ResourceType type, RenderDriver & driver);
    virtual ~RenderResource();

    inline Id resourceId() const { return m_id; }
    inline ResourceType resourceType() const { return m_type; }

    inline void setVersion(uint64_t version) { m_version = version; }
    inline uint64_t version() const { return m_version; }
    inline void invalidate() { setVersion(version() + 1); }
  private:
    uint64_t m_version;
    Id m_id;
    ResourceType m_type;
    RenderDriver & m_driver;
  };
}
#endif // LUMINOUS_RENDERRESOURCE_HPP