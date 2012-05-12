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
    inline RenderResource(Id id, ResourceType type)
      : m_id(id)
      , m_version(0)
      , m_type(type)
    {}

    virtual inline ~RenderResource() {}

    inline Id resourceId() const { return m_id; }
    inline ResourceType resourceType() const { return m_type; }

    inline void setVersion(uint64_t version) { m_version = version; }
    inline uint64_t version() const { return m_version; }
    inline void invalidate() { setVersion(version() + 1); }
  private:
    uint64_t m_version;
    Id m_id;
    ResourceType m_type;
  };
}
#endif // LUMINOUS_RENDERRESOURCE_HPP