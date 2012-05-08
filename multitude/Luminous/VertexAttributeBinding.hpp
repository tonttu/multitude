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
    VertexAttributeBinding();
    ~VertexAttributeBinding();

    void addBinding(const std::shared_ptr<HardwareBuffer> & buffer, const std::shared_ptr<VertexDescription> & description);
    void removeBinding(const std::shared_ptr<HardwareBuffer> & buffer);

    void clear();

    virtual void bind(unsigned int threadIndex) = 0;
    virtual void unbind(unsigned int threadIndex) = 0;

    bool dirty() const;
  protected:
    void setDirty(bool dirty);
  private:
    class Impl;
    Impl * m_impl;
  };
}

#endif // LUMINOUS_VERTEXATTRIBUTEBINDING_HPP
