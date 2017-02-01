#include "SwapGroups.hpp"

#include <Radiant/Platform.hpp>

namespace Luminous {

#ifdef RADIANT_LINUX

#include <GL/glx.h>

  bool SwapGroups::isExtensionSupported()
  {
    return (glXGetProcAddress((GLubyte*)"glXJoinSwapGroupNV") != nullptr);
  }

  bool SwapGroups::queryMaxSwapGroup(GLuint& maxGroups, GLuint& maxBarriers)
  {
    using functionPtr = Bool (*)(Display *dpy, int screen, GLuint *maxGroups, GLuint *maxBarriers);

    auto glXQueryMaxSwapGroupsNV = (functionPtr)glXGetProcAddress((GLubyte*)"glXQueryMaxSwapGroupsNV");

    auto dpy = glXGetCurrentDisplay();
    auto screen = DefaultScreen(dpy);

    return glXQueryMaxSwapGroupsNV(dpy, screen, &maxGroups, &maxBarriers);
  }

  bool SwapGroups::joinSwapGroup(GLuint group)
  {
    using functionPtr = Bool (*)(Display *dpy, GLXDrawable drawable, GLuint group);

    auto glXJoinSwapGroupNV = (functionPtr)glXGetProcAddress((GLubyte*)"glXJoinSwapGroupNV");

    auto dpy = glXGetCurrentDisplay();
    auto drawable = glXGetCurrentDrawable();

    return glXJoinSwapGroupNV(dpy, drawable, group);
  }

  bool SwapGroups::bindSwapBarrier(GLuint group, GLuint barrier)
  {
    using functionPtr = Bool (*)(Display *dpy, GLuint group, GLuint barrier);

    auto glXBindSwapBarrierNV = (functionPtr)glXGetProcAddress((GLubyte*)"glXBindSwapBarrierNV");

    auto dpy = glXGetCurrentDisplay();

    return glXBindSwapBarrierNV(dpy, group, barrier);
  }

  bool SwapGroups::querySwapGroup(GLuint& group, GLuint& barrier)
  {
    using functionPtr = Bool (*)(Display *dpy, GLXDrawable drawable, GLuint *group, GLuint *barrier);

    auto glXQuerySwapGroupNV = (functionPtr)glXGetProcAddress((GLubyte*)"glXQuerySwapGroupNV");

    auto dpy = glXGetCurrentDisplay();
    auto drawable = glXGetCurrentDrawable();

    return glXQuerySwapGroupNV(dpy, drawable, &group, &barrier);
  }

#elif defined(RADIANT_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

  bool SwapGroups::isExtensionSupported()
  {
    return wglGetProcAddress("wglJoinSwapGroupNV") != nullptr;
  }

  bool SwapGroups::queryMaxSwapGroup(GLuint& maxGroups, GLuint& maxBarriers)
  {
    using functionPtr = BOOL (*)(HDC hDC, GLuint *maxGroups, GLuint *maxBarriers);

    auto dc = wglGetCurrentDC();
    auto wglQueryMaxSwapGroupsNV = (functionPtr)wglGetProcAddress("wglQueryMaxSwapGroupsNV");

    return wglQueryMaxSwapGroupsNV(dc, &maxGroups, &maxBarriers);
  }

  bool SwapGroups::joinSwapGroup(GLuint group)
  {
    using functionPtr = BOOL (*)(HDC hDC, GLuint group);

    auto dc = wglGetCurrentDC();
    auto wglJoinSwapGroupNV = (functionPtr)wglGetProcAddress("wglJoinSwapGroupNV");

    return wglJoinSwapGroupNV(dc, group);
  }

  bool SwapGroups::bindSwapBarrier(GLuint group, GLuint barrier)
  {
    using functionPtr = BOOL (*)(GLuint group, GLuint barrier);

    auto wglBindSwapBarrierNV = (functionPtr)wglGetProcAddress("wglBindSwapBarrierNV");

    return wglBindSwapBarrierNV(group, barrier);
  }

  bool SwapGroups::querySwapGroup(GLuint& group, GLuint& barrier)
  {
    using functionPtr = BOOL (*)(HDC hDC, GLuint *group, GLuint *barrier);

    auto dc = wglGetCurrentDC();
    auto wglQuerySwapGroupNV = (functionPtr)wglGetProcAddress("wglQuerySwapGroupNV");

    return wglQuerySwapGroupNV(dc, &group, &barrier);
  }

#else

  bool SwapGroups::isExtensionSupported()
  {
    return false;
  }

  bool SwapGroups::queryMaxSwapGroup(GLuint & /*maxGroups*/, GLuint & /*maxBarriers*/)
  {
    return false;
  }

  bool SwapGroups::joinSwapGroup(GLuint /*group*/)
  {
    return false;
  }

  bool SwapGroups::bindSwapBarrier(GLuint /*group*/, GLuint /*barrier*/)
  {
    return false;
  }

  bool SwapGroups::querySwapGroup(GLuint& /*group*/, GLuint& /*barrier*/)
  {
    return false;
  }

#endif

}
