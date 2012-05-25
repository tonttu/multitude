#if !defined (LUMINOUS_RENDERRESOURCE_HPP)
#define LUMINOUS_RENDERRESOURCE_HPP

#include "Luminous/Luminous.hpp"
#include "Patterns/NotCopyable.hpp"

#include <stdint.h>

namespace Luminous
{
  class RenderResource : public Patterns::NotCopyable
  {
  public:
    typedef uint64_t Id;
  public:
    RenderResource(Id id, ResourceType type, RenderDriver & driver);
    virtual ~RenderResource();

    inline Id resourceId() const { return m_id; }
    inline ResourceType resourceType() const { return m_type; }

    inline void setGeneration(uint64_t generation) { m_generation = generation; }
    inline uint64_t generation() const { return m_generation; }
    inline void invalidate() { m_generation++; }
  private:
    uint64_t m_generation;
    Id m_id;
    ResourceType m_type;
    RenderDriver & m_driver;
  };
}
#endif // LUMINOUS_RENDERRESOURCE_HPP