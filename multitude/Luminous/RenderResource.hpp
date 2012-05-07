#if !defined (LUMINOUS_RENDERRESOURCE_HPP)
#define LUMINOUS_RENDERRESOURCE_HPP

#include "Luminous/Luminous.hpp"
#include <stdint.h>

namespace Luminous
{
  class RenderResource
  {
  public:
    virtual ~RenderResource();

    typedef uint64_t Id;
    Id resourceId() const;
    int threadCount() const;

  protected:
    RenderResource(int threadCount);
    // Instruct the resource to reallocate the GPU data
    void reallocateGPU();
    // Instruct the resource to update the GPU data
    void updateGPU();

    virtual void initializeResources(int threadIndex);
    virtual void reallocateResources(int threadIndex);
    virtual void updateResources(int threadIndex);
    virtual void deinitializeResources(int threadIndex);

  private:
    // Update the GPU resources (called by the resource manager)
    void update(int threadIndex);
    // Returns true if all resources have been released (called by the resource manager)
    bool released() const;

  private:
    class Impl;
    Impl * m_impl;
  };
}
#endif // LUMINOUS_RENDERRESOURCE_HPP