#if !defined (LUMINOUS_VERTEXATTRIBUTEBINDING_HPP)
#define LUMINOUS_VERTEXATTRIBUTEBINDING_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/VertexDescription.hpp"

#include <Radiant/RefPtr.hpp>
#include <vector>

namespace Luminous
{
  class VertexAttributeBinding
  {
  public:
    virtual void addBinding(const std::shared_ptr<HardwareBuffer> & buffer, const std::shared_ptr<VertexDescription> & description) = 0;
    virtual void removeBinding(const std::shared_ptr<HardwareBuffer> & buffer) = 0;

    virtual void clear() = 0;

    virtual void bind(unsigned int threadIndex) = 0;
    virtual void unbind(unsigned int threadIndex) = 0;
  };
}

#endif // LUMINOUS_VERTEXATTRIBUTEBINDING_HPP
