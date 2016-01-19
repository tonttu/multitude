#ifndef GPUASSOCIATION_HPP
#define GPUASSOCIATION_HPP

#include "Export.hpp"

#include <glbinding/ContextHandle.h>

namespace Luminous
{  

  class GPUAssociation
  {
  public:
    /// Is GPU association supported?
    static LUMINOUS_API bool isSupported();

    /// Get number of GPUs on the system
    static LUMINOUS_API unsigned int numGPUs();
    /// Get GPU id for the given OpenGL context
    static LUMINOUS_API unsigned int gpuId(glbinding::ContextHandle handle);
    /// Get amount of RAM available on the given GPU in MB
    static LUMINOUS_API unsigned int gpuRam(unsigned int gpuId);
  };
}

#endif // GPUASSOCIATION_HPP
