#if !defined (LUMINOUS_HARDWAREBUFFERGL_HPP)
#define LUMINOUS_HARDWAREBUFFERGL_HPP

#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/RenderResource.hpp"
#include "Luminous/GLUtils.hpp"

namespace Luminous
{
  class HardwareBufferGL
    : public HardwareBuffer
    , public RenderResource
  {
  public:
    HardwareBufferGL(RenderResource::Id id, BufferType bufferType, unsigned int threadCount);
    ~HardwareBufferGL();

    // HardwareBuffer interface
    virtual void reallocate(size_t bytes, BufferUsage usage) OVERRIDE;
    virtual size_t size() OVERRIDE;

    virtual void read(char * data, size_t bytes, size_t offset) const OVERRIDE;
    virtual void write(const char * data, size_t bytes, size_t offset) OVERRIDE;

    virtual void bind(unsigned int threadIndex) OVERRIDE;
    virtual void unbind(unsigned int threadIndex) OVERRIDE;

    virtual BufferType type() const OVERRIDE;
    virtual BufferUsage usage() const OVERRIDE;

    // RenderResource interface
    virtual void initializeResources(unsigned int threadIndex) OVERRIDE;
    virtual void reallocateResources(unsigned int threadIndex) OVERRIDE;
    virtual void updateResources(unsigned int threadIndex) OVERRIDE;
    virtual void deinitializeResources(unsigned int threadIndex) OVERRIDE;

    //
    GLuint handle(unsigned int threadIndex) const;

  private:
    class Impl;
    Impl * m_impl;
  };
}
#endif // LUMINOUS_HARDWAREBUFFERGL_HPP