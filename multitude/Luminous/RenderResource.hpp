/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (LUMINOUS_RENDERRESOURCE_HPP)
#define LUMINOUS_RENDERRESOURCE_HPP

#include "Luminous/Luminous.hpp"
#include "Patterns/NotCopyable.hpp"

#include <unordered_map>

#include <cstdint>

namespace Luminous
{

  /// This class provides common interface for different resources needed
  /// during rendering.
  class LUMINOUS_API RenderResource
  {
  public:

    /// Hash used to identify rendering resources
    struct Hash
    {
      /// Hash data
      uint64_t data[2];

      /// Compare the order of hashes
      /// @param h hash to compare
      /// @return true if this hash is smaller than the given hash
      inline bool operator<(const Hash & h) const
      {
        return data[0] == h.data[0] ? data[1] < h.data[1] : data[0] < h.data[0];
      }

      /// Compare two hashes for equality
      /// @param h hash to compare
      /// @return true if the hashes are equal; otherwise false
      inline bool operator==(const Hash & h) const
      {
        return data[0] == h.data[0] && data[1] == h.data[1];
      }
    };

    /// Id of a resource
    typedef uint64_t Id;

    /// Different types of render resources
    enum Type
    {
      /// Vertex array @sa VertexArray
      VertexArray,
      /// Buffer for vertex or index data @sa Buffer
      Buffer,
      /// Shader program consisting of multiple independent shaders @sa Shader
      Program,
      /// Texture @sa Texture
      Texture,
      /// Render buffer @sa RenderBuffer
      RenderBuffer,
      /// Frame buffer @sa FrameBuffer
      FrameBuffer
    };

  public:
    /// Constructor of RenderResource
    /// @param type Type of the resource
    RenderResource(Type type);
    /// Destructor of RenderResource
    virtual ~RenderResource();

    /// Move constructor
    /// @param rr Resource to move
    RenderResource(RenderResource && rr);
    /// Move assignment operator
    /// @param rr RenderResource to move
    /// @return Reference to this
    RenderResource & operator=(RenderResource && rr);

    /// Returns identifier of resource. It is guaranteed that each RenderResource has
    /// unique id regardless of its type.
    /// @return Identifier of this resource
    inline Id resourceId() const { return m_id; }
    /// Returns type of the resource
    /// @return Resource type
    inline Type resourceType() const { return m_type; }

    /// Returns generation of this resource. If resource and its corresponding GPU class have a different
    /// generation data is updated on GPU.
    /// @return Generation of this object
    inline int generation() const { return m_generation; }
    /// Sets generation for this resource
    /// @param generation New generation
    inline void setGeneration(int generation) { m_generation = generation; }
    /// Invalidates correspondent GPU objects and forces uploading of data to GPU
    inline void invalidate() { ++m_generation ; }

    /// Set resource expiration time. The resource will be released after it has not been used for this period
    /// @param seconds How many seconds after usage we store this object
    inline void setExpiration(unsigned int seconds) { m_expiration = seconds; }
    /// Returns resource expiration time
    /// @return How many seconds after usage we store this object
    inline unsigned int expiration() const { return m_expiration; }

  protected:
    /// Copy constructor
    /// @param rr resource to copy
    RenderResource(const RenderResource & rr);
    /// Assignment operator
    /// @param rr resource to copy
    /// @return reference to this
    RenderResource & operator=(const RenderResource & rr);

  private:
    int m_generation;
    Id m_id;
    Type m_type;
    unsigned int m_expiration;
  };
}

/// @cond

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

/// @endcond

#endif // LUMINOUS_RENDERRESOURCE_HPP
