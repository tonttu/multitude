#ifndef GPUASSOCIATION_HPP
#define GPUASSOCIATION_HPP

#include "Export.hpp"
#include "Luminous.hpp"

#include <Patterns/NotCopyable.hpp>

namespace Luminous
{  
  class OpenGLContextHandle : public Patterns::NotCopyable
  {
  public:
    OpenGLContextHandle();
    ~OpenGLContextHandle();

    bool getCurrentContext();

    void* getRawHandle();

  private:
    class D;
    D * m_d;

    friend class GPUAssociation;
  };


  class GPUAssociation
  {
  public:
    /// Is GPU association supported?
    static LUMINOUS_API bool isSupported();

    /// Get number of GPUs on the system
    static LUMINOUS_API unsigned int numGPUs();
    /// Get GPU id for the given OpenGL context
    static LUMINOUS_API unsigned int gpuId(OpenGLContextHandle *handle);
    /// Get amount of RAM available on the given GPU in MB
    static LUMINOUS_API unsigned int gpuRam(unsigned int gpuId);
  };
}

#endif // GPUASSOCIATION_HPP
