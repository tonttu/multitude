#include "GPUAssociation.hpp"
#include "Luminous.hpp"

#include <Radiant/Platform.hpp>

#include <cassert>

namespace Luminous
{

  /// @todo fixme esa re-implement
  bool GPUAssociation::isSupported()
  {
    bool ok = false;

//#if defined(RADIANT_WINDOWS)
//    ok = WGLEW_AMD_gpu_association;
//#elif defined(RADIANT_LINUX)
//    ok = GLXEW_AMD_gpu_association;
//#endif

    return ok;
  }

  unsigned int GPUAssociation::numGPUs()
  {
    unsigned int count = 0;

//#if defined(RADIANT_WINDOWS)
//    count = wglGetGPUIDsAMD(0, nullptr);
//#elif defined(RADIANT_LINUX)
//    count = glXGetGPUIDsAMD(0, nullptr);
//#endif

    return count;
  }

  unsigned int GPUAssociation::gpuId(glbinding::ContextHandle handle)
  {
    unsigned int id = 0;

//#if defined(RADIANT_WINDOWS)
//    id = wglGetContextGPUIDAMD(handle->m_d->ctx);
//#elif defined(RADIANT_LINUX)
//    id = glXGetContextGPUIDAMD(handle->m_d->ctx);
//#endif

    return id;
  }

  unsigned int GPUAssociation::gpuRam(unsigned int gpuId)
  {
    GLuint totalMemoryInMB = 0;
//#if defined(RADIANT_WINDOWS)
//    wglGetGPUInfoAMD(gpuId, WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, 1, &totalMemoryInMB);
//#elif defined(RADIANT_LINUX)
//    glXGetGPUInfoAMD(gpuId, GLX_GPU_RAM_AMD, GL_UNSIGNED_INT, 1, &totalMemoryInMB);
//#endif

    return totalMemoryInMB;
  }

}
