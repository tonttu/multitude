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

    enum Type
    {
      VertexArray,
      Buffer,
      ShaderProgram,
      VertexShader,
      VertexDescription,
      FragmentShader,
      GeometryShader,
      Texture,
    };

  public:
    LUMINOUS_API RenderResource(Type type);
    LUMINOUS_API virtual ~RenderResource();

    inline Id resourceId() const { return m_id; }
    inline Type resourceType() const { return m_type; }

    inline void setGeneration(uint64_t generation) { m_generation = generation; }
    inline uint64_t generation() const { return m_generation; }
    inline void invalidate() { m_generation++; }

    // Set resource expiration time. The resource will be released after it has not been used for this period
    inline void setExpiration(unsigned int seconds) { m_expiration = seconds; }
    // Returns resource expiration time
    inline unsigned int expiration() const { return m_expiration; }
  private:
    uint64_t m_generation;
    Id m_id;
    unsigned int m_expiration;
    Type m_type;
  };
}
#endif // LUMINOUS_RENDERRESOURCE_HPP