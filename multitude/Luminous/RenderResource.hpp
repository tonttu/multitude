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
    inline RenderResource(Id id)
      : m_id(id)
      , m_version(0)
    {}

    virtual inline ~RenderResource() {}

    inline Id resourceId() const { return m_id; }

    inline void setVersion(uint64_t version) { m_version = version; }
    inline uint64_t version() const { return m_version; }
    inline void invalidate() { setVersion(version() + 1); }
  private:
    uint64_t m_version;
    Id m_id;
  };
}
#endif // LUMINOUS_RENDERRESOURCE_HPP