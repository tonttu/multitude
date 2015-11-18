#include "GPUAssociation.hpp"

#include <Radiant/Platform.hpp>

#include <cassert>

// Platform specific OpenGL context handle. Using these includes in a header
// messes things up with Qt (conflicting #defines), so we have to hide them
// here.
#if defined(RADIANT_WINDOWS)
  #include <GL/wglew.h>
  typedef HGLRC PlatformOpenGLContextHandle;
#elif defined(RADIANT_LINUX)
  #include <GL/glxew.h>
  typedef GLXContext PlatformOpenGLContextHandle;
#elif defined(RADIANT_OSX)
  typedef void* PlatformOpenGLContextHandle;
#endif

namespace Luminous
{
  class OpenGLContextHandle::D
  {
  public:
    D() : ctx(nullptr) {}

    PlatformOpenGLContextHandle ctx;
  };

  OpenGLContextHandle::OpenGLContextHandle()
    : m_d(new D())
  {}

  OpenGLContextHandle::~OpenGLContextHandle()
  {
    delete m_d;
  }

  bool OpenGLContextHandle::getCurrentContext()
  {
    // Grab the OpenGL context handle
#ifdef RADIANT_WINDOWS
    m_d->ctx = wglGetCurrentContext();
#elif defined(RADIANT_LINUX)
    m_d->ctx = glXGetCurrentContext();
#endif
    return m_d->ctx != nullptr;
  }

  void *OpenGLContextHandle::getRawHandle()
  {
    return m_d->ctx;
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  bool GPUAssociation::isSupported()
  {
    bool ok = false;

#if defined(RADIANT_WINDOWS)
    ok = WGLEW_AMD_gpu_association;
#elif defined(RADIANT_LINUX)
    ok = GLXEW_AMD_gpu_association;
#endif

    return ok;
  }

  unsigned int GPUAssociation::numGPUs()
  {
    GLuint ids[16] = {0};
    unsigned int count = 0;
	
#if defined(RADIANT_WINDOWS)
    count = wglGetGPUIDsAMD(16, ids);
#elif defined(RADIANT_LINUX)
    count = glXGetGPUIDsAMD(16, ids);
#endif

    return count;
  }

  unsigned int GPUAssociation::gpuId(OpenGLContextHandle* handle)
  {
    unsigned int id = 0;

#if defined(RADIANT_WINDOWS)
    id = wglGetContextGPUIDAMD(handle->m_d->ctx);
#elif defined(RADIANT_LINUX)
    id = glXGetContextGPUIDAMD(handle->m_d->ctx);
#endif

    return id;
  }

  unsigned int GPUAssociation::gpuRam(unsigned int gpuId)
  {
    GLuint totalMemoryInMB = 0;
#if defined(RADIANT_WINDOWS)
    wglGetGPUInfoAMD(gpuId, WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(GLuint), &totalMemoryInMB);
#elif defined(RADIANT_LINUX)
    glXGetGPUInfoAMD(gpuId, GLX_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(GLuint), &totalMemoryInMB);
#endif

    return totalMemoryInMB;
  }

}
