/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "GPUAssociation.hpp"

#include <Radiant/Platform.hpp>

#include <cassert>

#if defined(RADIANT_LINUX)
# include <GL/glx.h>
# include <QtPlatformHeaders/QGLXNativeContext>
#elif defined(RADIANT_WINDOWS)
# include <Windows.h>
# include <QtPlatformHeaders/QWGLNativeContext>
#endif

namespace Luminous
{

  bool GPUAssociation::isSupported()
  {
    bool ok = false;

#if defined(RADIANT_WINDOWS)
    ok = (wglGetProcAddress("wglGetGPUIDsAMD") != nullptr);
#elif defined(RADIANT_LINUX)
    ok = (glXGetProcAddress((GLubyte*)("glXGetGPUIDsAMD")) != nullptr);
#endif

    return ok;
  }

  unsigned int GPUAssociation::numGPUs()
  {
    unsigned int count = 0;

#if defined(RADIANT_WINDOWS)
    using functionPtr = UINT (*)(UINT maxCount, UINT *ids);

    auto wglGetGPUIDsAMD = (functionPtr)wglGetProcAddress("wglGetGPUIDsAMD");

    count = wglGetGPUIDsAMD(0, nullptr);
#elif defined(RADIANT_LINUX)
    using functionPtr = unsigned int (*)(unsigned int maxCount, unsigned int *ids);

    auto glXGetGPUIDsAMD = (functionPtr)glXGetProcAddress((GLubyte*)"glXGetGPUIDsAMD");

    count = glXGetGPUIDsAMD(0, nullptr);
#endif

    return count;
  }

  unsigned int GPUAssociation::gpuId(QOpenGLContext& context)
  {
    unsigned int id = 0;

#if defined(RADIANT_WINDOWS)
    QWGLNativeContext nativeContext = context.nativeHandle().value<QWGLNativeContext>();
    auto wglContext = nativeContext.context();

    using functionPtr = UINT (*)(HGLRC hglrc);
    auto wglGetContextGPUIDAMD = (functionPtr)wglGetProcAddress("wglGetContextGPUIDAMD");

    id = wglGetContextGPUIDAMD(wglContext);

#elif defined(RADIANT_LINUX)
    QGLXNativeContext nativeContext = context.nativeHandle().value<QGLXNativeContext>();
    auto glxContext = nativeContext.context();

    using functionPtr = unsigned int (*)(GLXContext ctx);
    auto glXGetContextGPUIDAMD = (functionPtr)glXGetProcAddress((GLubyte*)"glXGetContextGPUIDAMD");

    id = glXGetContextGPUIDAMD(glxContext);

#else
    (void)context;
#endif

    return id;
  }

  unsigned int GPUAssociation::gpuRam(unsigned int gpuId)
  {
    unsigned int totalMemoryInMB = 0;
#if defined(RADIANT_WINDOWS)
    using functionPtr = int (*)(unsigned int id, int property, GLenum dataType, unsigned int size, void *data);
    const int WGL_GPU_RAM_AMD = 0x21A3;

    auto wglGetGPUInfoAMD = (functionPtr)wglGetProcAddress("wglGetGPUInfoAMD");

    wglGetGPUInfoAMD(gpuId, WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, 1, &totalMemoryInMB);

#elif defined(RADIANT_LINUX)
    using functionPtr = int (*)(unsigned int id, int property, GLenum dataType, unsigned int size, void *data);

    auto glXGetGPUInfoAMD = (functionPtr)glXGetProcAddress((GLubyte*)"glXGetGPUInfoAMD");

    glXGetGPUInfoAMD(gpuId, GLX_GPU_RAM_AMD, GL_UNSIGNED_INT, 1, &totalMemoryInMB);

#else
    (void)gpuId;
#endif

    return totalMemoryInMB;
  }

}
