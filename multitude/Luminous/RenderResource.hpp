#if !defined (LUMINOUS_RENDERRESOURCE_HPP)
#define LUMINOUS_RENDERRESOURCE_HPP

#include "Luminous/Luminous.hpp"
#include "Patterns/NotCopyable.hpp"

#include <unordered_map>

#include <cstdint>

namespace Luminous
{
  class LUMINOUS_API RenderResource
  {
  public:
    struct Hash
    {
      uint64_t data[2];
      inline bool operator<(const Hash & h) const
      {
        return data[0] == h.data[0] ? data[1] < h.data[1] : data[0] < h.data[0];
      }
      inline bool operator==(const Hash & h) const
      {
        return data[0] == h.data[0] && data[1] == h.data[1];
      }
    };

    typedef uint64_t Id;

    enum Type
    {
      VertexArray,
      Buffer,
      Program,
      Texture,
      RenderBuffer,
      FrameBuffer
    };

  public:
    RenderResource(Type type);
    virtual ~RenderResource();

    RenderResource(RenderResource && rr);
    RenderResource & operator=(RenderResource && rr);

    inline Id resourceId() const { return m_id; }
    inline Type resourceType() const { return m_type; }

    inline int generation() const { return m_generation; }
    inline void setGeneration(int generation) { m_generation = generation; }
    inline void invalidate() { ++m_generation ; }

    // Set resource expiration time. The resource will be released after it has not been used for this period
    inline void setExpiration(unsigned int seconds) { m_expiration = seconds; }
    // Returns resource expiration time
    inline unsigned int expiration() const { return m_expiration; }

  protected:
    RenderResource(RenderResource & rr);
    RenderResource & operator=(RenderResource & rr);

  private:
    int m_generation;
    Id m_id;
    Type m_type;
    unsigned int m_expiration;
  };
}

namespace std
{
  template<> struct hash<Luminous::RenderResource::Hash>
  {
    inline size_t operator()(const Luminous::RenderResource::Hash & hash) const
    {
      std::hash<uint64_t> hasher;
      return hasher(hash.data[0]) ^ hasher(hash.data[1]);
    }
  };
}

#endif // LUMINOUS_RENDERRESOURCE_HPP
