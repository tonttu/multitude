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
    ok = GLX_AMD_gpu_association;
#endif
    //check that extension actually exists
    if (ok) {
      typedef GLuint (GLAPIENTRY * PFNGLXGETGPUIDSAMDPROC) (GLuint maxCount, GLuint* ids);
      PFNGLXGETGPUIDSAMDPROC glXGetGPUIDsAMD = nullptr;
      glXGetGPUIDsAMD = (PFNGLXGETGPUIDSAMDPROC)glXGetProcAddressARB((const GLubyte*)"glXGetGPUIDsAMD");
      if (!glXGetGPUIDsAMD) {
        ok = false;
      }
    }

    Radiant::warning("AMD_GPU_association extension: %s", ok ? "yes" : "no");

    return ok;
  }

  unsigned int GPUAssociation::numGPUs()
  {
    GLuint ids[16] = {0};
    unsigned int count = 0;
	
#if defined(RADIANT_WINDOWS)
    count = wglGetGPUIDsAMD(16, ids);
#elif defined(RADIANT_LINUX)

    typedef GLuint (GLAPIENTRY * PFNGLXGETGPUIDSAMDPROC) (GLuint maxCount, GLuint* ids);

    PFNGLXGETGPUIDSAMDPROC glXGetGPUIDsAMD = nullptr;

    glXGetGPUIDsAMD = (PFNGLXGETGPUIDSAMDPROC)glXGetProcAddressARB((const GLubyte*)"glXGetGPUIDsAMD");
    assert(glXGetGPUIDsAMD);

    count = glXGetGPUIDsAMD(16, ids);
#endif

    return count;
  }

  unsigned int GPUAssociation::gpuId(OpenGLContextHandle* handle)
  {
    unsigned id = 0;

#if defined(RADIANT_WINDOWS)
    id = wglGetContextGPUIDAMD(handle->m_d->ctx);
#elif defined(RADIANT_LINUX)

    typedef GLuint (GLAPIENTRY * PFNGLXGETCONTEXTGPUIDAMDPROC) (GLXContext ctx);

    PFNGLXGETCONTEXTGPUIDAMDPROC glXGetContextGPUIDAMD = nullptr;

    glXGetContextGPUIDAMD = (PFNGLXGETCONTEXTGPUIDAMDPROC)glXGetProcAddressARB((const GLubyte*)"glXGetContextGPUIDAMD");
    assert(glXGetContextGPUIDAMD);

    id = glXGetContextGPUIDAMD(handle->m_d->ctx);
#endif

    return id;
  }
}
